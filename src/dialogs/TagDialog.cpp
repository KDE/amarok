/******************************************************************************
 * Copyright (C) 2004 Mark Kretschmann <kretschmann@kde.org>                  *
 *           (C) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>               *
 *           (C) 2005-2006 Alexandre Pereira de Oliveira <aleprj@gmail.com>   *
 *           (C) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                   *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#define DEBUG_PREFIX "TagDialog"

#include "TagDialog.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "CollectionManager.h"
#include "covermanager/CoverFetcher.h"
#include "Debug.h"
#include "EditCapability.h"
#include "FilenameLayoutDialog.h"
#include "MainWindow.h"
#include "MetaQueryMaker.h"
#include "MetaUtility.h"
#include "QueryMaker.h"
#include "StatusBar.h"       //for status messages
#include "TagGuesser.h"
#include "trackpickerdialog.h"
#include "ui_tagdialogbase.h"
#include "UpdateCapability.h"

#include <KApplication>
#include <KComboBox>
#include <KCursor>
#include <KGlobal>
#include <KHTMLView>
#include <KIconLoader>
#include <KLineEdit>
#include <KMessageBox>
#include <KNumInput>
#include <KRun>
#include <KTabWidget>
#include <KTextEdit>
#include <KVBox>

#include <QCheckBox>
#include <qdom.h>
#include <QFile>
#include <QLabel>
#include <QLayout>
#include <QMap>
#include <QPair>
#include <QPushButton>
#include <QTimer>
#include <QToolTip>

#include "metadata/tfile_helper.h" //TagLib::File::isWritable

/*class TagDialogWriter : public ThreadManager::Job
{
public:
    TagDialogWriter( const QMap<QString, Meta::TrackPtr> tagsToChange );
    bool doJob();
    void completeJob();
private:
    QList<bool> m_failed;
    QList<Meta::TrackPtr> m_tags;
    bool    m_updateView;
    int     m_successCount;
    int     m_failCount;
    QStringList m_failedURLs;
};*/


TagDialog::TagDialog( const Meta::TrackList &tracks, QWidget *parent )
    : KDialog( parent )
    , m_currentCover()
    , m_tracks( tracks )
    , m_currentTrack( tracks.first() )
    , m_trackIterator( m_tracks )
    , m_queryMaker( 0 )
    , ui( new Ui::TagDialogBase() )
{
    DEBUG_BLOCK

    ui->setupUi( this );
    init();
    startDataQuery();
}

TagDialog::TagDialog( Meta::TrackPtr track, QWidget *parent )
    : KDialog( parent )
    , m_currentCover()
    , m_tracks()
    , m_currentTrack( track )
    , m_trackIterator( m_tracks )   //makes the compiler happy
    , m_queryMaker( 0 )
    , ui( new Ui::TagDialogBase() )
{
    DEBUG_BLOCK

    m_tracks.append( track );
    //we changed the list after creating the iterator, so create a new iterator
    m_trackIterator = QListIterator<Meta::TrackPtr >( m_tracks );
    ui->setupUi( this );
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

    ui->setupUi( this );
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

    // NOTE: temporary crash prevention.  TreeView should _always_ be updated
    // if tags have changed.

    if ( !m_tracks.isEmpty() )
    {
        delete m_labelCloud;
    }
    else
        debug() << "Empty tracklist?  Must mean TreeView has not been updated!";

    delete ui;
}

void
TagDialog::setTab( int id )
{
    ui->kTabWidget->setCurrentIndex( id );
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
        if ( d_track )
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
        m_currentTrack = m_tracks.first();
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
        m_albums << album->name();
    }
}

void
TagDialog::resultReady( const QString &collectionId, const Meta::ArtistList &artists )
{
    Q_UNUSED( collectionId )
    foreach( Meta::ArtistPtr artist, artists )
    {
        m_artists << artist->name();
    }
}

void
TagDialog::resultReady( const QString &collectionId, const Meta::ComposerList &composers )
{
    Q_UNUSED( collectionId )
    foreach( Meta::ComposerPtr composer, composers )
    {
        m_composers << composer->name();
    }
}

void
TagDialog::resultReady( const QString &collectionId, const Meta::GenreList &genres )
{
    Q_UNUSED( collectionId )
    foreach( Meta::GenrePtr genre, genres )
    {
        m_genres << genre->name();
    }
}

void
TagDialog::dataQueryDone()
{
    DEBUG_BLOCK

    m_dataQueryMaker->deleteLater();
    m_dataQueryMaker = 0;
    //we simply clear the completion data of all comboboxes
    //then load the current track again. that's more work than necessary
    //but the performance impact should be negligible

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
}

void
TagDialog::cancelPressed() //SLOT
{
    QApplication::restoreOverrideCursor(); // restore the cursor before closing the dialog
    reject();
}


void
TagDialog::accept() //SLOT
{
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
        m_currentTrack = m_trackIterator.previous();
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
        m_currentTrack = m_trackIterator.next();
    }
    loadTags( m_currentTrack );
    enableItems();
    readTags();
}

inline void
TagDialog::perTrack()
{
    m_perTrack = !m_perTrack;
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
}


void
TagDialog::enableItems()
{
    ui->checkBox_perTrack->setChecked( m_perTrack );
    ui->pushButton_previous->setEnabled( m_perTrack && m_trackIterator.hasPrevious() );
    ui->pushButton_next->setEnabled( m_perTrack && m_trackIterator.hasNext() );
}


inline void
TagDialog::checkModified() //SLOT
{
    ui->pushButton_ok->setEnabled( hasChanged() || storedTags.count() > 0 || storedScores.count() > 0
                               || storedLyrics.count() > 0 || storedRatings.count() > 0 || newLabels.count() > 0 );
}

void
TagDialog::loadCover()
{
    if( !m_currentTrack->album() )
    {
        return;
    }

    ui->pixmap_cover->setPixmap( m_currentTrack->album()->image( AmarokConfig::coverPreviewSize() ) );
    QString artist = m_currentTrack->artist() ? m_currentTrack->artist()->name() : QString();
    ui->pixmap_cover->setInformation( artist, m_currentTrack->album()->name() );

    const int s = AmarokConfig::coverPreviewSize();
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
    
    const int dcode = dialog->exec();
    QString schemeFromDialog = QString();       //note to self: see where to put it from an old revision
    debug() << "FilenameLayoutDialog finished.";
    schemeFromDialog = "";
    if( dcode == KDialog::Accepted )
    {
        schemeFromDialog = widget->getParsableScheme();
    }
    else
    {
        debug() << "WARNING: Have not received a new scheme from FilenameLayoutDialog";
    }
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

        QMessageBox::warning(this, i18n( "No filename scheme to extract tags from" ), i18n( "Please choose a filename scheme to describe the layout of the filename(s) to extract the tags." ));
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
            ui->kTextEdit_comment->setText( guesser.comment() );
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

void
TagDialog::musicbrainzQuery() //SLOT
{
#ifdef HAVE_TUNEPIMP
    kDebug() ;

    m_mbTrack = m_currentTrack->playableUrl();
    KTRMLookup* ktrm = new KTRMLookup( m_mbTrack.path(), true );
    connect( ktrm, SIGNAL( sigResult( KTRMResultList, QString ) ), SLOT( queryDone( KTRMResultList, QString ) ) );
    connect( ui->pushButton_cancel, SIGNAL( clicked() ), ktrm, SLOT( deleteLater() ) );

    ui->pushButton_musicbrainz->setEnabled( false );
    ui->pushButton_musicbrainz->setText( i18n( "Generating audio fingerprint..." ) );
    QApplication::setOverrideCursor( Qt::BusyCursor );
#endif
}

void
TagDialog::queryDone( KTRMResultList results, QString error ) //SLOT
{
#ifdef HAVE_TUNEPIMP

    if ( !error.isEmpty() ) {
        KMessageBox::sorry( this, i18n( "Tunepimp (MusicBrainz tagging library) returned the following error: \"%1\".", error) );
    }
    else {
        if ( !results.isEmpty() )
        {
            TrackPickerDialog* t = new TrackPickerDialog( m_mbTrack.fileName(), results, this );
            t->show();
            connect( t, SIGNAL( finished() ), SLOT( resetMusicbrainz() ) ); // clear m_mbTrack
        }
        else {
            KMessageBox::sorry( this, i18n( "The track was not found in the MusicBrainz database." ) );
            resetMusicbrainz(); // clear m_mbTrack
        }
    }

    QApplication::restoreOverrideCursor();
    ui->pushButton_musicbrainz->setEnabled( true );
    ui->pushButton_musicbrainz->setText( m_buttonMbText );
#else
    Q_UNUSED( results );
    Q_UNUSED( error );
#endif
}

void
TagDialog::fillSelected( KTRMResult selected ) //SLOT
{
#ifdef HAVE_TUNEPIMP
    if ( m_bundle.url() == m_mbTrack ) {
        if ( !selected.title().isEmpty() )    kLineEdit_title->setText( selected.title() );
        if ( !selected.artist().isEmpty() )   ui->kComboBox_artist->setCurrentText( selected.artist() );
        if ( !selected.album().isEmpty() )    ui->kComboBox_album->setCurrentText( selected.album() );
        if ( selected.track() != 0 )          ui->qSpinBox_track->setValue( selected.track() );
        if ( selected.year() != 0 )           ui->qSpinBox_year->setValue( selected.year() );
    } else {
        MetaBundle mb;
        mb.setPath( m_mbTrack.path() );
        if ( !selected.title().isEmpty() )    mb.setTitle( selected.title() );
        if ( !selected.artist().isEmpty() )   mb.setArtist( selected.artist() );
        if ( !selected.album().isEmpty() )    mb.setAlbum( selected.album() );
        if ( selected.track() != 0 )          mb.setTrack( selected.track() );
        if ( selected.year() != 0 )           mb.setYear( selected.year() );

        storedTags.replace( m_mbTrack.path(), mb );
    }
#else
    Q_UNUSED(selected);
#endif
}

void TagDialog::resetMusicbrainz() //SLOT
{
#ifdef HAVE_TUNEPIMP
    m_mbTrack = "";
#endif
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

    // Connects for modification check
    connect( ui->kLineEdit_title,     SIGNAL(textChanged( const QString& )),     SLOT(checkModified()) );
    connect( ui->kComboBox_composer,  SIGNAL(activated( int )),                  SLOT(checkModified()) );
    connect( ui->kComboBox_composer,  SIGNAL(editTextChanged( const QString& )), SLOT(checkModified()) );
    connect( ui->kComboBox_artist,    SIGNAL(activated( int )),                  SLOT(checkModified()) );
    connect( ui->kComboBox_artist,    SIGNAL(editTextChanged( const QString& )), SLOT(checkModified()) );
    connect( ui->kComboBox_album,     SIGNAL(activated( int )),                  SLOT(checkModified()) );
    connect( ui->kComboBox_album,     SIGNAL(editTextChanged( const QString& )), SLOT(checkModified()) );
    connect( ui->kComboBox_genre,     SIGNAL(activated( int )),                  SLOT(checkModified()) );
    connect( ui->kComboBox_genre,     SIGNAL(editTextChanged( const QString& )), SLOT(checkModified()) );
    connect( ui->ratingWidget,        SIGNAL(ratingChanged( int )),              SLOT(checkModified()) );
    connect( ui->qSpinBox_track,      SIGNAL(valueChanged( int )),               SLOT(checkModified()) );
    connect( ui->qSpinBox_year,       SIGNAL(valueChanged( int )),               SLOT(checkModified()) );
    connect( ui->qSpinBox_score,      SIGNAL(valueChanged( int )),               SLOT(checkModified()) );
    connect( ui->kTextEdit_comment,   SIGNAL(textChanged()),                     SLOT(checkModified()) );
    connect( ui->kTextEdit_lyrics,    SIGNAL(textChanged()),                     SLOT(checkModified()) );
    connect( ui->kTextEdit_selectedLabels, SIGNAL(textChanged()),                SLOT(checkModified()) );
    connect( ui->qSpinBox_discNumber, SIGNAL(valueChanged( int )),               SLOT(checkModified()) );

    // Remember original button text
    m_buttonMbText = ui->pushButton_musicbrainz->text();

    connect( ui->pushButton_cancel,   SIGNAL(clicked()), SLOT(cancelPressed()) );
    connect( ui->pushButton_ok,       SIGNAL(clicked()), SLOT(accept()) );
    connect( ui->pushButton_open,     SIGNAL(clicked()), SLOT(openPressed()) );
    connect( ui->pushButton_previous, SIGNAL(clicked()), SLOT(previousTrack()) );
    connect( ui->pushButton_next,     SIGNAL(clicked()), SLOT(nextTrack()) );
    connect( ui->checkBox_perTrack,   SIGNAL(clicked()), SLOT(perTrack()) );

    // set an icon for the open-in-konqui button
    ui->pushButton_open->setIcon( KIcon( "folder-amarok" ) );

    //Update cover
    //FIXME: Port 2.0
//     connect( CollectionDB::instance(), SIGNAL( coverFetched( const QString&, const QString& ) ),
//             this, SLOT( loadCover( const QString&, const QString& ) ) );
//     connect( CollectionDB::instance(), SIGNAL( coverChanged( const QString&, const QString& ) ),
//              this, SLOT( loadCover( const QString&, const QString& ) ) );



#ifdef HAVE_TUNEPIMP
    connect( ui->pushButton_musicbrainz, SIGNAL(clicked()), SLOT(musicbrainzQuery()) );
#else
    ui->pushButton_musicbrainz->setToolTip( i18n("Please install MusicBrainz to enable this functionality") );
#endif

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
        readTags();
    }
}

void
TagDialog::startDataQuery()
{
    Collection *coll = CollectionManager::instance()->primaryCollection();
    if( !coll )
    {
        return;
    }
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

    debug() << "before album() stuff";
    QString niceTitle;
    if( m_currentTrack->album() && m_currentTrack->album()->name().isEmpty() )
    {
        if( !m_currentTrack->name().isEmpty() )
        {
            if( !m_currentTrack->artist()->name().isEmpty() )
                niceTitle = i18n( "<b>%1</b> by <b>%2</b>", m_currentTrack->name(),  m_currentTrack->artist()->name() );
            else
                niceTitle = QString( "<b>%1</b>").arg( m_currentTrack->name() );
        }
        else
            niceTitle = m_currentTrack->prettyName();
    }
    else if( m_currentTrack->album() )
    {
        niceTitle = i18n( "<b>%1</b> by <b>%2</b> on <b>%3</b>" ,
            m_currentTrack->name(), m_currentTrack->artist()->name(), m_currentTrack->album()->name() );
    }
    else
        niceTitle = i18n( "<b>%1</b> by <b>%2</b>" , m_currentTrack->name(), m_currentTrack->artist()->name() );

    debug() << "after album() stuff";

    ui->trackArtistAlbumLabel->setText( niceTitle );
    ui->trackArtistAlbumLabel2->setText( niceTitle );

    ui->kLineEdit_title->setText( m_currentTrack->name() );
    selectOrInsertText( m_currentTrack->artist()->name(), ui->kComboBox_artist );
    selectOrInsertText( m_currentTrack->album()->name(), ui->kComboBox_album );
    selectOrInsertText( m_currentTrack->genre()->name(), ui->kComboBox_genre );
    selectOrInsertText( m_currentTrack->composer()->name(), ui->kComboBox_composer );
    ui->ratingWidget->setRating( m_currentTrack->rating() );
    ui->ratingWidget->setMaxRating( 10 );
    ui->qSpinBox_track->setValue( m_currentTrack->trackNumber() );
    ui->qSpinBox_year->setValue( m_currentTrack->year()->name().toInt() );
    ui->qSpinBox_score->setValue( static_cast<int>(m_currentTrack->score()) );
    ui->qSpinBox_discNumber->setValue( m_currentTrack->discNumber() );
    ui->kTextEdit_comment->setText( m_currentTrack->comment() );

    QString summaryText, statisticsText;
    const QString body2cols = "<tr><td><nobr>%1</nobr></td><td><b>%2</b></td></tr>";
    const QString body1col = "<tr><td colspan=2>%1</td></td></tr>";
    const QString emptyLine = "<tr><td colspan=2></td></tr>";

    summaryText = "<table width=100%><tr><td width=50%><table>";
    summaryText += body2cols.arg( i18n("Length:"), unknownSafe( Meta::secToPrettyTime( m_currentTrack->length() ) ) );
    summaryText += body2cols.arg( i18n("Bitrate:"), unknownSafe( Meta::prettyBitrate( m_currentTrack->bitrate() ) ) );
    summaryText += body2cols.arg( i18n("Samplerate:"), unknownSafe( QString::number( m_currentTrack->sampleRate() ) ) );
    summaryText += body2cols.arg( i18n("Size:"), unknownSafe( Meta::prettyFilesize( m_currentTrack->filesize() ) ) );
    summaryText += body2cols.arg( i18n("Format:"), unknownSafe( m_currentTrack->type() ) );

    summaryText += "</table></td><td width=50%><table>";
    summaryText += body2cols.arg( i18n("Score:"), QString::number( static_cast<int>( m_currentTrack->score() ) ) );
    // TODO: this should say something pretty like "3½ stars", but that can't happen until after strings are unfrozen
    summaryText += body2cols.arg( i18n("Rating:"), QString::number( static_cast<double>(m_currentTrack->rating())/2.0) );

    summaryText += body2cols.arg( i18n("Playcount:"), QString::number( m_currentTrack->playCount() ) );
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
    ui->kTextEdit_lyrics->setText( m_lyrics );

    ui->pixmap_cover->setPixmap( m_currentTrack->album()->image( AmarokConfig::coverPreviewSize() ) );
    ui->pixmap_cover->setInformation( m_currentTrack->artist()->name(), m_currentTrack->album()->name() );
    const int s = AmarokConfig::coverPreviewSize();
    ui->pixmap_cover->setMinimumSize( s, s );
    ui->pixmap_cover->setMaximumSize( s, s );


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
    ui->qSpinBox_score->setEnabled( editable );
    ui->kTextEdit_comment->setEnabled( editable );
    ui->kTextEdit_selectedLabels->setEnabled( editable );
    m_labelCloud->view()->setEnabled( editable );

    if( local )
    {
        ui->pushButton_musicbrainz->show();
        ui->pushButton_guessTags->show();
    }
    else
    {
       ui->pushButton_musicbrainz->hide();
       ui->pushButton_guessTags->hide();
    }

    // If it's a local file, write the directory to m_path, else disable the "open in konqui" button
    if ( local )
        m_path = m_currentTrack->playableUrl().directory();
    else
        ui->pushButton_open->setEnabled( false );

    ui->pushButton_ok->setEnabled( storedTags.count() > 0 || storedScores.count() > 0
                              || storedLyrics.count() > 0 || storedRatings.count() > 0
                              || newLabels.count() > 0 );

#ifdef HAVE_TUNEPIMP

    // Don't enable button if a query is in progress already (or if the file isn't local)
    ui->pushButton_musicbrainz->setEnabled( m_bundle.url().isLocalFile() && m_mbTrack.isEmpty() );
#else
    ui->pushButton_musicbrainz->setEnabled( false );
#endif

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
    ui->kTextEdit_comment->setText( "" );
    ui->qSpinBox_track->setValue( ui->qSpinBox_track->minimum() );
    ui->qSpinBox_discNumber->setValue( ui->qSpinBox_discNumber->minimum() );
    ui->qSpinBox_year->setValue( ui->qSpinBox_year->minimum() );

    ui->qSpinBox_score->setValue( ui->qSpinBox_score->minimum() );
    ui->ratingWidget->setRating( 0 );

    ui->kLineEdit_title->setEnabled( false );
    ui->qSpinBox_track->setEnabled( false );

    ui->pushButton_musicbrainz->hide();
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

    ui->pushButton_musicbrainz->show();
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
        selectOrInsertText( first.value( Meta::Field::ARTIST ).toString(), ui->kComboBox_artist );
    }
    if( album )
    {
        cur_item = ui->kComboBox_album->currentIndex();
        m_currentData.insert( Meta::Field::ALBUM, first.value( Meta::Field::ALBUM ) );
        selectOrInsertText( first.value( Meta::Field::ALBUM ).toString(), ui->kComboBox_album );
    }
    if( genre )
    {
        cur_item = ui->kComboBox_genre->currentIndex();
        m_currentData.insert( Meta::Field::GENRE, first.value( Meta::Field::GENRE ) );
        selectOrInsertText( first.value( Meta::Field::GENRE ).toString(), ui->kComboBox_genre );
    }
    if( comment )
    {
        m_currentData.insert( Meta::Field::COMMENT, first.value( Meta::Field::COMMENT ) );
        ui->kTextEdit_comment->setText( first.value( Meta::Field::COMMENT ).toString() );
    }
    if( composer )
    {
        cur_item = ui->kComboBox_composer->currentIndex();
        m_currentData.insert( Meta::Field::COMPOSER, first.value( Meta::Field::COMPOSER ) );
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
    m_currentTrack = m_tracks.first();

    ui->trackArtistAlbumLabel2->setText( i18np( "Editing 1 file", "Editing %1 files", songCount ) );

    const QString body = "<tr><td><nobr>%1:</nobr></td><td><b>%2</b></td></tr>";
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
    /*QMap<QString, int> counterMap;
    const KUrl::List::ConstIterator end = m_urlList.constEnd();
    KUrl::List::ConstIterator iter = m_urlList.constBegin();
    for(; iter != end; ++iter )
    {
        QStringList labels = labelsForURL( *iter );
        oldForeach( labels )
        {
            if ( counterMap.contains( *it ) )
                counterMap[ *it ] = counterMap[ *it ] +1;
            else
                counterMap[ *it ] = 1;
        }
    }
    int n = m_urlList.count();
    QStringList result;
    QMap<QString, int>::ConstIterator counterEnd( counterMap.constEnd() );
    for(QMap<QString, int>::ConstIterator it = counterMap.constBegin(); it != counterEnd; ++it )
    {
        if ( it.value() == n )
            result.append( it.key() );
    }
    return result;*/
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

    modified |= !equalString( ui->kTextEdit_comment->toPlainText(), m_currentData.value( Meta::Field::COMMENT ).toString() );

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
    storeTags( m_currentTrack );
}

void
TagDialog::storeTags( const Meta::TrackPtr &track )
{
    int result = changes();
    if( result & TagDialog::TAGSCHANGED )
    {
        QVariantMap map( m_currentData );

        map.insert( Meta::Field::TITLE, ui->kLineEdit_title->text() );
        map.insert( Meta::Field::COMPOSER, ui->kComboBox_composer->currentText() );
        map.insert( Meta::Field::ARTIST, ui->kComboBox_artist->currentText() );
        map.insert( Meta::Field::ALBUM, ui->kComboBox_album->currentText() );
        map.insert( Meta::Field::COMMENT, ui->kTextEdit_comment->toPlainText() );
        map.insert( Meta::Field::GENRE, ui->kComboBox_genre->currentText() );
        map.insert( Meta::Field::TRACKNUMBER, ui->qSpinBox_track->value() );
        map.insert( Meta::Field::YEAR, ui->qSpinBox_year->value() );
        map.insert( Meta::Field::DISCNUMBER, ui->qSpinBox_discNumber->value() );
        storedTags.remove( track );
        storedTags.insert( track, map );
    }
    if( result & TagDialog::SCORECHANGED )
    {
        storedScores.remove( track );
        storedScores.insert( track, ui->qSpinBox_score->value() );
    }

    if( result & TagDialog::RATINGCHANGED )
    {
        storedRatings.remove( track );
        storedRatings.insert( track, ui->ratingWidget->rating() );
    }

    if( result & TagDialog::LYRICSCHANGED )
    {
        if ( ui->kTextEdit_lyrics->toPlainText().isEmpty() )
        {
            storedLyrics.remove( track );
            storedLyrics.insert( track, QString() );
        }
        else
        {
            QDomDocument doc;
            QDomElement e = doc.createElement( "lyrics" );
            e.setAttribute( "artist", ui->kComboBox_artist->currentText() );
            e.setAttribute( "title", ui->kLineEdit_title->text() );
            QDomText t = doc.createTextNode( ui->kTextEdit_lyrics->toPlainText() );
            e.appendChild( t );
            doc.appendChild( e );
            storedLyrics.remove( track );
            storedLyrics.insert( track, doc.toString() );
        }
    }
    /*if( result & TagDialog::LABELSCHANGED ) {
        generateDeltaForLabelList( labelListFromText( ui->kTextEdit_selectedLabels->toPlainText() ) );
        QStringList tmpLabels;
        if ( newLabels.find( url ) != newLabels.end() )
            tmpLabels = newLabels[ url ];
        else
            tmpLabels = originalLabels[ url ];
        //apply delta
        oldForeach( m_removedLabels )
        {
            tmpLabels.removeAt( tmpLabels.indexOf(*it) );
        }
        oldForeach( m_addedLabels )
        {
            // this just feels dirty...
            if( tmpLabels.indexOf( *it ) == tmpLabels.indexOf( *tmpLabels.end() ) )
                tmpLabels.append( *it );
        }
        newLabels.remove( url );
        newLabels.insert( url, tmpLabels );
    }*/
}

void
TagDialog::storeTags( const Meta::TrackPtr &track, int changes, const QVariantMap &data )
{
    if( changes & TagDialog::TAGSCHANGED )
    {
        storedTags.insert( track, data );
    }
    if( changes & TagDialog::SCORECHANGED )
    {
        storedScores.insert( track, data.value( Meta::Field::SCORE ).toDouble() );
    }
    if( changes & TagDialog::RATINGCHANGED )
    {
        storedRatings.insert( track, data.value( Meta::Field::RATING ).toInt() );
    }
}

void
TagDialog::storeLabels( const Meta::TrackPtr &track, const QStringList &labels )
{
    newLabels.remove( track );
    newLabels.insert( track, labels );
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
    QString xml = lyricsForTrack( track );

    QDomDocument doc;
    if( doc.setContent( xml ) )
        m_lyrics = doc.documentElement().text();
    else
        m_lyrics.clear();
}

void
TagDialog::loadLabels( const Meta::TrackPtr &track )
{
    Q_UNUSED( track )
    AMAROK_NOTIMPLEMENTED
    /*DEBUG_BLOCK
    m_labels = labelsForURL( url );
    originalLabels[ url.path() ] = m_labels;
    QString text;
    oldForeach( m_labels )
    {
        if ( !text.isEmpty() )
            text.append( ", " );
        text.append( *it );
    }
    ui->kTextEdit_selectedLabels->setText( text );
    m_commaSeparatedLabels = text;*/
}

QVariantMap
TagDialog::dataForTrack( const Meta::TrackPtr &track )
{
    if( storedTags.contains( track ) )
        return storedTags[ track ];

    return Meta::Field::mapFromTrack( track.data() );
}

double
TagDialog::scoreForTrack( const Meta::TrackPtr &track )
{
    if( storedScores.contains( track ) )
        return storedScores[ track ];

    return track->score();
}

int
TagDialog::ratingForTrack( const Meta::TrackPtr &track )
{
    if( storedRatings.contains( track ) )
        return storedRatings[ track ];

    return track->rating();
}

QString
TagDialog::lyricsForTrack( const Meta::TrackPtr &track )
{
    if( storedLyrics.contains( track ) )
        return storedLyrics[ track ];

    return track->cachedLyrics();
}

QStringList
TagDialog::labelsForTrack( const Meta::TrackPtr &track )
{
    AMAROK_NOTIMPLEMENTED
    if( newLabels.contains( track ) )
        return newLabels[ track ];
    if( originalLabels.contains( track ) )
        return originalLabels[ track ];
    //TODO: port 2.0
    //QStringList tmp = CollectionDB::instance()->getLabels( url.path(), CollectionDB::typeUser );
    QStringList tmp;
    originalLabels[ track ] = tmp;
    return tmp;
}

void
TagDialog::saveTags()
{
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
        if( storedScores.contains( track ) )
        {
            track->setScore( storedScores[ track ] );
        }
        if( storedRatings.contains( track ) )
        {
            track->setRating( storedRatings[ track ] );
        }
        if( storedLyrics.contains( track ) )
        {
            track->setCachedLyrics( storedLyrics[ track ] );
            emit lyricsChanged( track->uidUrl() );
        }
        Meta::EditCapability *ec = track->as<Meta::EditCapability>();
        if( !ec || !ec->isEditable() )
        {
            continue;
        }
        QVariantMap data = storedTags[ track ];
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

    // build a map, such that at least one track represents a unique collection

    QMap<QString, Meta::TrackPtr> collectionsToUpdateMap;

    foreach( Meta::TrackPtr track, m_tracks )
    {
        Meta::UpdateCapability *uc = track->as<Meta::UpdateCapability>();
        if( !uc )
        {
            continue;
        }

        QString collId = track->collection()->collectionId();
        
        if( !collectionsToUpdateMap.contains( collId ) )
            collectionsToUpdateMap.insert( collId, track );
    }

    // use the representative tracks to send updated signals

    foreach( Meta::TrackPtr track, collectionsToUpdateMap.values() )
    {
        Meta::UpdateCapability *uc = track->as<Meta::UpdateCapability>();

        uc->collectionUpdated();
    }
    
    //TODO: port 2.0
    /*
    QMap<QString, QStringList>::ConstIterator endLabels( newLabels.constEnd() );
    for(QMap<QString, QStringList>::ConstIterator it = newLabels.constBegin(); it != endLabels; ++it ) {
        CollectionDB::instance()->setLabels( it.key(), it.value(),
                CollectionDB::instance()->uniqueIdFromUrl( KUrl( it.key() ) ), CollectionDB::typeUser );
    }
    CollectionDB::instance()->cleanLabels();*/

    //ThreadManager::instance()->queueJob( new TagDialogWriter( storedTags ) );

}

void
TagDialog::applyToAllTracks()
{
    generateDeltaForLabelList( labelListFromText( ui->kTextEdit_selectedLabels->toPlainText() ) );

    foreach( Meta::TrackPtr track, m_tracks )
    {
        /* we have to update the values if they changed, so:
           1) !kLineEdit_field->text().isEmpty() && kLineEdit_field->text() != data.value( Meta::Field::field )
           i.e.: The user wrote something on the field, and it's different from
           what we have in the tag.
           2) !data.value( Meta::Field::field ).isEmpty() && kLineEdit_field->text().isEmpty()
           i.e.: The user was shown some value for the field (it was the same
           for all selected tracks), and he deliberately emptied it.
           TODO: All this mess is because the dialog uses "" to represent what the user
                 doesn't want to change, maybe we can think of something better?
         */

        //do not use Meta::Field::updateTrack() here!
        //that function removes metadata from the track if it cannot find a key in the map

        QVariantMap data = dataForTrack( track );

        int changed = 0;
        QString artist = data.contains( Meta::Field::ARTIST ) ? data.value( Meta::Field::ARTIST ).toString() : QString();
        if( (!ui->kComboBox_artist->currentText().isEmpty() && ui->kComboBox_artist->currentText() != artist )
            || ( ui->kComboBox_artist->currentText().isEmpty() && !artist.isEmpty() ) )
        {
            data.insert( Meta::Field::ARTIST, ui->kComboBox_artist->currentText() );
            changed |= TagDialog::TAGSCHANGED;
        }

        QString album = data.contains( Meta::Field::ALBUM ) ? data.value( Meta::Field::ALBUM ).toString() : QString();
        if( ( !ui->kComboBox_album->currentText().isEmpty() && ui->kComboBox_album->currentText() != album )
            || ( ui->kComboBox_album->currentText().isEmpty() && !album.isEmpty() ) )
        {
            data.insert( Meta::Field::ALBUM, ui->kComboBox_album->currentText() );
            changed |= TagDialog::TAGSCHANGED;
        }

        QString genre = data.contains( Meta::Field::GENRE ) ? data.value( Meta::Field::GENRE ).toString() : QString();
        if( ( !ui->kComboBox_genre->currentText().isEmpty() && ui->kComboBox_genre->currentText() != genre )
            || ( ui->kComboBox_genre->currentText().isEmpty() && !genre.isEmpty() ) )
        {
            data.insert( Meta::Field::GENRE, ui->kComboBox_genre->currentText() );
            changed |= TagDialog::TAGSCHANGED;
        }

        QString comment = data.contains( Meta::Field::COMMENT ) ? data.value( Meta::Field::COMMENT ).toString() : QString();
        if( ( !ui->kTextEdit_comment->toPlainText().isEmpty() && ui->kTextEdit_comment->toPlainText() != comment )
            || ( ui->kTextEdit_comment->toPlainText().isEmpty() && !comment.isEmpty() ) )
        {
            data.insert( Meta::Field::COMMENT, ui->kTextEdit_comment->toPlainText() );
            changed |= TagDialog::TAGSCHANGED;
        }

        QString composer = data.contains( Meta::Field::COMPOSER ) ? data.value( Meta::Field::COMPOSER ).toString() : QString();
        if( ( !ui->kComboBox_composer->currentText().isEmpty() && ui->kComboBox_composer->currentText() != composer )
            || ( ui->kComboBox_composer->currentText().isEmpty() && !composer.isEmpty() ) )
        {
            data.insert( Meta::Field::COMPOSER, ui->kComboBox_composer->currentText() );
            changed |= TagDialog::TAGSCHANGED;
        }

        int year = data.contains( Meta::Field::YEAR ) ? data.value( Meta::Field::YEAR ).toInt() : 0;
        if( ( ui->qSpinBox_year->value() && ui->qSpinBox_year->value() != year )
            || ( ! ui->qSpinBox_year->value() && year ) )
        {
            data.insert( Meta::Field::YEAR, ui->qSpinBox_year->value() );
            changed |= TagDialog::TAGSCHANGED;
        }

        int discnumber = data.contains( Meta::Field::DISCNUMBER ) ? data.value( Meta::Field::DISCNUMBER ).toInt() : 0;
        if( ( ui->qSpinBox_discNumber->value() && ui->qSpinBox_discNumber->value() != discnumber )
            || ( ! ui->qSpinBox_discNumber->value() && discnumber ) )
        {
            data.insert( Meta::Field::DISCNUMBER, ui->qSpinBox_discNumber->value() );
            changed |= TagDialog::TAGSCHANGED;
        }

        int score = data.contains( Meta::Field::SCORE ) ? data.value( Meta::Field::SCORE ).toInt() : 0;
        if( ( ui->qSpinBox_score->value() && ui->qSpinBox_score->value() != score )
            || ( ! ui->qSpinBox_score->value() && score ) )
        {
            data.insert( Meta::Field::SCORE, ui->qSpinBox_score->value() );
            changed |= TagDialog::SCORECHANGED;
        }

        unsigned int rating = data.contains( Meta::Field::RATING ) ? data.value( Meta::Field::RATING ).toUInt() : 0;
        if( ( ui->ratingWidget->rating() && ui->ratingWidget->rating() != rating )
            || ( !ui->ratingWidget->rating() && rating ) )
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

/*bool
TagDialog::writeTag( MetaBundle &mb, bool updateCB )
{
    TagLib::FileName path = TagLibEncodeName( mb.url().path() );
    if ( !TagLib::File::isWritable( path ) ) {
        The::statusBar()->longMessage( i18n(
           "The file %1 is not writable.", mb.url().fileName() ), KDE::StatusBar::Error );
        return false;
    }

    //visual feedback
    QApplication::setOverrideCursor( Qt::WaitCursor );

    bool result = mb.save();
    mb.updateFilesize();

    if( result )
        //update the collection db
        CollectionDB::instance()->updateTags( mb.url().path(), mb, updateCB );

    QApplication::restoreOverrideCursor();

    return result;
}*/

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

/*TagDialogWriter::TagDialogWriter( const QMap<QString, Meta::TrackPtr> tagsToChange )
        : ThreadManager::Job( "TagDialogWriter" ),
          m_successCount ( 0 ),
          m_failCount    ( 0 )
{
    QApplication::setOverrideCursor( Qt::WaitCursor );
    QMap<QString, Meta::TrackPtr>::ConstIterator end = tagsToChange.constEnd();
    for(QMap<QString, Meta::TrackPtr>::ConstIterator it = tagsToChange.constBegin(); it != end; ++it ) {
        Meta::TrackPtr mb = it.value();
        m_tags += mb;
    }
}

bool
TagDialogWriter::doJob()
{
    //FIXME: Port 2.0
//     for( int i = 0, size=m_tags.size(); i<size; ++i ) {
//         TagLib::FileName path = reinterpret_cast<const wchar_t *>( m_tags[i]->playableUrl().path().utf16() );
//         if ( !TagLib::File::isWritable( path ) ) {
//             The::statusBar()->longMessageThreadSafe( i18n(
//                 "The file %1 is not writable.", m_tags[i]->playableUrl().fileName() ), KDE::StatusBar::Error );
//             m_failed += true;
//             continue;
//         }
//
//         //FIXME: Port 2.0
// //         bool result = m_tags[i].save();
// //         m_tags[i].updateFilesize();
//
//         if( result )
//             m_successCount++;
//         else {
//             m_failCount++;
//             m_failedURLs += m_tags[i].prettyUrl();
//         }
//         m_failed += !result;
//     }
     return true;
}

void
TagDialogWriter::completeJob()
{
     for( int i = 0, size=m_tags.size(); i<size; ++i ) {
        if ( !m_failed[i] ) {
            //Port 2.0
//             CollectionDB::instance()->updateTags( m_tags[i].url().path(), m_tags[i], false ); //do not update browsers
// PORT 2.0            Playlist::instance()->updateMetaData( m_tags[i] );
        }
     }
     QApplication::restoreOverrideCursor();
//PORT 2.0     if ( m_successCount )
//PORT 2.0        CollectionView::instance()->databaseChanged();
     if ( m_failCount )
        The::statusBar()->longMessage( i18n(
                        "Sorry, the tags for the following files could not be changed:\n%1", m_failedURLs.join( ";\n" ) ), KDE::StatusBar::Error );
}*/


#include "TagDialog.moc"
