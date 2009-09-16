/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>                         *
 * Copyright (c) 2005-2006 Alexandre Pereira de Oliveira <aleprj@gmail.com>             *
 * Copyright (c) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                             *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "TagDialog"

#include "TagDialog.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "CollectionManager.h"
#include "covermanager/CoverFetcher.h"
#include "covermanager/CoverFetchingActions.h"
#include "Debug.h"
#include "EditCapability.h"
#include "FilenameLayoutDialog.h"
#include "MainWindow.h"
#include "MetaQueryMaker.h"
#include "MetaUtility.h"
#include "QueryMaker.h"
#include "statusbar/StatusBar.h"       //for status messages
#include "TagGuesser.h"
#include "ui_TagDialogBase.h"
#include "UpdateCapability.h"

#include <KGlobal>
#include <KHTMLView>
#include <KLineEdit>
#include <KMenu>
#include <KRun>

#include <qdom.h>
#include <QMap>
#include <QPair>
#include <QTimer>

TagDialog::TagDialog( const Meta::TrackList &tracks, QWidget *parent )
    : KDialog( parent )
    , m_currentCover()
    , m_tracks( tracks )
    , m_trackIterator( m_tracks )
    , m_queryMaker( 0 )
    , ui( new Ui::TagDialogBase() )
{
    DEBUG_BLOCK

    setCurrentTrack( m_tracks.first() );

    ui->setupUi( mainWidget() );
    resize( minimumSizeHint() ); 
    init();
    startDataQuery();
}

TagDialog::TagDialog( Meta::TrackPtr track, QWidget *parent )
    : KDialog( parent )
    , m_currentCover()
    , m_tracks()
    , m_trackIterator( m_tracks )   //makes the compiler happy
    , m_queryMaker( 0 )
    , ui( new Ui::TagDialogBase() )
{
    DEBUG_BLOCK

    setCurrentTrack( m_tracks.first() );

    m_tracks.append( track );
    //we changed the list after creating the iterator, so create a new iterator
    m_trackIterator = QListIterator<Meta::TrackPtr >( m_tracks );
    ui->setupUi( mainWidget() );
    resize( minimumSizeHint() ); 
    init();
    startDataQuery();
}

TagDialog::TagDialog( QueryMaker *qm )
    : KDialog( The::mainWindow() )
    , m_currentCover()
    , m_tracks()
    , m_currentTrack()
    , m_trackIterator( m_tracks )
    , m_queryMaker( qm )
    , ui( new Ui::TagDialogBase() )
{
    DEBUG_BLOCK

    ui->setupUi( mainWidget() );
    resize( minimumSizeHint() ); 
    startDataQuery();
    qm->setQueryType( QueryMaker::Track );
    connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), this, SLOT( resultReady( QString, Meta::TrackList ) ), Qt::QueuedConnection );
    connect( qm, SIGNAL( queryDone() ), this, SLOT( queryDone() ), Qt::QueuedConnection );
    qm->run();
}

TagDialog::~TagDialog()
{
    DEBUG_BLOCK

    Amarok::config( "TagDialog" ).writeEntry( "CurrentTab", ui->kTabWidget->currentIndex() );

    if( m_currentTrack && m_currentTrack->album() )
        unsubscribeFrom( m_currentTrack->album() );

    // NOTE: temporary crash prevention.  TreeView should _always_ be updated
    // if tags have changed.

    if ( !m_tracks.isEmpty() )
        delete m_labelCloud;
    else
        debug() << "Empty tracklist?  Must mean TreeView has not been updated!";

    delete ui;
}

void
TagDialog::setTab( int id )
{
    ui->kTabWidget->setCurrentIndex( id );
}

void
TagDialog::setCurrentTrack( Meta::TrackPtr track )
{
    DEBUG_BLOCK
    if( m_currentTrack && m_currentTrack->album() )
        unsubscribeFrom( m_currentTrack->album() );

    m_currentTrack = track;

    if( m_currentTrack && m_currentTrack->album() )
        subscribeTo( m_currentTrack->album() );
}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
////////////////////////////////////////////////////////////////////////////////

void
TagDialog::resultReady( const QString &collectionId, const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    Q_UNUSED( collectionId )

    m_tracks << tracks;
    foreach( Meta::TrackPtr d_track, tracks )
    {
        if ( d_track && d_track->artist() )
            debug() << "Artist is: " << d_track->artist()->name();
    }
}

void
TagDialog::queryDone()
{
    DEBUG_BLOCK

    delete m_queryMaker;
    m_trackIterator = QListIterator<Meta::TrackPtr >( m_tracks );
    if( m_tracks.size() )
    {
        setCurrentTrack( m_tracks.first() );
        init();
        QTimer::singleShot( 0, this, SLOT( show() ) );
    }
    else
    {
        deleteLater();
    }
}

void
TagDialog::resultReady( const QString &collectionId, const Meta::AlbumList &albums )
{
    Q_UNUSED( collectionId )

    foreach( Meta::AlbumPtr album, albums )
    {
        if( !album->name().isEmpty() )
            m_albums << album->name();
    }

    m_albums.sort();
}

void
TagDialog::resultReady( const QString &collectionId, const Meta::ArtistList &artists )
{
    Q_UNUSED( collectionId )

    foreach( Meta::ArtistPtr artist, artists )
    {
        if( !artist->name().isEmpty() )
            m_artists << artist->name();
    }

    m_artists.sort();
}

void
TagDialog::resultReady( const QString &collectionId, const Meta::ComposerList &composers )
{
    Q_UNUSED( collectionId )

    foreach( Meta::ComposerPtr composer, composers )
    {
        if( !composer->name().isEmpty() )
            m_composers << composer->name();
    }

    m_composers.sort();
}

void
TagDialog::resultReady( const QString &collectionId, const Meta::GenreList &genres )
{
    Q_UNUSED( collectionId )

    foreach( Meta::GenrePtr genre, genres )
    {
        if( !genre->name().isEmpty() )  // Where the heck do the empty genres come from?
            m_genres << genre->name();
    }

    m_genres.sort();
}

void
TagDialog::dataQueryDone()
{
    DEBUG_BLOCK

    m_dataQueryMaker->deleteLater();
    m_dataQueryMaker = 0;

    // basically we want to ignore the fact that the fields are being
    // edited because we do it not the user, so it results in empty
    // tags being saved to files---data loss is BAD!
    QMap< QString, bool > m_fieldEditedSave = m_fieldEdited;

    //we simply clear the completion data of all comboboxes
    //then load the current track again. that's more work than necessary
    //but the performance impact should be negligible
    // we do this because if we insert items and the contents of the textbox
    // are not in the list, it clears the textbox. which is bad --lfranchi 2.22.09
    QString saveText( ui->kComboBox_artist->lineEdit()->text() );
    ui->kComboBox_artist->clear();
    ui->kComboBox_artist->insertItems( 0, m_artists );
    ui->kComboBox_artist->lineEdit()->setText( saveText );

    saveText = ui->kComboBox_album->lineEdit()->text();
    ui->kComboBox_album->clear();
    ui->kComboBox_album->insertItems( 0, m_albums );
    ui->kComboBox_album->lineEdit()->setText( saveText );

    saveText = ui->kComboBox_composer->lineEdit()->text();
    ui->kComboBox_composer->clear();
    ui->kComboBox_composer->insertItems( 0, m_composers );
    ui->kComboBox_composer->lineEdit()->setText( saveText );

    saveText = ui->kComboBox_genre->lineEdit()->text();
    ui->kComboBox_genre->clear();
    ui->kComboBox_genre->insertItems( 0, m_genres );
    ui->kComboBox_genre->lineEdit()->setText( saveText );

   

    if( !m_queryMaker )  //track query complete or not necessary
    {
        if( m_perTrack )
        {
            readTags();
        }
        else
        {
            readMultipleTracks();
        }
    }

    m_fieldEdited = m_fieldEditedSave;
}

void
TagDialog::cancelPressed() //SLOT
{
    DEBUG_BLOCK

    QApplication::restoreOverrideCursor(); // restore the cursor before closing the dialog
    reject();
}


void
TagDialog::accept() //SLOT
{
    DEBUG_BLOCK

    ui->pushButton_ok->setEnabled( false ); //visual feedback
    saveTags();

    KDialog::accept();
}


inline void
TagDialog::openPressed() //SLOT
{
    new KRun( m_path, this );
}


inline void
TagDialog::previousTrack()
{
    storeTags( m_currentTrack );

    if( m_trackIterator.hasPrevious() )
    {
        if ( m_currentTrack == m_trackIterator.peekPrevious() )
            m_trackIterator.previous();

        setCurrentTrack( m_trackIterator.previous() );
    }
    loadTags( m_currentTrack );
    enableItems();
    readTags();
}


inline void
TagDialog::nextTrack()
{
    storeTags( m_currentTrack );

    if( m_trackIterator.hasNext() )
    {
        if ( m_currentTrack == m_trackIterator.peekNext() )
            m_trackIterator.next();

        setCurrentTrack( m_trackIterator.next() );
    }
    loadTags( m_currentTrack );
    enableItems();
    readTags();
}

inline void
TagDialog::perTrack()
{
    m_perTrack = !m_perTrack;
    m_fieldEdited.clear();
    if( m_perTrack )
    {
        // just switched to per track mode
        applyToAllTracks();
        setSingleTrackMode();
        loadTags( m_currentTrack );
        readTags();
    }
    else
    {
        storeTags( m_currentTrack );
        setMultipleTracksMode();
        readMultipleTracks();
    }

    enableItems();
    m_fieldEdited.clear();
}


void
TagDialog::enableItems()
{
    ui->checkBox_perTrack->setChecked( m_perTrack );
    ui->pushButton_previous->setEnabled( m_perTrack && m_trackIterator.hasPrevious() );
    ui->pushButton_next->setEnabled( m_perTrack && m_trackIterator.hasNext() );
}

inline void
TagDialog::composerModified()
{
    m_fieldEdited[ "composer" ] = true;
    checkModified();
}

inline void
TagDialog::artistModified()
{
    m_fieldEdited[ "artist" ] = true;
    checkModified();
}

inline void
TagDialog::albumModified()
{
    m_fieldEdited[ "album" ] = true;
    checkModified();
}

inline void
TagDialog::genreModified()
{
    m_fieldEdited[ "genre" ] = true;
    checkModified();
}

inline void
TagDialog::ratingModified()
{
    m_fieldEdited[ "rating" ] = true;
    checkModified();
}

inline void
TagDialog::yearModified()
{
    m_fieldEdited[ "year" ] = true;
    checkModified();
}

inline void
TagDialog::scoreModified()
{
    m_fieldEdited[ "score" ] = true;
    checkModified();
}

inline void
TagDialog::commentModified()
{
    DEBUG_BLOCK
    m_fieldEdited[ "comment" ] = true;
    checkModified();
}

inline void
TagDialog::discNumberModified()
{
    m_fieldEdited[ "discNumber" ] = true;
    checkModified();
}

inline void
TagDialog::checkModified() //SLOT
{
    ui->pushButton_ok->setEnabled( hasChanged() || m_storedTags.count() > 0 || m_storedScores.count() > 0
                               || m_storedLyrics.count() > 0 || m_storedRatings.count() > 0 || m_newLabels.count() > 0 );
}

void
TagDialog::loadCover()
{
    if( !m_currentTrack->album() )
        return;

    const int s = 100; // Image preview size

    ui->pixmap_cover->setPixmap( m_currentTrack->album()->image(s) );
    QString artist = m_currentTrack->artist() ? m_currentTrack->artist()->name() : QString();
    ui->pixmap_cover->setInformation( artist, m_currentTrack->album()->name() );

    ui->pixmap_cover->setMinimumSize( s, s );
    ui->pixmap_cover->setMaximumSize( s, s );
}

//creates a KDialog and executes the FilenameLayoutDialog. Grabs a filename scheme, extracts tags (via TagGuesser) from filename and fills the appropriate fields on TagDialog.
void
TagDialog::guessFromFilename() //SLOT
{
    //was: setFilenameSchemes()
    KDialog *dialog = new KDialog( this );
    //FilenameLayoutDialog *dialog = new FilenameLayoutDialog(this);
    dialog->setCaption( i18n( "Filename Layout Chooser" ) );
    dialog->setButtons( KDialog::Ok | KDialog::Cancel );
    FilenameLayoutDialog *widget = new FilenameLayoutDialog( dialog );
    dialog->setMainWidget( widget );
    connect( dialog, SIGNAL( accepted() ),
             widget, SLOT( onAccept() ) );

    const int dcode = dialog->exec();
    QString schemeFromDialog; //note to self: see where to put it from an old revision
    debug() << "FilenameLayoutDialog finished.";
    if( dcode == KDialog::Accepted )
        schemeFromDialog = widget->getParsableScheme();
    else
        debug() << "WARNING: Have not received a new scheme from FilenameLayoutDialog";

    debug() << "I have " << schemeFromDialog << " as filename scheme to use.";
    //legacy tagguesserconfigdialog code follows, needed to make everything hackishly work
    //probably not the best solution
    QStringList schemes;    //now, this is really bad: the old TagGuesser code requires a QStringList of schemes to work on, and by default uses the first one.
                            //It was done like this because the user could pick from a list of default and previously used schemes. I'm just adding my scheme as the first in the list.
                            //So there's a bunch of rotting unused code but I suggest leaving it like this for now so we can rollback to the old dialog if I don't manage to fix the
                            //new one before 2.0.
    schemes += schemeFromDialog;


    if( schemeFromDialog.isEmpty() )
    {
        //FIXME: remove this before release

        debug()<<"No filename scheme to extract tags from. Please choose a filename scheme to describe the layout of the filename(s) to extract the tags.";
    }
    else
    {
        TagGuesser::setSchemeStrings( schemes );
        debug() << "Sent scheme to TagGuesser, let's see what he does with it...";

    //here starts the old guessFromFilename() code
        int cur = 0;

        TagGuesser guesser( m_currentTrack->playableUrl().path(), widget );
        if( !guesser.title().isNull() )
            ui->kLineEdit_title->setText( guesser.title() );

        if( !guesser.artist().isNull() )
        {
            cur = ui->kComboBox_artist->currentIndex();
            ui->kComboBox_artist->setItemText( cur, guesser.artist() );
        }

        if( !guesser.album().isNull() )
        {
            cur = ui->kComboBox_album->currentIndex();
            ui->kComboBox_album->setItemText( cur, guesser.album() );
        }

        if( !guesser.track().isNull() )
            ui->qSpinBox_track->setValue( guesser.track().toInt() );
        if( !guesser.comment().isNull() )
            ui->qPlainTextEdit_comment->setPlainText( guesser.comment() );
        if( !guesser.year().isNull() )
            ui->qSpinBox_year->setValue( guesser.year().toInt() );

        if( !guesser.composer().isNull() )
        {
            cur = ui->kComboBox_composer->currentIndex();
            ui->kComboBox_composer->setItemText( cur, guesser.composer() );
        }

        if( !guesser.genre().isNull() )
        {
            cur = ui->kComboBox_genre->currentIndex();
            ui->kComboBox_genre->setItemText( cur, guesser.genre() );
        }
    }
    delete widget;
    delete dialog;
}

////////////////////////////////////////////////////////////////////////////////
// PRIVATE
////////////////////////////////////////////////////////////////////////////////

void TagDialog::init()
{
    DEBUG_BLOCK
    // delete itself when closing
    setAttribute( Qt::WA_DeleteOnClose );
    setButtons( KDialog::None );

    KConfigGroup config = Amarok::config( "TagDialog" );

    ui->kTabWidget->addTab( ui->summaryTab   , i18n( "Summary" ) );
    ui->kTabWidget->addTab( ui->tagsTab      , i18n( "Tags" ) );
    ui->kTabWidget->addTab( ui->lyricsTab    , i18n( "Lyrics" ) );
    ui->kTabWidget->addTab( ui->statisticsTab, i18n( "Statistics" ) );

    const int labelsIndex = ui->kTabWidget->addTab( ui->labelsTab    , i18n( "Labels" ) );
    ui->labelsTab->setEnabled( false );
    ui->kTabWidget->removeTab( labelsIndex ); //Labels are not yet implemented, therefore disable the tab for now

    ui->kTabWidget->setCurrentIndex( config.readEntry( "CurrentTab", 0 ) );

    ui->kComboBox_artist->completionObject()->setIgnoreCase( true );
    ui->kComboBox_artist->setCompletionMode( KGlobalSettings::CompletionPopup );

    ui->kComboBox_album->completionObject()->setIgnoreCase( true );
    ui->kComboBox_album->setCompletionMode( KGlobalSettings::CompletionPopup );

    ui->kComboBox_composer->completionObject()->setIgnoreCase( true );
    ui->kComboBox_composer->setCompletionMode( KGlobalSettings::CompletionPopup );

    ui->kComboBox_genre->completionObject()->setIgnoreCase( true );
    ui->kComboBox_genre->setCompletionMode( KGlobalSettings::CompletionPopup );

//     const QStringList labels = CollectionDB::instance()->labelList();
    const QStringList labels;
    //TODO: figure out a way to add auto-completion support to kTestEdit_selectedLabels

    m_labelCloud = new KHTMLPart( ui->labels_favouriteLabelsFrame );
    QSizePolicy policy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_labelCloud->view()->setSizePolicy(policy);

    new QVBoxLayout( ui->labels_favouriteLabelsFrame );
    //labels_favouriteLabelsFrame->layout()->addWidget( m_labelCloud->view() );
    //const QStringList favoriteLabels = CollectionDB::instance()->favoriteLabels();
    //QString html = generateHTML( favoriteLabels );
    //m_labelCloud->write( html );
    //connect( m_labelCloud->browserExtension(), SIGNAL( openUrlRequest( const KUrl &, const KParts::OpenUrlArguments&, const KParts::BrowserArguments& ) ),
    //         this,                             SLOT( openUrlRequest( const KUrl & ) ) );

    // looks better to have a blank label than 0, we can't do this in
    // the UI file due to bug in Designer
    ui->qSpinBox_track->setSpecialValueText( " " );
    ui->qSpinBox_year->setSpecialValueText( " " );
    ui->qSpinBox_score->setSpecialValueText( " " );
    ui->qSpinBox_discNumber->setSpecialValueText( " " );

    // set an icon for the open-in-konqui button
    ui->pushButton_open->setIcon( KIcon( "folder-amarok" ) );

    connect( ui->pushButton_guessTags, SIGNAL(clicked()), SLOT( guessFromFilename() ) );

    if( m_tracks.count() > 1 )
    {   //editing multiple tracks
        m_perTrack = false;
        setMultipleTracksMode();
        readMultipleTracks();
        enableItems();
    }
    else
    {
        m_perTrack = true;
        ui->checkBox_perTrack->hide();
        ui->pushButton_previous->hide();
        ui->pushButton_next->hide();

        loadLyrics( m_currentTrack );
        loadLabels( m_currentTrack );
        loadTags( m_currentTrack );
        readTags();
    }

    // Connects for modification check
    // only set to overwrite-on-save if the text has changed
    connect( ui->kLineEdit_title,     SIGNAL( textChanged( const QString& ) ),     SLOT(checkModified()) );
    connect( ui->kComboBox_composer,  SIGNAL( activated( int ) ),                  SLOT(checkModified()) );
    connect( ui->kComboBox_composer,  SIGNAL( editTextChanged( const QString& ) ), SLOT(composerModified()) );
    connect( ui->kComboBox_artist,    SIGNAL( activated( int ) ),                  SLOT(checkModified()) );
    connect( ui->kComboBox_artist,    SIGNAL( editTextChanged( const QString& ) ), SLOT(artistModified()) );
    connect( ui->kComboBox_album,     SIGNAL( activated( int ) ),                  SLOT(checkModified()) );
    connect( ui->kComboBox_album,     SIGNAL( editTextChanged( const QString& ) ), SLOT(albumModified()) );
    connect( ui->kComboBox_genre,     SIGNAL( activated( int ) ),                  SLOT(checkModified()) );
    connect( ui->kComboBox_genre,     SIGNAL( editTextChanged( const QString& ) ), SLOT(genreModified()) );
    connect( ui->ratingWidget,        SIGNAL( ratingChanged( int ) ),              SLOT(ratingModified()) );
    connect( ui->qSpinBox_track,      SIGNAL( valueChanged( int ) ),               SLOT(checkModified()) );
    connect( ui->qSpinBox_year,       SIGNAL( valueChanged( int ) ),               SLOT(yearModified()) );
    connect( ui->qSpinBox_score,      SIGNAL( valueChanged( int ) ),               SLOT(scoreModified()) );
    connect( ui->qPlainTextEdit_comment,   SIGNAL( textChanged() ),                     SLOT(commentModified()) );
    connect( ui->kTextEdit_lyrics,    SIGNAL( textChanged() ),                     SLOT(checkModified()) );
    connect( ui->kTextEdit_selectedLabels, SIGNAL( textChanged() ),                SLOT(checkModified()) );
    connect( ui->qSpinBox_discNumber, SIGNAL( valueChanged( int ) ),               SLOT(discNumberModified()) );

    connect( ui->pushButton_cancel,   SIGNAL( clicked() ), SLOT( cancelPressed() ) );
    connect( ui->pushButton_ok,       SIGNAL( clicked() ), SLOT( accept() ) );
    connect( ui->pushButton_open,     SIGNAL( clicked() ), SLOT( openPressed() ) );
    connect( ui->pushButton_previous, SIGNAL( clicked() ), SLOT( previousTrack() ) );
    connect( ui->pushButton_next,     SIGNAL( clicked() ), SLOT( nextTrack() ) );
    connect( ui->checkBox_perTrack,   SIGNAL( clicked() ), SLOT( perTrack() ) );

    ui->pixmap_cover->setContextMenuPolicy( Qt::CustomContextMenu );
    connect( ui->pixmap_cover, SIGNAL( customContextMenuRequested(const QPoint &) ), SLOT( showCoverMenu(const QPoint &) ) );
}

void
TagDialog::startDataQuery()
{
    Amarok::Collection *coll = CollectionManager::instance()->primaryCollection();
    if( !coll )
        return;

    QueryMaker *artist = coll->queryMaker()->setQueryType( QueryMaker::Artist );
    QueryMaker *album = coll->queryMaker()->setQueryType( QueryMaker::Album );
    QueryMaker *composer = coll->queryMaker()->setQueryType( QueryMaker::Composer );
    QueryMaker *genre = coll->queryMaker()->setQueryType( QueryMaker::Genre );
    QList<QueryMaker*> queries;
    queries << artist << album << composer << genre;

    //MetaQueryMaker will run multiple different queries just fine as long as we do not use it
    //to set the query type. Configuring the queries is ok though

    m_dataQueryMaker = new MetaQueryMaker( queries );
    connect( m_dataQueryMaker, SIGNAL( queryDone() ), SLOT( dataQueryDone() ) );
    connect( m_dataQueryMaker, SIGNAL( newResultReady( QString, Meta::ArtistList ) ), SLOT( resultReady( QString, Meta::ArtistList ) ), Qt::QueuedConnection );
    connect( m_dataQueryMaker, SIGNAL( newResultReady( QString, Meta::AlbumList ) ), SLOT( resultReady( QString, Meta::AlbumList ) ), Qt::QueuedConnection );
    connect( m_dataQueryMaker, SIGNAL( newResultReady( QString, Meta::ComposerList ) ), SLOT( resultReady( QString, Meta::ComposerList ) ), Qt::QueuedConnection );
    connect( m_dataQueryMaker, SIGNAL( newResultReady( QString, Meta::GenreList ) ), SLOT( resultReady( QString, Meta::GenreList ) ), Qt::QueuedConnection );
    m_dataQueryMaker->run();
}


inline const QString TagDialog::unknownSafe( QString s )
{
    return ( s.isNull() || s.isEmpty() || s == "?" || s == "-" )
           ? i18nc( "The value for this tag is not known", "Unknown" )
           : s;
}

void
TagDialog::metadataChanged( Meta::AlbumPtr album )
{
    DEBUG_BLOCK

    if( !m_currentTrack || !m_currentTrack->album() )
        return;

    debug() << "setting album cover?";

    if( album == m_currentTrack->album() )
    {
        debug() << "yep";
        loadCover();
    }
}

void
TagDialog::showCoverMenu( const QPoint &pos )
{
    Meta::AlbumPtr album = m_currentTrack->album();
    if( !album )
        return; // TODO: warning or something?

    QAction *displayCoverAction = new DisplayCoverAction( this, album );
    QAction *unsetCoverAction   = new UnsetCoverAction( this, album );

    if( !album->hasImage() )
    {
        displayCoverAction->setEnabled( false );
        unsetCoverAction->setEnabled( false );
    }

    KMenu *menu = new KMenu( this );
    menu->addAction( displayCoverAction );
    menu->addAction( new FetchCoverAction( this, album ) );
    menu->addAction( new SetCustomCoverAction( this, album ) );
    menu->addAction( unsetCoverAction );

    menu->exec( ui->pixmap_cover->mapToGlobal(pos) );
}

const QStringList TagDialog::statisticsData()
{
    AMAROK_NOTIMPLEMENTED

    QStringList data, values;
    //TODO: port
    /*
    const uint artist_id = CollectionDB::instance()->artistID( m_bundle.artist() );
    const uint album_id  = CollectionDB::instance()->albumID ( m_bundle.album() );

    QueryBuilder qb;

    if ( !m_bundle.artist().isEmpty() ) {
        // tracks by this artist
        qb.clear();
        qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabSong, QueryBuilder::valTitle );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, QString::number( artist_id ) );
        values = qb.run();
        data += i18n( "Tracks by this Artist" );
        data += values[0];


        // albums by this artist
        qb.clear();
        qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabAlbum, QueryBuilder::valID );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, QString::number( artist_id ) );
        qb.groupBy( QueryBuilder::tabSong, QueryBuilder::valAlbumID );
        qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
        qb.setOptions( QueryBuilder::optNoCompilations );
        values = qb.run();
        data += i18n( "Albums by this Artist" );
        data += QString::number( values.count() );


        // Favorite track by this artist
        qb.clear();
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
        qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, QString::number( artist_id ) );
        qb.sortByFavorite();
        qb.setLimit( 0, 1 );
        values = qb.run();
        data += i18n( "Favorite by this Artist" );
        data += values.isEmpty() ? QString() : values[0];

        if ( !m_bundle.album().isEmpty() ) {
            // Favorite track on this album
            qb.clear();
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
            qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
            qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, QString::number( album_id ) );
            qb.sortByFavorite();
            qb.setLimit( 0, 1 );
            values = qb.run();
            data += i18n( "Favorite on this Album" );
            data += values.isEmpty() ? QString() : values[0];
        }

        // Related Artists
        const QString sArtists = CollectionDB::instance()->similarArtists( m_bundle.artist(), 4 ).join(", ");
        if ( !sArtists.isEmpty() ) {
            data += i18n( "Related Artists" );
            data += sArtists;
        }
    }*/
    return data;
}

void TagDialog::readTags()
{
    DEBUG_BLOCK

    const bool local = m_currentTrack->playableUrl().isLocalFile();

    setWindowTitle( KDialog::makeStandardCaption( i18n("Track Details: %1 by %2",
                    m_currentTrack->name(),  m_currentTrack->artist() ? m_currentTrack->artist()->name() : QString() ) ) );

    QString niceTitle;

    const QFontMetrics fnt =  ui->trackArtistAlbumLabel->fontMetrics();
    const int len = ui->trackArtistAlbumLabel->width();
    QString curTrackAlbName;
    QString curArtistName;

    QString curTrackName = fnt.elidedText( Qt::escape( m_currentTrack->name() ), Qt::ElideRight, len );
    QString curTrackPretName = fnt.elidedText( Qt::escape( m_currentTrack->prettyName() ), Qt::ElideRight, len );

    if( m_currentTrack->album() )
        curTrackAlbName = fnt.elidedText( Qt::escape( m_currentTrack->album()->name() ), Qt::ElideRight, len );
    if( m_currentTrack->artist() )
        curArtistName = fnt.elidedText( Qt::escape( m_currentTrack->artist()->name() ), Qt::ElideRight, len );


    if( m_currentTrack->album() && m_currentTrack->album()->name().isEmpty() )
    {
        if( !m_currentTrack->name().isEmpty() )
        {
            if( !m_currentTrack->artist()->name().isEmpty() )
                niceTitle = i18n( "<b>%1</b> by <b>%2</b>", curTrackName,  curArtistName );
            else
                niceTitle = QString( "<b>%1</b>").arg( curTrackName );
        }
        else
            niceTitle = curTrackPretName;
    }
    else if( m_currentTrack->album() )
        niceTitle = i18n( "<b>%1</b> by <b>%2</b> on <b>%3</b>" , curTrackName, curArtistName, curTrackAlbName );
    else if( m_currentTrack->artist() )
        niceTitle = i18n( "<b>%1</b> by <b>%2</b>" , curTrackName, curArtistName );
    else
        niceTitle = i18n( "<b>%1</b>" , curTrackName );

    ui->trackArtistAlbumLabel->setText( niceTitle );
    ui->trackArtistAlbumLabel2->setText( niceTitle );

    ui->kLineEdit_title->setText( m_currentData.value( Meta::Field::TITLE ).toString() );
    if( m_currentData.contains( Meta::Field::ARTIST ) )
        selectOrInsertText( m_currentData.value( Meta::Field::ARTIST ).toString(), ui->kComboBox_artist );
    else
        selectOrInsertText( QString(), ui->kComboBox_artist );
    if( m_currentData.contains( Meta::Field::ALBUM )  )
        selectOrInsertText( m_currentData.value( Meta::Field::ALBUM ).toString(), ui->kComboBox_album );
    else
        selectOrInsertText( QString(), ui->kComboBox_album );
    if( m_currentData.contains( Meta::Field::GENRE ) )
        selectOrInsertText( m_currentData.value( Meta::Field::GENRE ).toString(), ui->kComboBox_genre );
    else
        selectOrInsertText( QString(), ui->kComboBox_genre );
    if( m_currentData.contains( Meta::Field::COMPOSER ) )
        selectOrInsertText( m_currentData.value( Meta::Field::COMPOSER ).toString(), ui->kComboBox_composer );
    else
        selectOrInsertText( QString(), ui->kComboBox_genre );
    ui->ratingWidget->setRating( m_currentData.value( Meta::Field::RATING ).toInt() );
    ui->ratingWidget->setMaxRating( 10 );
    ui->qSpinBox_track->setValue( m_currentData.value( Meta::Field::TRACKNUMBER ).toInt() );
    if( m_currentData.contains( Meta::Field::YEAR ) )
        ui->qSpinBox_year->setValue( m_currentData.value( Meta::Field::YEAR ).toInt() );
    else
        ui->qSpinBox_year->setValue( 0 );
    ui->qSpinBox_score->setValue( m_currentData.value( Meta::Field::SCORE ).toInt() );
    ui->qSpinBox_discNumber->setValue( m_currentData.value( Meta::Field::DISCNUMBER ).toInt() );
    ui->qPlainTextEdit_comment->setPlainText( Qt::escape( m_currentData.value( Meta::Field::COMMENT ).toString() ) );

    QString summaryText, statisticsText;
    const QString body2cols = "<tr><td><nobr>%1</nobr></td><td><b>%2</b></td></tr>";
    const QString body1col = "<tr><td colspan=2>%1</td></td></tr>";
    const QString emptyLine = "<tr><td colspan=2></td></tr>";

    summaryText = "<table width=100%><tr><td width=50%><table>";
    summaryText += body2cols.arg( i18n("Length:"), unknownSafe( Meta::secToPrettyTime( m_currentTrack->length() ) ) );
    summaryText += body2cols.arg( i18n("Bit rate:"), unknownSafe( Meta::prettyBitrate( m_currentTrack->bitrate() ) ) );
    summaryText += body2cols.arg( i18n("Sample rate:"), unknownSafe( QString::number( m_currentTrack->sampleRate() ) ) );
    summaryText += body2cols.arg( i18n("Size:"), unknownSafe( Meta::prettyFilesize( m_currentTrack->filesize() ) ) );
    summaryText += body2cols.arg( i18n("Format:"), unknownSafe( m_currentTrack->type() ) );

    summaryText += "</table></td><td width=50%><table>";
    summaryText += body2cols.arg( i18n("Score:"), QString::number( static_cast<int>( m_currentTrack->score() ) ) );
    // TODO: this should say something pretty like "3 stars", but that can't happen until after strings are unfrozen
    summaryText += body2cols.arg( i18n("Rating:"), QString::number( static_cast<double>(m_currentTrack->rating())/2.0) );

    summaryText += body2cols.arg( i18n("Play Count:"), QString::number( m_currentTrack->playCount() ) );
    QDate firstPlayed = QDateTime::fromTime_t( m_currentTrack->firstPlayed() ).date();
    QDate lastPlayed = QDateTime::fromTime_t( m_currentTrack->lastPlayed() ).date();
    summaryText += body2cols.arg( i18n("First Played:"),
                   m_currentTrack->playCount() ? KGlobal::locale()->formatDate( firstPlayed, KLocale::ShortDate ) : i18nc( "When this track was first played", "Never") );
    summaryText += body2cols.arg( i18nc("a single item (singular)", "Last Played:"),
                   m_currentTrack->playCount() ? KGlobal::locale()->formatDate( lastPlayed, KLocale::ShortDate ) : i18nc( "When this track was last played", "Never") );
    summaryText += body2cols.arg( i18n("Collection:"),
                                  m_currentTrack->inCollection() ? m_currentTrack->collection()->prettyName() : i18nc( "The collection this track is part of", "None") );

    summaryText += "</table></td></tr></table>";
    ui->summaryLabel->setText( summaryText );

    statisticsText = "<table>";

    QStringList sData = statisticsData();
    for ( int i = 0; i < sData.count(); i+=2 )
    {
        statisticsText += body2cols.arg( sData[i], sData[i+1] );
    }

    statisticsText += "</table>";

    ui->statisticsLabel->setText( statisticsText );
    ui->kLineEdit_location->setText( m_currentTrack->prettyUrl() );

    //lyrics
    // check if the lyrics data contains "<html" (note the missing closing bracket,
    // this enables XHTML lyrics to be recognized) - otherwise the data is plaintext
    if( m_lyrics.contains( "<html" , Qt::CaseInsensitive ) )
        ui->kTextEdit_lyrics->setHtml( m_lyrics );
    else
         ui->kTextEdit_lyrics->setPlainText( m_lyrics );

    loadCover();

    // enable only for editable files
#define enableOrDisable( X ) \
    ui->X->setEnabled( editable ); \
    qobject_cast<KLineEdit*>(ui->X->lineEdit())->setClearButtonShown( editable )

    const bool editable = m_currentTrack->hasCapabilityInterface( Meta::Capability::Editable );
    ui->kLineEdit_title->setEnabled( editable );
    ui->kLineEdit_title->setClearButtonShown( editable );

    enableOrDisable( kComboBox_artist );
    enableOrDisable( kComboBox_composer );
    enableOrDisable( kComboBox_album );
    enableOrDisable( kComboBox_genre );
#undef enableOrDisable
    ui->qSpinBox_track->setEnabled( editable );
    ui->qSpinBox_discNumber->setEnabled( editable );
    ui->qSpinBox_year->setEnabled( editable );
    ui->qPlainTextEdit_comment->setEnabled( editable );
    ui->kTextEdit_selectedLabels->setEnabled( editable );
    m_labelCloud->view()->setEnabled( editable );
    ui->ratingWidget->setEnabled( true );
    ui->qSpinBox_score->setEnabled( true );
    ui->pushButton_guessTags->setEnabled( editable );

    if( local )
        ui->pushButton_guessTags->show();
    else
       ui->pushButton_guessTags->hide();

    // If it's a local file, write the directory to m_path, else disable the "open in konqui" button
    if ( local )
        m_path = m_currentTrack->playableUrl().directory();
    else
        ui->pushButton_open->setEnabled( false );

    ui->pushButton_ok->setEnabled( m_storedTags.count() > 0 || m_storedScores.count() > 0
                              || m_storedLyrics.count() > 0 || m_storedRatings.count() > 0
                              || m_newLabels.count() > 0 );

    //PORT 2.0
//     if( m_playlistItem ) {
//         ui->pushButton_previous->setEnabled( m_playlistItem->itemAbove() );
//         ui->pushButton_next->setEnabled( m_playlistItem->itemBelow() );
//     }
}


void
TagDialog::setMultipleTracksMode()
{
    ui->kTabWidget->setTabEnabled( ui->kTabWidget->indexOf(ui->summaryTab), false );
    ui->kTabWidget->setTabEnabled( ui->kTabWidget->indexOf(ui->lyricsTab), false );

    ui->kComboBox_artist->setItemText( ui->kComboBox_artist->currentIndex(), "" );
    ui->kComboBox_album->setItemText( ui->kComboBox_album->currentIndex(), "" );
    ui->kComboBox_genre->setItemText( ui->kComboBox_genre->currentIndex(), "" );
    ui->kComboBox_composer->setItemText( ui->kComboBox_composer->currentIndex(), "" );
    ui->kLineEdit_title->setText( "" );
    ui->qPlainTextEdit_comment->setPlainText( "" );
    ui->qSpinBox_track->setValue( ui->qSpinBox_track->minimum() );
    ui->qSpinBox_discNumber->setValue( ui->qSpinBox_discNumber->minimum() );
    ui->qSpinBox_year->setValue( ui->qSpinBox_year->minimum() );

    ui->qSpinBox_score->setValue( ui->qSpinBox_score->minimum() );
    ui->ratingWidget->setRating( 0 );

    ui->kLineEdit_title->setEnabled( false );
    ui->qSpinBox_track->setEnabled( false );

    ui->pushButton_guessTags->hide();

    ui->locationLabel->hide();
    ui->kLineEdit_location->hide();
    ui->pushButton_open->hide();
    ui->pixmap_cover->hide();
}

void
TagDialog::setSingleTrackMode()
{
    ui->kTabWidget->setTabEnabled( ui->kTabWidget->indexOf(ui->summaryTab), true );
    ui->kTabWidget->setTabEnabled( ui->kTabWidget->indexOf(ui->lyricsTab), true );

    ui->kLineEdit_title->setEnabled( true );
    ui->qSpinBox_track->setEnabled( true );

    ui->pushButton_guessTags->show();

    ui->locationLabel->show();
    ui->kLineEdit_location->show();
    ui->pushButton_open->show();
    ui->pixmap_cover->show();
}


void
TagDialog::readMultipleTracks()
{
    setWindowTitle( KDialog::makeStandardCaption( i18ncp( "The amount of tracks being edited", "1 Track", "Information for %1 Tracks", m_tracks.count() ) ) );

    //Check which fields are the same for all selected tracks
    QListIterator< Meta::TrackPtr > it( m_tracks );

    m_currentData = QVariantMap();

    QVariantMap first = dataForTrack( it.peekNext() );

    bool artist=true, album=true, genre=true, comment=true, year=true,
         score=true, rating=true, composer=true, discNumber=true;
    int songCount=0, ratingCount=0, ratingSum=0, scoreCount=0;
    double scoreSum = 0.f;
    while( it.hasNext() )
    {
        QVariantMap data = dataForTrack( it.next() );
        songCount++;
        if ( data.value( Meta::Field::RATING ).toInt() )
        {
            ratingCount++;
            ratingSum += data.value( Meta::Field::RATING ).toInt();
        }
        if ( data.value( Meta::Field::SCORE ).toDouble() > 0.f )
        {
            scoreCount++;
            scoreSum += data.value( Meta::Field::SCORE ).toDouble();
        }

        if( !it.peekPrevious()->playableUrl().isLocalFile() )
        {
            // If we have a non local file, don't even lose more time comparing
            artist = album = genre = comment = year = false;
            score  = rating = composer = discNumber = false;
            continue;
        }
        if ( artist && data.value( Meta::Field::ARTIST ).toString() != first.value( Meta::Field::ARTIST ).toString() )
            artist = false;
        if ( album && data.value( Meta::Field::ALBUM ).toString() != first.value( Meta::Field::ALBUM ).toString() )
            album = false;
        if ( genre && data.value( Meta::Field::GENRE ).toString() != first.value( Meta::Field::GENRE ).toString() )
            genre = false;
        if ( comment && data.value( Meta::Field::COMMENT ).toString() != first.value( Meta::Field::COMMENT ).toString() )
            comment = false;
        if ( year && data.value( Meta::Field::YEAR ).toInt() != first.value( Meta::Field::YEAR ).toInt() )
            year = false;
        if ( composer && data.value( Meta::Field::COMPOSER ).toString() != first.value( Meta::Field::COMPOSER ).toString() )
            composer = false;
        if ( discNumber && data.value( Meta::Field::DISCNUMBER ).toInt() != first.value( Meta::Field::DISCNUMBER ).toInt() )
            discNumber = false;
        //score is double internally, but we only show ints in the tab
        if ( score && data.value( Meta::Field::SCORE ).toInt() != first.value( Meta::Field::SCORE ).toInt() )
            score = false;
        if ( rating && data.value( Meta::Field::RATING ).toInt() != first.value( Meta::Field::RATING ).toInt() )
            rating = false;
    }
    // Set them in the dialog and in the track ( so we don't break hasChanged() )
    int cur_item;
    if( artist )
    {
        cur_item = ui->kComboBox_artist->currentIndex();
        m_currentData.insert( Meta::Field::ARTIST, first.value( Meta::Field::ARTIST ) );
        ui->kComboBox_artist->completionObject()->insertItems( m_artists );
        selectOrInsertText( first.value( Meta::Field::ARTIST ).toString(), ui->kComboBox_artist );
    }
    if( album )
    {
        cur_item = ui->kComboBox_album->currentIndex();
        m_currentData.insert( Meta::Field::ALBUM, first.value( Meta::Field::ALBUM ) );
        ui->kComboBox_album->completionObject()->insertItems( m_albums );
        selectOrInsertText( first.value( Meta::Field::ALBUM ).toString(), ui->kComboBox_album );
    }
    if( genre )
    {
        cur_item = ui->kComboBox_genre->currentIndex();
        m_currentData.insert( Meta::Field::GENRE, first.value( Meta::Field::GENRE ) );
        ui->kComboBox_genre->completionObject()->insertItems( m_genres );
        selectOrInsertText( first.value( Meta::Field::GENRE ).toString(), ui->kComboBox_genre );
    }
    if( comment )
    {
        m_currentData.insert( Meta::Field::COMMENT, first.value( Meta::Field::COMMENT ) );
        ui->qPlainTextEdit_comment->setPlainText( first.value( Meta::Field::COMMENT ).toString() );
    }
    if( composer )
    {
        cur_item = ui->kComboBox_composer->currentIndex();
        m_currentData.insert( Meta::Field::COMPOSER, first.value( Meta::Field::COMPOSER ) );
        ui->kComboBox_composer->completionObject()->insertItems( m_composers );
        selectOrInsertText( first.value( Meta::Field::COMPOSER ).toString(), ui->kComboBox_composer );
    }
    if( year )
    {
        m_currentData.insert( Meta::Field::YEAR, first.value( Meta::Field::YEAR ) );
        ui->qSpinBox_year->setValue( first.value( Meta::Field::YEAR ).toInt() );
    }
    if( discNumber )
    {
        m_currentData.insert( Meta::Field::DISCNUMBER, first.value( Meta::Field::DISCNUMBER ) );
        ui->qSpinBox_discNumber->setValue( first.value( Meta::Field::DISCNUMBER ).toInt() );
    }
    if( score )
    {
        m_currentData.insert( Meta::Field::SCORE, first.value( Meta::Field::SCORE ) );
        ui->qSpinBox_score->setValue( first.value( Meta::Field::SCORE ).toInt() );
    }
    if( rating )
    {
        m_currentData.insert( Meta::Field::RATING, first.value( Meta::Field::RATING ) );
        ui->ratingWidget->setRating( first.value( Meta::Field::RATING ).toUInt() );
    }

    m_trackIterator.toFront();

    setCurrentTrack( m_tracks.first() );

    ui->trackArtistAlbumLabel2->setText( i18np( "Editing 1 file", "Editing %1 files", songCount ) );

    const QString body = "<tr><td><nobr>%1</nobr></td><td><b>%2</b></td></tr>";
    QString statisticsText = "<table>";

    statisticsText += body.arg( i18n( "Rated Songs:" ) , QString::number( ratingCount )  );
    if( ratingCount )
        statisticsText += body.arg( i18n( "Average Rating:" ) , QString::number( (float)ratingSum / (float)ratingCount/2.0, 'f', 1  ) );

    statisticsText += body.arg( i18n( "Scored Songs:" ) , QString::number( scoreCount )  );
    if( scoreCount )
        statisticsText += body.arg( i18n( "Average Score:" ) , QString::number( scoreSum / scoreCount, 'f', 1 ) );


    statisticsText += "</table>";

    ui->statisticsLabel->setText( statisticsText );

    QStringList commonLabels = getCommonLabels();
    QString text;
    oldForeach ( commonLabels )
    {
        if ( !text.isEmpty() )
            text.append( ", " );
        text.append( *it );
    }
    ui->kTextEdit_selectedLabels->setText( text );
    m_commaSeparatedLabels = text;

    // This will reset a wrongly enabled Ok button
    checkModified();
}

QStringList
TagDialog::getCommonLabels()
{
    AMAROK_NOTIMPLEMENTED
    return QStringList();
}

inline bool
equalString( const QString &a, const QString &b )
{
    return (a.isEmpty() && b.isEmpty()) ? true : a == b;
}

bool
TagDialog::hasChanged()
{
    return changes();
}

int
TagDialog::changes()
{
    int result = TagDialog::NOCHANGE;
    bool modified = false;
    modified |= !equalString( ui->kComboBox_artist->lineEdit()->text(), m_currentData.value( Meta::Field::ARTIST ).toString() );
    modified |= !equalString( ui->kComboBox_album->lineEdit()->text(), m_currentData.value( Meta::Field::ALBUM ).toString() );
    modified |= !equalString( ui->kComboBox_genre->lineEdit()->text(), m_currentData.value( Meta::Field::GENRE ).toString() );
    modified |= ui->qSpinBox_year->value()  != m_currentData.value( Meta::Field::YEAR ).toInt();
    modified |= ui->qSpinBox_discNumber->value()  != m_currentData.value( Meta::Field::DISCNUMBER ).toInt();
    modified |= !equalString( ui->kComboBox_composer->lineEdit()->text(), m_currentData.value( Meta::Field::COMPOSER ).toString() );

    modified |= !equalString( ui->qPlainTextEdit_comment->toPlainText(), m_currentData.value( Meta::Field::COMMENT ).toString() );

    if( !m_tracks.count() || m_perTrack )
    { //ignore these on MultipleTracksMode
        modified |= !equalString( ui->kLineEdit_title->text(), m_currentData.value( Meta::Field::TITLE ).toString() );
        modified |= ui->qSpinBox_track->value() != m_currentData.value( Meta::Field::TRACKNUMBER ).toInt();
    }
    if( modified )
        result |= TagDialog::TAGSCHANGED;

    if( ui->qSpinBox_score->value() != m_currentData.value( Meta::Field::SCORE ).toInt() )
        result |= TagDialog::SCORECHANGED;
    unsigned int currentRating = m_currentData.value( Meta::Field::RATING ).toUInt();
    if( ui->ratingWidget->rating() != currentRating )
        result |= TagDialog::RATINGCHANGED;

    if( !m_tracks.count() || m_perTrack )
    { //ignore these on MultipleTracksMode
        if ( !equalString( ui->kTextEdit_lyrics->toPlainText(), m_lyrics ) )
            result |= TagDialog::LYRICSCHANGED;
    }

    if( !equalString( ui->kTextEdit_selectedLabels->toPlainText(), m_commaSeparatedLabels ) )
        result |= TagDialog::LABELSCHANGED;

    return result;
}

void
TagDialog::storeTags()
{
    DEBUG_BLOCK

    storeTags( m_currentTrack );
}

void
TagDialog::storeTags( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK

    const int result = changes();

    if( result & TagDialog::TAGSCHANGED )
    {
        debug() << "TagDialog::TAGSCHANGED";
        QVariantMap map( m_currentData );

        //do not nedlessly update everything, as theat wrecks havoc with grouping in the playlist....

        if ( ui->kLineEdit_title->text() != track->name() )
            map.insert( Meta::Field::TITLE, ui->kLineEdit_title->text() );
        if ( !track->composer() || ui->kComboBox_composer->currentText() != track->composer()->name() )
            map.insert( Meta::Field::COMPOSER, ui->kComboBox_composer->currentText() );
        if ( !track->artist() || ui->kComboBox_artist->currentText() != track->artist()->name() )
            map.insert( Meta::Field::ARTIST, ui->kComboBox_artist->currentText() );
        if ( !track->album() || ui->kComboBox_album->currentText() != track->album()->name() )
            map.insert( Meta::Field::ALBUM, ui->kComboBox_album->currentText() );
        if ( ui->qPlainTextEdit_comment->toPlainText() != track->comment() )
            map.insert( Meta::Field::COMMENT, ui->qPlainTextEdit_comment->toPlainText() );
        if ( !track->genre() || ui->kComboBox_genre->currentText() != track->genre()->name() )
            map.insert( Meta::Field::GENRE, ui->kComboBox_genre->currentText() );
        if ( ui->qSpinBox_track->value() != track->trackNumber() )
            map.insert( Meta::Field::TRACKNUMBER, ui->qSpinBox_track->value() );
        if ( !track->year() || QString::number( ui->qSpinBox_year->value() ) != track->year()->name() )
            map.insert( Meta::Field::YEAR, ui->qSpinBox_year->value() );
        if ( ui->qSpinBox_discNumber->value() != track->discNumber() )
            map.insert( Meta::Field::DISCNUMBER, ui->qSpinBox_discNumber->value() );

        m_storedTags.remove( track );
        m_storedTags.insert( track, map );
    }
    if( result & TagDialog::SCORECHANGED )
    {
        debug() << "TagDialog::SCORECHANGED";
        m_storedScores.remove( track );
        m_storedScores.insert( track, ui->qSpinBox_score->value() );
    }

    if( result & TagDialog::RATINGCHANGED )
    {
        debug() << "TagDialog::RATINGCHANGED";
        m_storedRatings.remove( track );
        m_storedRatings.insert( track, ui->ratingWidget->rating() );
    }

    if( result & TagDialog::LYRICSCHANGED )
    {
        debug() << "TagDialog::LYRICSCHANGED";

        if ( ui->kTextEdit_lyrics->toHtml().isEmpty() )
        {
            m_storedLyrics.remove( track );
            m_storedLyrics.insert( track, QString() );
        }
        else
        {
            m_storedLyrics.remove( track );
            m_storedLyrics.insert( track, ui->kTextEdit_lyrics->toHtml() );
        }
    }
}

void
TagDialog::storeTags( const Meta::TrackPtr &track, int changes, const QVariantMap &data )
{
    if( changes & TagDialog::TAGSCHANGED )
    {
        m_storedTags.insert( track, data );
    }
    if( changes & TagDialog::SCORECHANGED )
    {
        m_storedScores.insert( track, data.value( Meta::Field::SCORE ).toDouble() );
    }
    if( changes & TagDialog::RATINGCHANGED )
    {
        m_storedRatings.insert( track, data.value( Meta::Field::RATING ).toInt() );
    }
}

void
TagDialog::storeLabels( const Meta::TrackPtr &track, const QStringList &labels )
{
    m_newLabels.remove( track );
    m_newLabels.insert( track, labels );
}


void
TagDialog::loadTags( const Meta::TrackPtr &track )
{
    m_currentData = dataForTrack( track );
    loadLyrics( track );
    loadLabels( track );
}

void
TagDialog::loadLyrics( const Meta::TrackPtr &track )
{
    QString html = lyricsForTrack( track );

    QDomDocument doc;
    if( !html.isEmpty() )
        m_lyrics = html;
    else
        m_lyrics.clear();
}

void
TagDialog::loadLabels( const Meta::TrackPtr &track )
{
    Q_UNUSED( track )
    AMAROK_NOTIMPLEMENTED
}

QVariantMap
TagDialog::dataForTrack( const Meta::TrackPtr &track )
{
    if( m_storedTags.contains( track ) )
        return m_storedTags[ track ];

    return Meta::Field::mapFromTrack( track );
}

double
TagDialog::scoreForTrack( const Meta::TrackPtr &track )
{
    if( m_storedScores.contains( track ) )
        return m_storedScores[ track ];

    return track->score();
}

int
TagDialog::ratingForTrack( const Meta::TrackPtr &track )
{
    if( m_storedRatings.contains( track ) )
        return m_storedRatings[ track ];

    return track->rating();
}

QString
TagDialog::lyricsForTrack( const Meta::TrackPtr &track )
{
    if( m_storedLyrics.contains( track ) )
        return m_storedLyrics[ track ];

    return track->cachedLyrics();
}

QStringList
TagDialog::labelsForTrack( const Meta::TrackPtr &track )
{
    AMAROK_NOTIMPLEMENTED
    if( m_newLabels.contains( track ) )
        return m_newLabels[ track ];
    if( m_originalLabels.contains( track ) )
        return m_originalLabels[ track ];
    //TODO: port 2.0
    //QStringList tmp = CollectionDB::instance()->getLabels( url.path(), CollectionDB::typeUser );
    QStringList tmp;
    m_originalLabels[ track ] = tmp;
    return tmp;
}

void
TagDialog::saveTags()
{
    DEBUG_BLOCK

    if( !m_perTrack )
    {
        applyToAllTracks();
    }
    else
    {
        storeTags();
    }

    foreach( Meta::TrackPtr track, m_tracks )
    {
        if( m_storedScores.contains( track ) )
        {
            track->setScore( m_storedScores[ track ] );
        }
        if( m_storedRatings.contains( track ) )
        {
            track->setRating( m_storedRatings[ track ] );
        }
        if( m_storedLyrics.contains( track ) )
        {
            track->setCachedLyrics( m_storedLyrics[ track ] );
            emit lyricsChanged( track->uidUrl() );
        }

        Meta::EditCapability *ec = track->create<Meta::EditCapability>();
        if( !ec )
        {
            debug() << "Track does not have Meta::EditCapability. Aborting loop.";
            continue;
        }
        if( !ec->isEditable() )
        {
            debug() << "Track not editable. Aborting loop.";
            The::statusBar()->shortMessage( i18n( "Writing to file failed. Please check permissions and available disc space." ) );
            continue;
        }

        QVariantMap data = m_storedTags[ track ];

        //there is really no need to write to the file if only info m_stored in the db has changed

        //the if from hell
        if ( data.contains( Meta::Field::TITLE ) || data.contains( Meta::Field::COMMENT ) ||
             data.contains( Meta::Field::ARTIST ) || data.contains( Meta::Field::ALBUM ) ||
             data.contains( Meta::Field::GENRE ) || data.contains( Meta::Field::COMPOSER ) ||
             data.contains( Meta::Field::YEAR ) || data.contains( Meta::Field::TRACKNUMBER ) ||
             data.contains( Meta::Field::TRACKNUMBER ) || data.contains( Meta::Field::DISCNUMBER ) )
        {

            debug() << "File info changed....";

            ec->beginMetaDataUpdate();
            if( data.contains( Meta::Field::TITLE ) )
                ec->setTitle( data.value( Meta::Field::TITLE ).toString() );
            if( data.contains( Meta::Field::COMMENT ) )
                ec->setComment( data.value( Meta::Field::COMMENT ).toString() );
            if( data.contains( Meta::Field::ARTIST ) )
                ec->setArtist( data.value( Meta::Field::ARTIST ).toString() );
            if( data.contains( Meta::Field::ALBUM ) )
                ec->setAlbum( data.value( Meta::Field::ALBUM ).toString() );
            if( data.contains( Meta::Field::GENRE ) )
                ec->setGenre( data.value( Meta::Field::GENRE ).toString() );
            if( data.contains( Meta::Field::COMPOSER ) )
                ec->setComposer( data.value( Meta::Field::COMPOSER ).toString() );
            if( data.contains( Meta::Field::YEAR ) )
                ec->setYear( data.value( Meta::Field::YEAR ).toString() );
            if( data.contains( Meta::Field::TRACKNUMBER ) )
                ec->setTrackNumber( data.value( Meta::Field::TRACKNUMBER ).toInt() );
            if( data.contains( Meta::Field::DISCNUMBER ) )
                ec->setDiscNumber( data.value( Meta::Field::DISCNUMBER ).toInt() );
            ec->endMetaDataUpdate();
        }
    }

    // build a map, such that at least one track represents a unique collection

    QMap<QString, Meta::TrackPtr> collectionsToUpdateMap;

    foreach( Meta::TrackPtr track, m_tracks )
    {
        Meta::UpdateCapability *uc = track->create<Meta::UpdateCapability>();
        if( !uc )
        {
            continue;
        }

        //if a track is not in a collection, no collection has to be updated either
        if( !track->collection() )
            continue;

        QString collId = track->collection()->collectionId();

        if( !collectionsToUpdateMap.contains( collId ) )
            collectionsToUpdateMap.insert( collId, track );
    }

    // use the representative tracks to send updated signals

    foreach( Meta::TrackPtr track, collectionsToUpdateMap.values() )
    {
        Meta::UpdateCapability *uc = track->create<Meta::UpdateCapability>();

        uc->collectionUpdated();
    }
}

void
TagDialog::applyToAllTracks()
{
    DEBUG_BLOCK
            debug() << m_fieldEdited;

    generateDeltaForLabelList( labelListFromText( ui->kTextEdit_selectedLabels->toPlainText() ) );

    foreach( Meta::TrackPtr track, m_tracks )
    {
        //do not use Meta::Field::updateTrack() here!
        //that function removes metadata from the track if it cannot find a key in the map

        QVariantMap data = dataForTrack( track );

        int changed = 0;
        if( m_fieldEdited.contains( "artist" ) && m_fieldEdited[ "artist" ] )
        {
            data.insert( Meta::Field::ARTIST, ui->kComboBox_artist->currentText() );
            changed |= TagDialog::TAGSCHANGED;
        }

        if( m_fieldEdited.contains( "album" ) && m_fieldEdited[ "album" ] )
        {
            data.insert( Meta::Field::ALBUM, ui->kComboBox_album->currentText() );
            changed |= TagDialog::TAGSCHANGED;
        }

        if( m_fieldEdited.contains( "genre" ) && m_fieldEdited[ "genre" ] )
        {
            data.insert( Meta::Field::GENRE, ui->kComboBox_genre->currentText() );
            changed |= TagDialog::TAGSCHANGED;
        }

        if( m_fieldEdited.contains( "comment" ) && m_fieldEdited[ "comment" ] )
        {
            data.insert( Meta::Field::COMMENT, ui->qPlainTextEdit_comment->toPlainText() );
            changed |= TagDialog::TAGSCHANGED;
        }

        if( m_fieldEdited.contains( "composer" ) && m_fieldEdited[ "composer" ] )
        {
            data.insert( Meta::Field::COMPOSER, ui->kComboBox_composer->currentText() );
            changed |= TagDialog::TAGSCHANGED;
        }

        if( m_fieldEdited.contains( "year" ) && m_fieldEdited[ "year" ] )
        {
            data.insert( Meta::Field::YEAR, ui->qSpinBox_year->value() );
            changed |= TagDialog::TAGSCHANGED;
        }

        if( m_fieldEdited.contains( "discNumber" ) && m_fieldEdited[ "discNumber" ] )
        {
            data.insert( Meta::Field::DISCNUMBER, ui->qSpinBox_discNumber->value() );
            changed |= TagDialog::TAGSCHANGED;
        }

        if( m_fieldEdited.contains( "score" ) && m_fieldEdited[ "score" ] )
        {
            data.insert( Meta::Field::SCORE, ui->qSpinBox_score->value() );
            changed |= TagDialog::SCORECHANGED;
        }

        if( m_fieldEdited.contains( "rating" ) && m_fieldEdited[ "rating" ] )
        {
            data.insert( Meta::Field::RATING, ui->ratingWidget->rating() );
            changed |= TagDialog::RATINGCHANGED;
        }

        storeTags( track, changed, data );

        QStringList tmpLabels = labelsForTrack( track );
        //apply delta
        foreach( const QString &label, m_removedLabels )
        {
            tmpLabels.removeAll( label );
        }
        foreach( const QString &label, m_addedLabels )
        {
            tmpLabels.append( label );
        }
        storeLabels( track, tmpLabels );
    }
}

QStringList
TagDialog::labelListFromText( const QString &text )
{
    const QStringList tmp = text.split(',', QString::SkipEmptyParts);
    //insert each string into a map to remove duplicates
    QMap<QString, int> map;
    oldForeach( tmp )
    {
        QString tmpString = (*it).trimmed();
        if ( !tmpString.isEmpty() )
        {
            map.remove( tmpString );
            map.insert( tmpString, 0 );
        }
    }
    QStringList result;
    QMap<QString, int>::ConstIterator endMap( map.constEnd() );
    for(QMap<QString, int>::ConstIterator it = map.constBegin(); it != endMap; ++it )
    {
        result.append( it.key() );
    }
    return result;
}

void
TagDialog::generateDeltaForLabelList( const QStringList &list )
{
    m_addedLabels.clear();
    m_removedLabels.clear();
    oldForeach( list )
    {
        if ( !m_labels.contains( *it ) )
            m_addedLabels.append( *it );
    }
    oldForeach( m_labels )
    {
        if ( !list.contains( *it ) )
            m_removedLabels.append( *it );
    }
    m_labels = list;
}

QString
TagDialog::generateHTML( const QStringList &labels )
{
    //the first column of each row is the label name, the second the number of assigned songs
    //loop through it to find the highest number of songs, can be removed if somebody figures out a better sql query
    QMap<QString, QPair<QString, int> > mapping;
    QStringList sortedLabels;
    int max = 1;
    oldForeach( labels )
    {
        QString label = *it;
        sortedLabels << label.toLower();
        ++it;
        int value = ( *it ).toInt();
        if ( value > max )
            max = value;
        mapping[label.toLower()] = QPair<QString, int>( label, value );
    }
    sortedLabels.sort();
    QString html = "<html><body>";
    oldForeach( sortedLabels )
    {
        QString key = *it;
        //generate a number in the range 1..10 based on  how much the label is used
        int labelUse = ( mapping[key].second * 10 ) / max;
        if ( labelUse == 0 )
            labelUse = 1;
        html.append( QString( "<span class='label size%1'><a href=\"label:%2\">%3</a></span> " )
                              .arg( QString::number( labelUse ), mapping[key].first, mapping[key].first ) );
    }
    html.append( "</body></html>" );
    debug() << "Dumping HTML for label cloud: " << html;
    return html;
}

void
TagDialog::openUrlRequest(const KUrl &url )         //SLOT
{
    DEBUG_BLOCK
    if ( url.protocol() == "label" )
    {
        QString text = ui->kTextEdit_selectedLabels->toPlainText();
        QStringList currentLabels = labelListFromText( text );
        if ( currentLabels.contains( url.path() ) )
            return;
        if ( !text.isEmpty() )
            text.append( ", " );
        text.append( url.path() );
        ui->kTextEdit_selectedLabels->setText( text );
    }
}

void
TagDialog::selectOrInsertText( const QString &text, QComboBox *comboBox )
{
    int index = comboBox->findText( text );
    if( index == -1 )
    {
        comboBox->insertItem( 0, text );    //insert at the beginning
        comboBox->setCurrentIndex( 0 );
    }
    else
    {
        comboBox->setCurrentIndex( index );
    }
}

#include "TagDialog.moc"

