// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>
// (c) 2005-2006 Alexandre Pereira de Oliveira <aleprj@gmail.com>
// See COPYING file for licensing information.

#include "amarok.h"
#include "debug.h"
#include "contextbrowser.h"
#include "collectionbrowser.h"
#include "collectiondb.h"
#include "coverfetcher.h"
#include "metabundle.h"
#include "playlist.h"
#include "playlistitem.h"
#include "statusbar.h"       //for status messages
#include "tagdialog.h"
#include "tagguesser.h"
#include "tagguesserconfigdialog.h"
#include "trackpickerdialog.h"

#include <taglib/tfile.h> //TagLib::File::isWritable

#include <qdom.h>
#include <qfile.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpair.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qcheckbox.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <kcursor.h>
#include <kglobal.h>
#include <khtmlview.h>
#include <kiconloader.h>
#include <ktabwidget.h>
#include <ktextedit.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <krun.h>
#include <kstandarddirs.h>


class TagDialogWriter : public ThreadManager::Job
{
public:
    TagDialogWriter( const QMap<QString, MetaBundle> tagsToChange );
    bool doJob();
    void completeJob();
private:
    QValueList<bool> m_failed;
    QValueList<MetaBundle> m_tags;
    bool    m_updateView;
    int     m_successCount;
    int     m_failCount;
    QStringList m_failedURLs;
};

TagDialog::TagDialog( const KURL& url, QWidget* parent )
    : TagDialogBase( parent )
    , m_bundle( url, true )
    , m_playlistItem( 0 )
    , m_currentCover( 0 )
{
    init();
}


TagDialog::TagDialog( const KURL::List list, QWidget* parent )
    : TagDialogBase( parent )
    , m_bundle()
    , m_playlistItem( 0 )
    , m_urlList( list )
    , m_currentCover( 0 )
{
    init();
}


TagDialog::TagDialog( const MetaBundle& mb, PlaylistItem* item, QWidget* parent )
    : TagDialogBase( parent )
    , m_bundle( mb )
    , m_playlistItem( item )
    , m_currentCover( 0 )
{
    init();
}


TagDialog::~TagDialog()
{
    Amarok::config( "TagDialog" )->writeEntry( "CurrentTab", kTabWidget->currentPageIndex() );
}

void
TagDialog::setTab( int id )
{
    kTabWidget->setCurrentPage( id );
}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
////////////////////////////////////////////////////////////////////////////////

void
TagDialog::cancelPressed() //SLOT
{
    QApplication::restoreOverrideCursor(); // restore the cursor before closing the dialog
    reject();
}


void
TagDialog::accept() //SLOT
{
    pushButton_ok->setEnabled( false ); //visual feedback
    saveTags();

    QDialog::accept();
}


inline void
TagDialog::openPressed() //SLOT
{
    Amarok::invokeBrowser( m_path );
}


inline void
TagDialog::previousTrack()
{
    if( m_playlistItem )
    {
        if( !m_playlistItem->itemAbove() ) return;

        storeTags();

        m_playlistItem = static_cast<PlaylistItem *>( m_playlistItem->itemAbove() );

        loadTags( m_playlistItem->url() );
    }
    else
    {
        storeTags( *m_currentURL );

        if( m_currentURL != m_urlList.begin() )
            --m_currentURL;
        loadTags( *m_currentURL );
        enableItems();
    }
    readTags();
}


inline void
TagDialog::nextTrack()
{
    if( m_playlistItem )
    {
        if( !m_playlistItem->itemBelow() ) return;

        storeTags();

        m_playlistItem = static_cast<PlaylistItem *>( m_playlistItem->itemBelow() );

        loadTags( m_playlistItem->url() );
    }
    else
    {
        storeTags( *m_currentURL );

        KURL::List::iterator next = m_currentURL;
        ++next;
        if( next != m_urlList.end() )
            ++m_currentURL;
        loadTags( *m_currentURL );
        enableItems();
    }
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
        loadTags( *m_currentURL );
        readTags();
    }
    else
    {
        storeTags( *m_currentURL );
        setMultipleTracksMode();
        readMultipleTracks();
    }

    enableItems();
}


void
TagDialog::enableItems()
{
    checkBox_perTrack->setChecked( m_perTrack );
    pushButton_previous->setEnabled( m_perTrack && m_currentURL != m_urlList.begin() );
    KURL::List::ConstIterator next = m_currentURL;
    ++next;
    pushButton_next->setEnabled( m_perTrack && next != m_urlList.end());
    if( m_urlList.count() == 1 )
    {
        checkBox_perTrack->setEnabled( false );
    }
    else
    {
        checkBox_perTrack->setEnabled( true );
    }
}


inline void
TagDialog::checkModified() //SLOT
{
    pushButton_ok->setEnabled( hasChanged() || storedTags.count() > 0 || storedScores.count() > 0
                               || storedLyrics.count() > 0 || storedRatings.count() > 0 || newLabels.count() > 0 );
}

void
TagDialog::loadCover( const QString &artist, const QString &album )
{
    if ( m_bundle.artist() != artist ||  m_bundle.album()!=album )
        return;

    // draw the album cover on the dialog
    QString cover = CollectionDB::instance()->albumImage( m_bundle );

    if( m_currentCover != cover )
    {
        pixmap_cover->setPixmap( QPixmap( cover, "PNG" ) );
        m_currentCover = cover;
    }
    pixmap_cover->setInformation( m_bundle.artist(), m_bundle.album() );
    const int s = AmarokConfig::coverPreviewSize();
    pixmap_cover->setMinimumSize( s, s );
    pixmap_cover->setMaximumSize( s, s );
}


void
TagDialog::setFileNameSchemes() //SLOT
{
    TagGuesserConfigDialog* dialog = new TagGuesserConfigDialog(this, "child");
    dialog->exec();
}


void
TagDialog::guessFromFilename() //SLOT
{
    TagGuesser guesser( m_bundle.url().path() );
    if( !guesser.title().isNull() )
        kLineEdit_title->setText( guesser.title() );
    if( !guesser.artist().isNull() )
        kComboBox_artist->setCurrentText( guesser.artist() );
    if( !guesser.album().isNull() )
        kComboBox_album->setCurrentText( guesser.album() );
    if( !guesser.track().isNull() )
        kIntSpinBox_track->setValue( guesser.track().toInt() );
    if( !guesser.comment().isNull() )
        kTextEdit_comment->setText( guesser.comment() );
    if( !guesser.year().isNull() )
        kIntSpinBox_year->setValue( guesser.year().toInt() );
    if( !guesser.composer().isNull() )
        kComboBox_composer->setCurrentText( guesser.composer() );
    if( !guesser.genre().isNull() )
        kComboBox_genre->setCurrentText( guesser.genre() );
}

void
TagDialog::musicbrainzQuery() //SLOT
{
#if HAVE_TUNEPIMP
    kdDebug() << k_funcinfo << endl;

    m_mbTrack = m_bundle.url();
    KTRMLookup* ktrm = new KTRMLookup( m_mbTrack.path(), true );
    connect( ktrm, SIGNAL( sigResult( KTRMResultList, QString ) ), SLOT( queryDone( KTRMResultList, QString ) ) );
    connect( pushButton_cancel, SIGNAL( clicked() ), ktrm, SLOT( deleteLater() ) );

    pushButton_musicbrainz->setEnabled( false );
    pushButton_musicbrainz->setText( i18n( "Generating audio fingerprint..." ) );
    QApplication::setOverrideCursor( KCursor::workingCursor() );
#endif
}

void
TagDialog::queryDone( KTRMResultList results, QString error ) //SLOT
{
#if HAVE_TUNEPIMP

    if ( !error.isEmpty() ) {
        KMessageBox::sorry( this, i18n( "Tunepimp (MusicBrainz tagging library) returned the following error: \"%1\"." ).arg(error) );
    }
    else {
        if ( !results.isEmpty() )
        {
            TrackPickerDialog* t = new TrackPickerDialog( m_mbTrack.filename(), results, this );
            t->show();
            connect( t, SIGNAL( finished() ), SLOT( resetMusicbrainz() ) ); // clear m_mbTrack
        }
        else {
            KMessageBox::sorry( this, i18n( "The track was not found in the MusicBrainz database." ) );
            resetMusicbrainz(); // clear m_mbTrack
        }
    }

    QApplication::restoreOverrideCursor();
    pushButton_musicbrainz->setEnabled( true );
    pushButton_musicbrainz->setText( m_buttonMbText );
#else
    Q_UNUSED(results);
    Q_UNUSED(error);
#endif
}

void
TagDialog::fillSelected( KTRMResult selected ) //SLOT
{
#if HAVE_TUNEPIMP
    kdDebug() << k_funcinfo << endl;


    if ( m_bundle.url() == m_mbTrack ) {
        if ( !selected.title().isEmpty() )    kLineEdit_title->setText( selected.title() );
        if ( !selected.artist().isEmpty() )   kComboBox_artist->setCurrentText( selected.artist() );
        if ( !selected.album().isEmpty() )    kComboBox_album->setCurrentText( selected.album() );
        if ( selected.track() != 0 )          kIntSpinBox_track->setValue( selected.track() );
        if ( selected.year() != 0 )           kIntSpinBox_year->setValue( selected.year() );
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
#if HAVE_TUNEPIMP
    m_mbTrack = "";
#endif
}

////////////////////////////////////////////////////////////////////////////////
// PRIVATE
////////////////////////////////////////////////////////////////////////////////

void TagDialog::init()
{
    // delete itself when closing
    setWFlags( getWFlags() | Qt::WDestructiveClose );

    KConfig *config = Amarok::config( "TagDialog" );

    kTabWidget->addTab( summaryTab, i18n( "Summary" ) );
    kTabWidget->addTab( tagsTab, i18n( "Tags" ) );
    kTabWidget->addTab( lyricsTab, i18n( "Lyrics" ) );
    kTabWidget->addTab( statisticsTab, i18n( "Statistics" ) );
    kTabWidget->addTab( labelsTab, i18n( "Labels" ) );
    kTabWidget->setCurrentPage( config->readNumEntry( "CurrentTab", 0 ) );

    const QStringList artists = CollectionDB::instance()->artistList();
    kComboBox_artist->insertStringList( artists );
    kComboBox_artist->completionObject()->insertItems( artists );
    kComboBox_artist->completionObject()->setIgnoreCase( true );
    kComboBox_artist->setCompletionMode( KGlobalSettings::CompletionPopup );

    const QStringList albums = CollectionDB::instance()->albumList();
    kComboBox_album->insertStringList( albums );
    kComboBox_album->completionObject()->insertItems( albums );
    kComboBox_album->completionObject()->setIgnoreCase( true );
    kComboBox_album->setCompletionMode( KGlobalSettings::CompletionPopup );

    const QStringList composers = CollectionDB::instance()->composerList();
    kComboBox_composer->insertStringList( composers );
    kComboBox_composer->completionObject()->insertItems( composers );
    kComboBox_composer->completionObject()->setIgnoreCase( true );
    kComboBox_composer->setCompletionMode( KGlobalSettings::CompletionPopup );

    kComboBox_rating->insertStringList( MetaBundle::ratingList() );

//    const QStringList genres = MetaBundle::genreList();
    const QStringList genres = CollectionDB::instance()->genreList();
    kComboBox_genre->insertStringList( genres );
    kComboBox_genre->completionObject()->insertItems( genres );
    kComboBox_genre->completionObject()->setIgnoreCase( true );

    const QStringList labels = CollectionDB::instance()->labelList();
    //TODO: figure out a way to add auto-completion support to kTestEdit_selectedLabels

    //m_labelCloud = new KHTMLPart( labels_favouriteLabelsFrame );
    m_labelCloud = new HTMLView( labels_favouriteLabelsFrame );
    m_labelCloud->view()->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored, false );
    m_labelCloud->view()->setVScrollBarMode( QScrollView::AlwaysOff );
    m_labelCloud->view()->setHScrollBarMode( QScrollView::AlwaysOff );

    new QVBoxLayout( labels_favouriteLabelsFrame );
    labels_favouriteLabelsFrame->layout()->add( m_labelCloud->view() );
    const QStringList favoriteLabels = CollectionDB::instance()->favoriteLabels();
    QString html = generateHTML( favoriteLabels );
    m_labelCloud->set( html );
    connect( m_labelCloud->browserExtension(), SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs & ) ),
             this,                             SLOT( openURLRequest( const KURL & ) ) );

    // looks better to have a blank label than 0, we can't do this in
    // the UI file due to bug in Designer
    kIntSpinBox_track->setSpecialValueText( " " );
    kIntSpinBox_year->setSpecialValueText( " " );
    kIntSpinBox_score->setSpecialValueText( " " );
    kIntSpinBox_discNumber->setSpecialValueText( " " );

    if( !AmarokConfig::useRatings() )
    {
        kComboBox_rating->hide();
        ratingLabel->hide();
    }
    if( !AmarokConfig::useScores() )
    {
        kIntSpinBox_score->hide();
        scoreLabel->hide();
    }

    //HACK due to deficiency in Qt that will be addressed in version 4
    // QSpinBox doesn't emit valueChanged if you edit the value with
    // the lineEdit until you change the keyboard focus
    connect( kIntSpinBox_year->child( "qt_spinbox_edit" ),  SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
    connect( kIntSpinBox_track->child( "qt_spinbox_edit" ), SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
    connect( kIntSpinBox_score->child( "qt_spinbox_edit" ), SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
    connect( kIntSpinBox_discNumber->child( "qt_spinbox_edit" ), SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );

    // Connects for modification check
    connect( kLineEdit_title,   SIGNAL(textChanged( const QString& )),  SLOT(checkModified()) );
    connect( kComboBox_composer,SIGNAL(activated( int )),               SLOT(checkModified()) );
    connect( kComboBox_composer,SIGNAL(textChanged( const QString& )),  SLOT(checkModified()) );
    connect( kComboBox_artist,  SIGNAL(activated( int )),               SLOT(checkModified()) );
    connect( kComboBox_artist,  SIGNAL(textChanged( const QString& )),  SLOT(checkModified()) );
    connect( kComboBox_album,   SIGNAL(activated( int )),               SLOT(checkModified()) );
    connect( kComboBox_album,   SIGNAL(textChanged( const QString& )),  SLOT(checkModified()) );
    connect( kComboBox_genre,   SIGNAL(activated( int )),               SLOT(checkModified()) );
    connect( kComboBox_genre,   SIGNAL(textChanged( const QString& )),  SLOT(checkModified()) );
    connect( kComboBox_rating,  SIGNAL(activated( int )),               SLOT(checkModified()) );
    connect( kComboBox_rating,  SIGNAL(textChanged( const QString& )),  SLOT(checkModified()) );
    connect( kIntSpinBox_track, SIGNAL(valueChanged( int )),            SLOT(checkModified()) );
    connect( kIntSpinBox_year,  SIGNAL(valueChanged( int )),            SLOT(checkModified()) );
    connect( kIntSpinBox_score, SIGNAL(valueChanged( int )),            SLOT(checkModified()) );
    connect( kTextEdit_comment, SIGNAL(textChanged()),                  SLOT(checkModified()) );
    connect( kTextEdit_lyrics,  SIGNAL(textChanged()),                  SLOT(checkModified()) );
    connect( kTextEdit_selectedLabels, SIGNAL(textChanged()),           SLOT(checkModified()) );

    // Remember original button text
    m_buttonMbText = pushButton_musicbrainz->text();

    connect( pushButton_cancel,   SIGNAL(clicked()), SLOT(cancelPressed()) );
    connect( pushButton_ok,       SIGNAL(clicked()), SLOT(accept()) );
    connect( pushButton_open,     SIGNAL(clicked()), SLOT(openPressed()) );
    connect( pushButton_previous, SIGNAL(clicked()), SLOT(previousTrack()) );
    connect( pushButton_next,     SIGNAL(clicked()), SLOT(nextTrack()) );
    connect( checkBox_perTrack,   SIGNAL(clicked()), SLOT(perTrack()) );

    // set an icon for the open-in-konqui button
    pushButton_open->setIconSet( SmallIconSet( Amarok::icon( "files" ) ) );

    //Update lyrics on Context Browser
    connect( this, SIGNAL(lyricsChanged( const QString& )), ContextBrowser::instance(), SLOT( lyricsChanged( const QString& ) ) );

    //Update cover
    connect( CollectionDB::instance(), SIGNAL( coverFetched( const QString&, const QString& ) ),
            this, SLOT( loadCover( const QString&, const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( coverChanged( const QString&, const QString& ) ),
             this, SLOT( loadCover( const QString&, const QString& ) ) );



#if HAVE_TUNEPIMP
    connect( pushButton_musicbrainz, SIGNAL(clicked()), SLOT(musicbrainzQuery()) );
#else
    QToolTip::add( pushButton_musicbrainz, i18n("Please install MusicBrainz to enable this functionality") );
#endif

    connect( pushButton_guessTags, SIGNAL(clicked()), SLOT( guessFromFilename() ) );
    connect( pushButton_setFilenameSchemes, SIGNAL(clicked()), SLOT( setFileNameSchemes() ) );

    if( m_urlList.count() ) {   //editing multiple tracks
        m_perTrack = false;
        setMultipleTracksMode();
        readMultipleTracks();

        checkBox_perTrack->setChecked( m_perTrack );
        if( m_urlList.count() == 1 )
        {
            checkBox_perTrack->setEnabled( false );
            pushButton_previous->setEnabled( false );
            pushButton_next->setEnabled( false );
        }
        else
        {
            checkBox_perTrack->setEnabled( true );
            pushButton_previous->setEnabled( m_perTrack );
            pushButton_next->setEnabled( m_perTrack );
        }
    }
    else
    {
        m_perTrack = true;
        checkBox_perTrack->hide();

        if( !m_playlistItem ) {
            //We have already loaded the metadata (from the file) in the constructor
            pushButton_previous->hide();
            pushButton_next->hide();
        }
        else
        {
            //Reload the metadata from the file, to be sure it's accurate
            loadTags( m_playlistItem->url() );
        }

        loadLyrics( m_bundle.url() );
        loadLabels(  m_bundle.url() );
        readTags();
    }


    // make it as small as possible
    resize( sizeHint().width(), minimumSize().height() );

}


inline const QString TagDialog::unknownSafe( QString s ) {
    return ( s.isNull() || s.isEmpty() || s == "?" || s == "-" )
           ? i18n ( "Unknown" )
           : s;
}

const QStringList TagDialog::statisticsData() {

    QStringList data, values;
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
        data += values[0];

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
            data += values[0];
        }

        // Related Artists
        const QString sArtists = CollectionDB::instance()->similarArtists( m_bundle.artist(), 4 ).join(", ");
        if ( !sArtists.isEmpty() ) {
            data += i18n( "Related Artists" );
            data += sArtists;
        }
    }
    return data;
}

void TagDialog::readTags()
{
    bool local = m_bundle.url().isLocalFile();

    setCaption( kapp->makeStdCaption( i18n("Track Information: %1 by %2").arg( m_bundle.title(),  m_bundle.artist() ) ) );

    QString niceTitle;
    if ( m_bundle.album().isEmpty() ) {
        if( !m_bundle.title().isEmpty() ) {
            if( !m_bundle.artist().isEmpty() )
                niceTitle = i18n( "<b>%1</b> by <b>%2</b>" ).arg( m_bundle.title(),  m_bundle.artist() );
            else
                niceTitle = QString( "<b>%1</b>" ).arg( m_bundle.title() );
        }
        else niceTitle = m_bundle.prettyTitle();
    }
    else {
        niceTitle = i18n( "<b>%1</b> by <b>%2</b> on <b>%3</b>" )
            .arg( m_bundle.title(), m_bundle.artist(), m_bundle.album() );
    }
    trackArtistAlbumLabel->setText( niceTitle );
    trackArtistAlbumLabel2->setText( niceTitle );

    kLineEdit_title        ->setText( m_bundle.title() );
    kComboBox_artist       ->setCurrentText( m_bundle.artist() );
    kComboBox_album        ->setCurrentText( m_bundle.album() );
    kComboBox_genre        ->setCurrentText( m_bundle.genre() );
    kComboBox_rating       ->setCurrentItem( m_bundle.rating() );
    kIntSpinBox_track      ->setValue( m_bundle.track() );
    kComboBox_composer     ->setCurrentText( m_bundle.composer() );
    kIntSpinBox_year       ->setValue( m_bundle.year() );
    kIntSpinBox_score      ->setValue( static_cast<int>(m_bundle.score()) );
    kIntSpinBox_discNumber ->setValue( m_bundle.discNumber() );
    kTextEdit_comment      ->setText( m_bundle.comment() );

    bool extended = m_bundle.hasExtendedMetaInformation();
    kIntSpinBox_discNumber->setEnabled( extended );
    kComboBox_composer->setEnabled( extended );


    QString summaryText, statisticsText;
    const QString body2cols = i18n( "<tr><td>Label:</td><td><b>Value</b></td></tr>", "<tr><td><nobr>%1:</nobr></td><td><b>%2</b></td></tr>" );
    const QString body1col = "<tr><td colspan=2>%1</td></td></tr>";
    const QString emptyLine = "<tr><td colspan=2></td></tr>";

    summaryText = "<table width=100%><tr><td width=50%><table>";
    summaryText += body2cols.arg( i18n("Length"), unknownSafe( m_bundle.prettyLength() ) );
    summaryText += body2cols.arg( i18n("Bitrate"), unknownSafe( m_bundle.prettyBitrate() ) );
    summaryText += body2cols.arg( i18n("Samplerate"), unknownSafe( m_bundle.prettySampleRate() ) );
    summaryText += body2cols.arg( i18n("Size"), unknownSafe( m_bundle.prettyFilesize()  ) );
    summaryText += body2cols.arg( i18n("Format"), unknownSafe( m_bundle.type() ) );

    summaryText += "</table></td><td width=50%><table>";
    if( AmarokConfig::useScores() )
        summaryText += body2cols.arg( i18n("Score"), QString::number( static_cast<int>( m_bundle.score() ) ) );
    if( AmarokConfig::useRatings() )
        summaryText += body2cols.arg( i18n("Rating"), m_bundle.prettyRating() );

    summaryText += body2cols.arg( i18n("Playcount"), QString::number( m_bundle.playCount() ) );
    summaryText += body2cols.arg( i18n("First Played"),
                   m_bundle.playCount() ? KGlobal::locale()->formatDate( CollectionDB::instance()->getFirstPlay( m_bundle.url().path() ).date() , true ) : i18n("Never") );
    summaryText += body2cols.arg( i18n("a single item (singular)", "Last Played"),
                   m_bundle.playCount() ? KGlobal::locale()->formatDate( CollectionDB::instance()->getLastPlay( m_bundle.url().path() ).date() , true ) : i18n("Never") );

    summaryText += "</table></td></tr></table>";
    summaryLabel->setText( summaryText );

    statisticsText = "<table>";

    QStringList sData = statisticsData();
    for ( uint i = 0; i<sData.count(); i+=2 ) {
        statisticsText += body2cols.arg( sData[i], sData[i+1] );
    }

    statisticsText += "</table>";

    statisticsLabel->setText( statisticsText );

    kLineEdit_location->setText( local ? m_bundle.url().path() : m_bundle.url().url() );

    //lyrics
    kTextEdit_lyrics->setText( m_lyrics );

    loadCover( m_bundle.artist(), m_bundle.album() );


    // enable only for local files
    kLineEdit_title->setReadOnly( !local );
    kComboBox_artist->setEnabled( local );
    kComboBox_album->setEnabled( local );
    kComboBox_genre->setEnabled( local );
    kComboBox_rating->setEnabled( local );
    kIntSpinBox_track->setEnabled( local );
    kIntSpinBox_year->setEnabled( local );
    kIntSpinBox_score->setEnabled( local );
    kTextEdit_comment->setEnabled( local );
    kTextEdit_selectedLabels->setEnabled( local );
    m_labelCloud->view()->setEnabled( local );

    if( local )
    {
       pushButton_musicbrainz->show();
       pushButton_guessTags->show();
       pushButton_setFilenameSchemes->show();
    }
    else
    {
       pushButton_musicbrainz->hide();
       pushButton_guessTags->hide();
       pushButton_setFilenameSchemes->hide();
    }

    // If it's a local file, write the directory to m_path, else disable the "open in konqui" button
    if ( local )
        m_path = m_bundle.url().directory();
    else
        pushButton_open->setEnabled( false );

    pushButton_ok->setEnabled( storedTags.count() > 0 || storedScores.count() > 0
                              || storedLyrics.count() > 0 || storedRatings.count() > 0
                              || newLabels.count() > 0 );

#if HAVE_TUNEPIMP
    // Don't enable button if a query is in progress already (or if the file isn't local)
    pushButton_musicbrainz->setEnabled( m_bundle.url().isLocalFile() && m_mbTrack.isEmpty() );
#else
    pushButton_musicbrainz->setEnabled( false );
#endif

    if( m_playlistItem ) {
        pushButton_previous->setEnabled( m_playlistItem->itemAbove() );
        pushButton_next->setEnabled( m_playlistItem->itemBelow() );
    }
}


void
TagDialog::setMultipleTracksMode()
{

    kTabWidget->setTabEnabled( summaryTab, false );
    kTabWidget->setTabEnabled( lyricsTab, false );

    kComboBox_artist->setCurrentText( "" );
    kComboBox_album->setCurrentText( "" );
    kComboBox_genre->setCurrentText( "" );
    kComboBox_composer->setCurrentText( "" );
    kLineEdit_title->setText( "" );
    kTextEdit_comment->setText( "" );
    kIntSpinBox_track->setValue( kIntSpinBox_track->minValue() );
    kIntSpinBox_discNumber->setValue( kIntSpinBox_discNumber->minValue() );
    kIntSpinBox_year->setValue( kIntSpinBox_year->minValue() );

    kIntSpinBox_score->setValue( kIntSpinBox_score->minValue() );
    kComboBox_rating->setCurrentItem(  0 );

    kLineEdit_title->setEnabled( false );
    kIntSpinBox_track->setEnabled( false );

    pushButton_musicbrainz->hide();
    pushButton_guessTags->hide();
    pushButton_setFilenameSchemes->hide();

    locationLabel->hide();
    kLineEdit_location->hide();
    pushButton_open->hide();
    pixmap_cover->hide();
}

void
TagDialog::setSingleTrackMode()
{

    kTabWidget->setTabEnabled( summaryTab, true );
    kTabWidget->setTabEnabled( lyricsTab, true );

    kLineEdit_title->setEnabled( true );
    kIntSpinBox_track->setEnabled( true );

    pushButton_musicbrainz->show();
    pushButton_guessTags->show();
    pushButton_setFilenameSchemes->show();

    locationLabel->show();
    kLineEdit_location->show();
    pushButton_open->show();
    pixmap_cover->show();
}


void
TagDialog::readMultipleTracks()
{

    setCaption( kapp->makeStdCaption( i18n("1 Track", "Information for %n Tracks", m_urlList.count()) ) );

    //Check which fields are the same for all selected tracks
    const KURL::List::ConstIterator end = m_urlList.end();
    KURL::List::ConstIterator it = m_urlList.begin();

    m_bundle = MetaBundle();

    MetaBundle first = bundleForURL( *it );

    bool artist=true, album=true, genre=true, comment=true, year=true,
         score=true, rating=true, composer=true, discNumber=true;
    int songCount=0, ratingCount=0, ratingSum=0, scoreCount=0;
    float scoreSum = 0.f;
    for ( ; it != end; ++it ) {
        MetaBundle mb = bundleForURL( *it );
        songCount++;
        if ( mb.rating() ) {
            ratingCount++;
            ratingSum+=mb.rating();
        }
        if ( mb.score() > 0.f ) {
            scoreCount++;
            scoreSum+=mb.score();
        }

        if( !mb.url().isLocalFile() ) {
            // If we have a non local file, don't even lose more time comparing
            artist = album = genre = comment = year = false;
            score  = rating = composer = discNumber = false;
            continue;
        }
        if ( artist && mb.artist()!=first.artist() )
            artist=false;
        if ( album && mb.album()!=first.album() )
            album=false;
        if ( genre && mb.genre()!=first.genre() )
            genre=false;
        if ( comment && mb.comment()!=first.comment() )
            comment=false;
        if ( year && mb.year()!=first.year() )
            year=false;
        if ( composer && mb.composer()!=first.composer() )
            composer=false;
        if ( discNumber && mb.discNumber()!=first.discNumber() )
            discNumber=false;
        if ( score && mb.score()!=first.score()  )
            score = false;
        if ( rating && mb.rating()!=first.rating()  )
            rating = false;
    }
    // Set them in the dialog and in m_bundle ( so we don't break hasChanged() )
    if (artist) {
        m_bundle.setArtist( first.artist() );
        kComboBox_artist->setCurrentText( first.artist() );
    }
    if (album) {
        m_bundle.setAlbum( first.album() );
        kComboBox_album->setCurrentText( first.album() );
    }
    if (genre) {
        m_bundle.setGenre( first.genre() );
        kComboBox_genre->setCurrentText( first.genre() );
    }
    if (comment) {
        m_bundle.setComment( first.comment() );
        kTextEdit_comment->setText( first.comment() );
    }
    if (composer) {
        m_bundle.setComposer( first.composer() );
        kComboBox_composer->setCurrentText( first.composer() );
    }
    if (year) {
        m_bundle.setYear( first.year() );
        kIntSpinBox_year->setValue( first.year() );
    }
    if (discNumber) {
        m_bundle.setDiscNumber( first.discNumber() );
        kIntSpinBox_discNumber->setValue( first.discNumber() );
    }
    if (score) {
        m_bundle.setScore( first.score() );
        kIntSpinBox_score->setValue( static_cast<int>( first.score() ) );
    }
    if (rating) {
        m_bundle.setRating( first.rating() );
        kComboBox_rating->setCurrentItem( first.rating() );
    }

    m_currentURL = m_urlList.begin();

    trackArtistAlbumLabel2->setText( i18n( "Editing 1 file", "Editing %n files", songCount ) );

    const QString body = i18n( "<tr><td>Label:</td><td><b>Value</b></td></tr>", "<tr><td><nobr>%1:</nobr></td><td><b>%2</b></td></tr>" );
    QString statisticsText = "<table>";

    if( AmarokConfig::useRatings() ) {
        statisticsText += body.arg( i18n( "Rated Songs" ) , QString::number( ratingCount )  );
        if ( ratingCount )
            statisticsText += body.arg( i18n( "Average Rating" ) , QString::number( (float)ratingSum / (float)ratingCount/2.0, 'f', 1  ) );
    }

    if( AmarokConfig::useRatings() ) {
        statisticsText += body.arg( i18n( "Scored Songs" ) , QString::number( scoreCount )  );
        if ( scoreCount )
            statisticsText += body.arg( i18n( "Average Score" ) , QString::number( scoreSum / scoreCount, 'f', 1 ) );
    }


    statisticsText += "</table>";

    statisticsLabel->setText( statisticsText );

    QStringList commonLabels = getCommonLabels();
    QString text;
    foreach ( commonLabels )
    {
        if ( !text.isEmpty() )
            text.append( ", " );
        text.append( *it );
    }
    kTextEdit_selectedLabels->setText( text );
    m_commaSeparatedLabels = text;

    // This will reset a wrongly enabled Ok button
    checkModified();
}

QStringList
TagDialog::getCommonLabels()
{
    DEBUG_BLOCK
    QMap<QString, int> counterMap;
    const KURL::List::ConstIterator end = m_urlList.end();
    KURL::List::ConstIterator iter = m_urlList.begin();
    for(; iter != end; ++iter )
    {
        QStringList labels = labelsForURL( *iter );
        foreach( labels )
        {
            if ( counterMap.contains( *it ) )
                counterMap[ *it ] = counterMap[ *it ] +1;
            else
                counterMap[ *it ] = 1;
        }
    }
    int n = m_urlList.count();
    QStringList result;
    QMap<QString, int>::ConstIterator counterEnd( counterMap.end() );
    for(QMap<QString, int>::ConstIterator it = counterMap.begin(); it != counterEnd; ++it )
    {
        if ( it.data() == n )
            result.append( it.key() );
    }
    return result;
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
    int result=TagDialog::NOCHANGE;
    bool modified = false;
    modified |= !equalString( kComboBox_artist->lineEdit()->text(), m_bundle.artist() );
    modified |= !equalString( kComboBox_album->lineEdit()->text(), m_bundle.album() );
    modified |= !equalString( kComboBox_genre->lineEdit()->text(), m_bundle.genre() );
    modified |= kIntSpinBox_year->value()  != m_bundle.year();
    modified |= kIntSpinBox_discNumber->value()  != m_bundle.discNumber();
    modified |= !equalString( kComboBox_composer->lineEdit()->text(), m_bundle.composer() );

    modified |= !equalString( kTextEdit_comment->text(), m_bundle.comment() );

    if (!m_urlList.count() || m_perTrack) { //ignore these on MultipleTracksMode
        modified |= !equalString( kLineEdit_title->text(), m_bundle.title() );
        modified |= kIntSpinBox_track->value() != m_bundle.track();
    }
    if (modified)
        result |= TagDialog::TAGSCHANGED;

    if (kIntSpinBox_score->value() != m_bundle.score())
        result |= TagDialog::SCORECHANGED;
    if (kComboBox_rating->currentItem() != ( m_bundle.rating() ) )
        result |= TagDialog::RATINGCHANGED;

    if (!m_urlList.count() || m_perTrack) { //ignore these on MultipleTracksMode
        if ( !equalString( kTextEdit_lyrics->text(), m_lyrics ) )
            result |= TagDialog::LYRICSCHANGED;
    }

    if ( !equalString( kTextEdit_selectedLabels->text(), m_commaSeparatedLabels ) )
        result |= TagDialog::LABELSCHANGED;

    return result;
}

void
TagDialog::storeTags()
{
    storeTags( m_bundle.url() );
}

void
TagDialog::storeTags( const KURL &kurl )
{
    int result = changes();
    QString url = kurl.path();
    if( result & TagDialog::TAGSCHANGED ) {
        MetaBundle mb( m_bundle );

        mb.setTitle( kLineEdit_title->text() );
        mb.setComposer( kComboBox_composer->currentText() );
        mb.setArtist( kComboBox_artist->currentText() );
        mb.setAlbum( kComboBox_album->currentText() );
        mb.setComment( kTextEdit_comment->text() );
        mb.setGenre( kComboBox_genre->currentText() );
        mb.setTrack( kIntSpinBox_track->value() );
        mb.setYear( kIntSpinBox_year->value() );
        mb.setDiscNumber( kIntSpinBox_discNumber->value() );
        mb.setLength( m_bundle.length() );
        mb.setBitrate( m_bundle.bitrate() );
        mb.setSampleRate( m_bundle.sampleRate() );
        storedTags.replace( url, mb );
    }
    if( result & TagDialog::SCORECHANGED )
        storedScores.replace( url, kIntSpinBox_score->value() );
    if( result & TagDialog::RATINGCHANGED )
        storedRatings.replace( url, kComboBox_rating->currentItem() );
    if( result & TagDialog::LYRICSCHANGED ) {
        if ( kTextEdit_lyrics->text().isEmpty() )
            storedLyrics.replace( url, QString::null );
        else {
            QDomDocument doc;
            QDomElement e = doc.createElement( "lyrics" );
            e.setAttribute( "artist", kComboBox_artist->currentText() );
            e.setAttribute( "title", kLineEdit_title->text() );
            QDomText t = doc.createTextNode( kTextEdit_lyrics->text() );
            e.appendChild( t );
            doc.appendChild( e );
            storedLyrics.replace( url, doc.toString() );
        }
    }
    if( result & TagDialog::LABELSCHANGED ) {
        generateDeltaForLabelList( labelListFromText( kTextEdit_selectedLabels->text() ) );
        QStringList tmpLabels;
        if ( newLabels.find( url ) != newLabels.end() )
            tmpLabels = newLabels[ url ];
        else
            tmpLabels = originalLabels[ url ];
        //apply delta
        foreach( m_removedLabels )
        {
            tmpLabels.remove( *it );
        }
        foreach( m_addedLabels )
        {
            if( tmpLabels.find( *it ) == tmpLabels.end() )
                tmpLabels.append( *it );
        }
        newLabels.replace( url, tmpLabels );
    }
}

void
TagDialog::storeTags( const KURL &url, int changes, const MetaBundle &mb )
{
    if ( changes & TagDialog::TAGSCHANGED )
        storedTags.replace( url.path(), mb );
    if ( changes & TagDialog::SCORECHANGED )
        storedScores.replace( url.path(), mb.score() );
    if ( changes & TagDialog::RATINGCHANGED )
        storedRatings.replace( url.path(), mb.rating() );
}

void
TagDialog::storeLabels( const KURL &url, const QStringList &labels )
{
    newLabels.replace( url.path(), labels );
}


void
TagDialog::loadTags( const KURL &url )
{
    m_bundle = bundleForURL( url );
    loadLyrics( url );
    loadLabels( url );
}

void
TagDialog::loadLyrics( const KURL &url )
{
    QString xml = lyricsForURL(url.path() );

    QDomDocument doc;
    if( doc.setContent( xml ) )
        m_lyrics = doc.documentElement().text();
    else
        m_lyrics = QString::null;
}

void
TagDialog::loadLabels( const KURL &url )
{
    DEBUG_BLOCK
    m_labels = labelsForURL( url );
    originalLabels[ url.path() ] = m_labels;
    QString text;
    foreach( m_labels )
    {
        if ( !text.isEmpty() )
            text.append( ", " );
        text.append( *it );
    }
    kTextEdit_selectedLabels->setText( text );
    m_commaSeparatedLabels = text;
}

MetaBundle
TagDialog::bundleForURL( const KURL &url )
{
    if( storedTags.find( url.path() ) != storedTags.end() )
        return storedTags[ url.path() ];

    return MetaBundle( url, url.isLocalFile() );
}

float
TagDialog::scoreForURL( const KURL &url )
{
    if( storedScores.find( url.path() ) != storedScores.end() )
        return storedScores[ url.path() ];

    return CollectionDB::instance()->getSongPercentage( url.path() );
}

int
TagDialog::ratingForURL( const KURL &url )
{
    if( storedRatings.find( url.path() ) != storedRatings.end() )
        return storedRatings[ url.path() ];

    return CollectionDB::instance()->getSongRating( url.path() );
}

QString
TagDialog::lyricsForURL( const KURL &url )
{
    if( storedLyrics.find( url.path() ) != storedLyrics.end() )
        return storedLyrics[ url.path() ];

    return CollectionDB::instance()->getLyrics( url.path() );
}

QStringList
TagDialog::labelsForURL( const KURL &url )
{
    if( newLabels.find( url.path() ) != newLabels.end() )
        return newLabels[ url.path() ];
    if( originalLabels.find( url.path() ) != originalLabels.end() )
        return originalLabels[ url.path() ];
    QStringList tmp = CollectionDB::instance()->getLabels( url.path(), CollectionDB::typeUser );
    originalLabels[ url.path() ] = tmp;
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

    QMap<QString, float>::ConstIterator endScore( storedScores.end() );
    for(QMap<QString, float>::ConstIterator it = storedScores.begin(); it != endScore; ++it ) {
        CollectionDB::instance()->setSongPercentage( it.key(), it.data() );
    }
    QMap<QString, int>::ConstIterator endRating( storedRatings.end() );
    for(QMap<QString, int>::ConstIterator it = storedRatings.begin(); it != endRating; ++it ) {
        CollectionDB::instance()->setSongRating( it.key(), it.data() );
    }
    QMap<QString, QString>::ConstIterator endLyrics( storedLyrics.end() );
    for(QMap<QString, QString>::ConstIterator it = storedLyrics.begin(); it != endLyrics; ++it ) {
        CollectionDB::instance()->setLyrics( it.key(), it.data(),
               CollectionDB::instance()->uniqueIdFromUrl( KURL( it.key() ) ) );
        emit lyricsChanged( it.key() );
    }
    QMap<QString, QStringList>::ConstIterator endLabels( newLabels.end() );
    for(QMap<QString, QStringList>::ConstIterator it = newLabels.begin(); it != endLabels; ++it ) {
        CollectionDB::instance()->setLabels( it.key(), it.data(),
                CollectionDB::instance()->uniqueIdFromUrl( KURL( it.key() ) ), CollectionDB::typeUser );
    }
    CollectionDB::instance()->cleanLabels();

    ThreadManager::instance()->queueJob( new TagDialogWriter( storedTags ) );

}

void
TagDialog::applyToAllTracks()
{
    generateDeltaForLabelList( labelListFromText( kTextEdit_selectedLabels->text() ) );

    const KURL::List::ConstIterator end = m_urlList.end();
    for ( KURL::List::ConstIterator it = m_urlList.begin(); it != end; ++it ) {

        /* we have to update the values if they changed, so:
           1) !kLineEdit_field->text().isEmpty() && kLineEdit_field->text() != mb.field
           i.e.: The user wrote something on the field, and it's different from
           what we have in the tag.
           2) !m_bundle.field().isEmpty() && kLineEdit_field->text().isEmpty()
           i.e.: The user was shown some value for the field (it was the same
           for all selected tracks), and he deliberately emptied it.
           TODO: All this mess is because the dialog uses "" to represent what the user
                 doesn't want to change, maybe we can think of something better?
         */

        MetaBundle mb = bundleForURL( *it );

        int changed = 0;
        if( !kComboBox_artist->currentText().isEmpty() && kComboBox_artist->currentText() != mb.artist() ||
                kComboBox_artist->currentText().isEmpty() && !m_bundle.artist().isEmpty() ) {
            mb.setArtist( kComboBox_artist->currentText() );
            changed |= TagDialog::TAGSCHANGED;
        }

        if( !kComboBox_album->currentText().isEmpty() && kComboBox_album->currentText() != mb.album() ||
                kComboBox_album->currentText().isEmpty() && !m_bundle.album().isEmpty() ) {
            mb.setAlbum( kComboBox_album->currentText() );
            changed |= TagDialog::TAGSCHANGED;
        }
        if( !kComboBox_genre->currentText().isEmpty() && kComboBox_genre->currentText() != mb.genre() ||
                kComboBox_genre->currentText().isEmpty() && !m_bundle.genre().isEmpty() ) {
            mb.setGenre( kComboBox_genre->currentText() );
            changed |= TagDialog::TAGSCHANGED;
        }
        if( !kTextEdit_comment->text().isEmpty() && kTextEdit_comment->text() != mb.comment() ||
                kTextEdit_comment->text().isEmpty() && !m_bundle.comment().isEmpty() ) {
            mb.setComment( kTextEdit_comment->text() );
            changed |= TagDialog::TAGSCHANGED;
        }
        if( !kComboBox_composer->currentText().isEmpty() && kComboBox_composer->currentText() != mb.composer() ||
             kComboBox_composer->currentText().isEmpty() && !m_bundle.composer().isEmpty() ) {
            mb.setComposer( kComboBox_composer->currentText() );
            changed |= TagDialog::TAGSCHANGED;
        }

        if( kIntSpinBox_year->value() && kIntSpinBox_year->value() != mb.year() ||
                !kIntSpinBox_year->value() && m_bundle.year() ) {
            mb.setYear( kIntSpinBox_year->value() );
            changed |= TagDialog::TAGSCHANGED;
        }
        if( kIntSpinBox_discNumber->value() && kIntSpinBox_discNumber->value() != mb.discNumber() ||
                !kIntSpinBox_discNumber->value() && m_bundle.discNumber() ) {
            mb.setDiscNumber( kIntSpinBox_discNumber->value() );
            changed |= TagDialog::TAGSCHANGED;
        }

        if( kIntSpinBox_score->value() && kIntSpinBox_score->value() != mb.score() ||
                !kIntSpinBox_score->value() && m_bundle.score() )
        {
            mb.setScore( kIntSpinBox_score->value() );
            changed |= TagDialog::SCORECHANGED;
        }

        if( kComboBox_rating->currentItem() && kComboBox_rating->currentItem() != m_bundle.rating() ||
                !kComboBox_rating->currentItem() && m_bundle.rating() )
        {
            mb.setRating( kComboBox_rating->currentItem() );
            changed |= TagDialog::RATINGCHANGED;
        }
        storeTags( *it, changed, mb );

        QStringList tmpLabels = labelsForURL( *it );
        //apply delta
        for( QStringList::Iterator iter = m_removedLabels.begin(); iter != m_removedLabels.end(); ++iter )
        {
            tmpLabels.remove( *iter );
        }
        for( QStringList::Iterator iter = m_addedLabels.begin(); iter != m_addedLabels.end(); ++iter )
        {
            if( tmpLabels.find( *iter ) == tmpLabels.end() )
                tmpLabels.append( *iter );
        }
        storeLabels( *it, tmpLabels );
    }
}

QStringList
TagDialog::labelListFromText( const QString &text )
{
    QStringList tmp = QStringList::split( ',', text );
    //insert each string into a map to remove duplicates
    QMap<QString, int> map;
    foreach( tmp )
    {
        QString tmpString = (*it).stripWhiteSpace();
        if ( !tmpString.isEmpty() )
            map.replace( tmpString, 0 );
    }
    QStringList result;
    QMap<QString, int>::ConstIterator endMap( map.end() );
    for(QMap<QString, int>::ConstIterator it = map.begin(); it != endMap; ++it ) {
        result.append( it.key() );
    }
    return result;
}

void
TagDialog::generateDeltaForLabelList( const QStringList &list )
{
    m_addedLabels.clear();
    m_removedLabels.clear();
    foreach( list )
    {
        if ( !m_labels.contains( *it ) )
            m_addedLabels.append( *it );
    }
    foreach( m_labels )
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
    foreach( labels )
    {
        QString label = *it;
        sortedLabels << label.lower();
        ++it;
        int value = ( *it ).toInt();
        if ( value > max )
            max = value;
        mapping[label.lower()] = QPair<QString, int>( label, value );
    }
    sortedLabels.sort();
    QString html = "<html><body>";
    foreach( sortedLabels )
    {
        QString key = *it;
        //generate a number in the range 1..10 based on  how much the label is used
        int labelUse = ( mapping[key].second * 10 ) / max;
        if ( labelUse == 0 )
            labelUse = 1;
        html.append( QString( "<span class='label size%1'><a href=\"label:%2\">%3</a></span> " )
                              .arg( QString::number( labelUse ), mapping[key].first, mapping[key].first ) );
    }
    html.append( "</html></body>" );
    debug() << "Dumping HTML for label cloud: " << html << endl;
    return html;
}

void
TagDialog::openURLRequest(const KURL &url )         //SLOT
{
    DEBUG_BLOCK
    if ( url.protocol() == "label" )
    {
        QString text = kTextEdit_selectedLabels->text();
        QStringList currentLabels = labelListFromText( text );
        if ( currentLabels.contains( url.path() ) )
            return;
        if ( !text.isEmpty() )
            text.append( ", " );
        text.append( url.path() );
        kTextEdit_selectedLabels->setText( text );
    }
}

bool
TagDialog::writeTag( MetaBundle &mb, bool updateCB )
{
    QCString path = QFile::encodeName( mb.url().path() );
    if ( !TagLib::File::isWritable( path ) ) {
        Amarok::StatusBar::instance()->longMessage( i18n(
           "The file %1 is not writable." ).arg( mb.url().fileName() ), KDE::StatusBar::Error );
        return false;
    }

    //visual feedback
    QApplication::setOverrideCursor( KCursor::waitCursor() );

    bool result = mb.save();
    mb.updateFilesize();

    if( result )
        //update the collection db
        CollectionDB::instance()->updateTags( mb.url().path(), mb, updateCB );

    QApplication::restoreOverrideCursor();

    return result;
}

TagDialogWriter::TagDialogWriter( const QMap<QString, MetaBundle> tagsToChange )
        : ThreadManager::Job( "TagDialogWriter" ),
          m_successCount ( 0 ),
          m_failCount    ( 0 )
{
    QApplication::setOverrideCursor( KCursor::waitCursor() );
    QMap<QString, MetaBundle>::ConstIterator end = tagsToChange.end();
    for(QMap<QString, MetaBundle>::ConstIterator it = tagsToChange.begin(); it != end; ++it ) {
        MetaBundle mb = it.data();
        mb.detach();
        m_tags += mb;
    }
}

bool
TagDialogWriter::doJob()
{
    for( int i = 0, size=m_tags.size(); i<size; ++i ) {
        QCString path = QFile::encodeName( m_tags[i].url().path() );
        if ( !TagLib::File::isWritable( path ) ) {
            Amarok::StatusBar::instance()->longMessageThreadSafe( i18n(
                "The file %1 is not writable." ).arg( m_tags[i].url().fileName() ), KDE::StatusBar::Error );
            m_failed += true;
            continue;
        }

        bool result = m_tags[i].save();
        m_tags[i].updateFilesize();

        if( result )
            m_successCount++;
        else {
            m_failCount++;
            m_failedURLs += m_tags[i].prettyURL();
        }
        m_failed += !result;
    }
    return true;
}

void
TagDialogWriter::completeJob()
{
     for( int i = 0, size=m_tags.size(); i<size; ++i ) {
        if ( !m_failed[i] ) {
            CollectionDB::instance()->updateTags( m_tags[i].url().path(), m_tags[i], false /* don't update browsers*/ );
            Playlist::instance()->updateMetaData( m_tags[i] );
        }
     }
     QApplication::restoreOverrideCursor();
     if ( m_successCount )
        CollectionView::instance()->databaseChanged();
     if ( m_failCount )
        Amarok::StatusBar::instance()->longMessage( i18n(
                        "Sorry, the tag for the following files could not be changed:\n" ).arg( m_failedURLs.join( ";\n" ) ), KDE::StatusBar::Error );
}


#include "tagdialog.moc"
