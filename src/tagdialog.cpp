// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>
// (c) 2005 Alexandre Pereira de Oliveira <aleprj@gmail.com>
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
#include <qlabel.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qcheckbox.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <kcursor.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <ktabwidget.h>
#include <ktextedit.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <krun.h>
#include <kstandarddirs.h>


TagDialog::TagDialog( const KURL& url, QWidget* parent )
    : TagDialogBase( parent )
    , m_bundle( url, true )
    , m_score ( CollectionDB::instance()->getSongPercentage( url.path() ) )
    , m_playcount( CollectionDB::instance()->getPlayCount( url.path() ) )
    , m_firstPlay ( CollectionDB::instance()->getFirstPlay( url.path() ) )
    , m_lastPlay ( CollectionDB::instance()->getLastPlay( url.path() ) )
    , m_playlistItem( 0 )
    , m_currentCover( 0 )
{
    init();
}


TagDialog::TagDialog( const KURL::List list, QWidget* parent )
    : TagDialogBase( parent )
    , m_bundle()
    , m_score ( 0 )
    , m_firstPlay ( QDateTime() )
    , m_lastPlay ( QDateTime()  )
    , m_playlistItem( 0 )
    , m_urlList( list )
    , m_currentCover( 0 )
{
    setCaption( kapp->makeStdCaption( i18n("1 Track", "Information for %n Tracks", list.count()) ) );
    init();
}


TagDialog::TagDialog( const MetaBundle& mb, PlaylistItem* item, QWidget* parent )
    : TagDialogBase( parent )
    , m_bundle( mb )
    , m_score ( CollectionDB::instance()->getSongPercentage( mb.url().path() ) )
    , m_playcount( CollectionDB::instance()->getPlayCount( mb.url().path() ) )
    , m_firstPlay ( CollectionDB::instance()->getFirstPlay( mb.url().path() ) )
    , m_lastPlay ( CollectionDB::instance()->getLastPlay( mb.url().path() ) )
    , m_playlistItem( item )
    , m_currentCover( 0 )
{
    init();
}


TagDialog::~TagDialog()
{
    KConfig *config = amaroK::config( "TagDialog" );
    config->writeEntry( "CurrentTab", kTabWidget->currentPageIndex() );
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
    amaroK::invokeBrowser( m_path );
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
    pushButton_ok->setEnabled( hasChanged() || storedTags.count() > 0 || storedScores.count() > 0|| storedLyrics.count() > 0 );
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
}

void
TagDialog::musicbrainzQuery() //SLOT
{
#if HAVE_TUNEPIMP
    kdDebug() << k_funcinfo << endl;

    m_mbTrack = m_bundle.url();
    KTRMLookup* ktrm = new KTRMLookup( m_mbTrack.path(), true );
    connect( ktrm, SIGNAL( sigResult( KTRMResultList, QString ) ), SLOT( queryDone( KTRMResultList, QString ) ) );

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

    //NOTE We allocate on the stack in Playlist
    if( parent() != Playlist::instance()->qscrollview() )
        setWFlags( getWFlags() | Qt::WDestructiveClose );

    KConfig *config = amaroK::config( "TagDialog" );

    kTabWidget->addTab( summaryTab, i18n( "Summary" ) );
    kTabWidget->addTab( tagsTab, i18n( "Tags" ) );
    kTabWidget->addTab( lyricsTab, i18n( "Lyrics" ) );
    kTabWidget->addTab( statisticsTab, i18n( "Statistics" ) );
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

    // Remember original button text
    m_buttonMbText = pushButton_musicbrainz->text();

    connect( pushButton_cancel,   SIGNAL(clicked()), SLOT(cancelPressed()) );
    connect( pushButton_ok,       SIGNAL(clicked()), SLOT(accept()) );
    connect( pushButton_open,     SIGNAL(clicked()), SLOT(openPressed()) );
    connect( pushButton_previous, SIGNAL(clicked()), SLOT(previousTrack()) );
    connect( pushButton_next,     SIGNAL(clicked()), SLOT(nextTrack()) );
    connect( checkBox_perTrack,   SIGNAL(clicked()), SLOT(perTrack()) );

    // set an icon for the open-in-konqui button
    pushButton_open->setIconSet( SmallIconSet( amaroK::icon( "files" ) ) );

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
            pushButton_previous->hide();
            pushButton_next->hide();
        }
        loadLyrics( m_bundle.url() );
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
        qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
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
            qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
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
    kComboBox_rating       ->setCurrentItem( m_bundle.rating() ? m_bundle.rating() - 1 : 0 );
    kIntSpinBox_track      ->setValue( m_bundle.track() );
    kComboBox_composer     ->setCurrentText( m_bundle.composer() );
    kIntSpinBox_year       ->setValue( m_bundle.year() );
    kIntSpinBox_score      ->setValue( m_score );
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
        summaryText += body2cols.arg( i18n("Score"), QString::number( m_score ) );
    if( AmarokConfig::useRatings() )
        summaryText += body2cols.arg( i18n("Rating"), m_bundle.prettyRating() );

    summaryText += body2cols.arg( i18n("Playcount"), QString::number( m_playcount ) );
    summaryText += body2cols.arg( i18n("First Played"), m_playcount ? KGlobal::locale()->formatDate( m_firstPlay.date() , true ) : i18n("Never") );
    summaryText += body2cols.arg( i18n("Last Played"), m_playcount ? KGlobal::locale()->formatDate( m_lastPlay.date() , true ) : i18n("Never") );

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

    // If it's a local file, write the directory to m_path, else disable the "open in konqui" button
    if ( local )
        m_path = m_bundle.url().directory();
    else
        pushButton_open->setEnabled( false );

    pushButton_ok->setEnabled( storedTags.count() > 0 || storedScores.count() > 0 || storedLyrics.count() > 0 );

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
    kTabWidget->setTabEnabled( statisticsTab, false );
    kTabWidget->setCurrentPage( 1 );

    kComboBox_artist->setCurrentText( "" );
    kComboBox_album->setCurrentText( "" );
    kComboBox_genre->setCurrentText( "" );
    kComboBox_composer->setCurrentText( "" );
    kLineEdit_title->setText( "" );
    kIntSpinBox_track->setValue( kIntSpinBox_track->minValue() );

    kLineEdit_title->setEnabled( false );
    kIntSpinBox_track->setEnabled( false );
    kIntSpinBox_discNumber->setEnabled( false );

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
    kTabWidget->setTabEnabled( statisticsTab, true );

    kComboBox_artist->setCurrentText( "" );
    kComboBox_album->setCurrentText( "" );
    kComboBox_genre->setCurrentText( "" );

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
    //Check which fields are the same for all selected tracks
    const KURL::List::ConstIterator end = m_urlList.end();
    KURL::List::ConstIterator it = m_urlList.begin();

    m_bundle = MetaBundle();

    MetaBundle first = bundleForURL( *it );
    uint scoreFirst = CollectionDB::instance()->getSongPercentage( first.url().path() );

    bool artist=true, album=true, genre=true, comment=true,
         year=true, score=true, composer=true, discNumber=true;
    for ( ; it != end; ++it ) {
        MetaBundle mb = bundleForURL( *it );

        if( !mb.url().isLocalFile() ) {
            // If we have a non local file, don't even lose more time comparing, just leave
            artist = album = genre = comment = year = score = composer = discNumber = false;
            break;
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

        uint scoreCurrent = CollectionDB::instance()->getSongPercentage( mb.url().path() );
        if ( score && scoreFirst != scoreCurrent )
            score = false;

        if (!artist && !album && !genre && !comment && !year && !score && !composer && !discNumber)
            break;
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
        m_score = scoreFirst;
        kIntSpinBox_score->setValue( scoreFirst );
    }

    m_currentURL = m_urlList.begin();

    // This will reset a wrongly enabled Ok button
    checkModified();
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

    if (!m_urlList.count() || m_perTrack) { //ignore these on MultipleTracksMode
        if (kIntSpinBox_score->value() != m_score)
            result |= TagDialog::SCORECHANGED;
        if (kComboBox_rating->currentItem() != ( m_bundle.rating() ? m_bundle.rating() - 1 : 0 ) )
            result |= TagDialog::RATINGCHANGED;
        if ( !equalString( kTextEdit_lyrics->text(), m_lyrics ) )
            result |= TagDialog::LYRICSCHANGED;
    }
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
        storedRatings.replace( url, kComboBox_rating->currentItem() ? kComboBox_rating->currentItem() + 1 : 0 );
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
}

void
TagDialog::storeTags( const KURL &url, int changes, const MetaBundle &mb, int score )
{
    if ( changes & TagDialog::TAGSCHANGED )
        storedTags.replace( url.path(), mb );
    if ( changes & TagDialog::SCORECHANGED )
        storedScores.replace( url.path(), score );
}


void
TagDialog::loadTags( const KURL &url )
{
    m_bundle = bundleForURL( url );
    m_score = scoreForURL( url );
    m_bundle.setRating( ratingForURL( url ) );

    loadLyrics( url );

    m_playcount = CollectionDB::instance()->getPlayCount( url.path() );
    m_firstPlay = CollectionDB::instance()->getFirstPlay( url.path() );
    m_lastPlay = CollectionDB::instance()->getLastPlay( url.path() );
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

MetaBundle
TagDialog::bundleForURL( const KURL &url )
{
    if( storedTags.find( url.path() ) != storedTags.end() )
        return storedTags[ url.path() ];

    return MetaBundle( url, true );
}

int
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

    QMap<QString, MetaBundle>::ConstIterator endStore( storedTags.end() );
    for(QMap<QString, MetaBundle>::ConstIterator it = storedTags.begin(); it != endStore; ++it ) {
        if( writeTag( it.data(), it == --storedTags.end() ) )    //update the collection browser if it's the last track
            Playlist::instance()->updateMetaData( it.data() );
        else
            amaroK::StatusBar::instance()->longMessage( i18n(
                        "Sorry, the tag for %1 could not be changed." ).arg( it.data().prettyURL() ), KDE::StatusBar::Error );
    }
    QMap<QString, int>::ConstIterator endScore( storedScores.end() );
    for(QMap<QString, int>::ConstIterator it = storedScores.begin(); it != endScore; ++it ) {
        CollectionDB::instance()->setSongPercentage( it.key(), it.data() );
    }
    QMap<QString, int>::ConstIterator endRating( storedRatings.end() );
    for(QMap<QString, int>::ConstIterator it = storedRatings.begin(); it != endRating; ++it ) {
        CollectionDB::instance()->setSongRating( it.key(), it.data() );
    }
    QMap<QString, QString>::ConstIterator endLyrics( storedLyrics.end() );
    for(QMap<QString, QString>::ConstIterator it = storedLyrics.begin(); it != endLyrics; ++it ) {
        CollectionDB::instance()->setLyrics( it.key(), it.data() );
        emit lyricsChanged( it.key() );
    }
}


void
TagDialog::applyToAllTracks()
{
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

        if( kIntSpinBox_score->value() && kIntSpinBox_score->value() != m_score ||
                !kIntSpinBox_score->value() && m_score )
        {
            m_score = kIntSpinBox_score->value();
            changed |= TagDialog::SCORECHANGED;
        }

        storeTags( *it, changed, mb, m_score );
    }
}


bool
TagDialog::writeTag( MetaBundle mb, bool updateCB )
{
    QCString path = QFile::encodeName( mb.url().path() );
    if ( !TagLib::File::isWritable( path ) ) {
        amaroK::StatusBar::instance()->longMessage( i18n(
           "The file %1 is not writable." ).arg( path ), KDE::StatusBar::Error );
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
#include "tagdialog.moc"
