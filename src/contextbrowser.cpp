// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information

#define DEBUG_PREFIX "ContextBrowser"

#include "amarok.h"
#include "amarokconfig.h"
#include "collectionbrowser.h" //FIXME for setupDirs()
#include "collectiondb.h"
#include "colorgenerator.h"
#include "config.h"        //for AMAZON_SUPPORT
#include "contextbrowser.h"
#include "coverfetcher.h"
#include "covermanager.h"
#include "debug.h"
#include "enginecontroller.h"
#include "metabundle.h"
#include "playlist.h"      //appendMedia()
#include "qstringx.h"

#include <qdatetime.h>
#include <qfile.h> // External CSS opening
#include <qimage.h>
#include <qregexp.h>
#include <qtextstream.h>  // External CSS reading

#include <kapplication.h> //kapp
#include <kfiledialog.h>
#include <kglobal.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kiconloader.h>
#include <kimageeffect.h> // gradient background image
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <krun.h>
#include <kstandarddirs.h> //locate file
#include <ktabbar.h>
#include <ktempfile.h>
#include <kurl.h>

#define escapeHTML(s)     QString(s).replace( "&", "&amp;" ).replace( "<", "&lt;" ).replace( ">", "&gt;" )
#define escapeHTMLAttr(s) QString(s).replace( "%", "%25" ).replace( "'", "%27" )


using amaroK::QStringx;

/**
 * Function that must be used when separating contextBrowser escaped urls
 */
static inline
void albumArtistFromUrl( QString url, QString &artist, QString &album )
{
    //KHTML removes the trailing space!
    if ( url.endsWith( " @@@" ) )
        url += ' ';

    const QStringList list = QStringList::split( " @@@ ", url, true );

    Q_ASSERT( !list.isEmpty() );

    artist = list.front();
    album  = list.back();
}


ContextBrowser::ContextBrowser( const char *name )
        : QTabWidget( 0, name )
        , EngineObserver( EngineController::instance() )
        , m_bgGradientImage( 0 )
        , m_headerGradientImage( 0 )
        , m_shadowGradientImage( 0 )
{
    m_homePage = new KHTMLPart( this, "home_page" );
    m_homePage->setDNDEnabled( true );
    m_currentTrackPage = new KHTMLPart( this, "current_track_page" );
    m_currentTrackPage->setDNDEnabled( true );
    m_lyricsPage = new KHTMLPart( this, "lyrics_page" );
    m_lyricsPage->setDNDEnabled( true );

    //aesthetics - no double frame
//     m_homePage->view()->setFrameStyle( QFrame::NoFrame );
//     m_currentTrackPage->view()->setFrameStyle( QFrame::NoFrame );
//     m_lyricsPage->view()->setFrameStyle( QFrame::NoFrame );

    addTab( m_homePage->view(),  SmallIconSet( "gohome" ), i18n( "Home" ) );
    addTab( m_currentTrackPage->view(), SmallIconSet( "today" ), i18n( "Current Track" ) );
    addTab( m_lyricsPage->view(), SmallIconSet( "document" ), i18n( "Lyrics" ) );

    m_dirtyHomePage = true;
    m_dirtyCurrentTrackPage = true;
    m_dirtyLyricsPage = true;

    m_emptyDB = CollectionDB::instance()->isEmpty();

    connect( this, SIGNAL( currentChanged( QWidget* ) ), SLOT( tabChanged( QWidget* ) ) );

    connect( m_homePage->browserExtension(), SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs & ) ),
             this,                             SLOT( openURLRequest( const KURL & ) ) );
    connect( m_homePage,                     SIGNAL( popupMenu( const QString&, const QPoint& ) ),
             this,                             SLOT( slotContextMenu( const QString&, const QPoint& ) ) );
    connect( m_currentTrackPage->browserExtension(), SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs & ) ),
             this,                                     SLOT( openURLRequest( const KURL & ) ) );
    connect( m_currentTrackPage,                     SIGNAL( popupMenu( const QString&, const QPoint& ) ),
             this,                                     SLOT( slotContextMenu( const QString&, const QPoint& ) ) );
    connect( m_lyricsPage->browserExtension(), SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs & ) ),
             this,                               SLOT( openURLRequest( const KURL & ) ) );
    connect( m_lyricsPage,                     SIGNAL( popupMenu( const QString&, const QPoint& ) ),
             this,                               SLOT( slotContextMenu( const QString&, const QPoint& ) ) );

    connect( CollectionDB::instance(), SIGNAL( scanStarted() ), SLOT( collectionScanStarted() ) );
    connect( CollectionDB::instance(), SIGNAL( scanDone( bool ) ), SLOT( collectionScanDone() ) );
    connect( CollectionDB::instance(), SIGNAL( databaseEngineChanged() ), SLOT( renderView() ) );
    connect( CollectionDB::instance(), SIGNAL( coverFetched( const QString&, const QString& ) ),
             this,                       SLOT( coverFetched( const QString&, const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( coverRemoved( const QString&, const QString& ) ),
             this,                       SLOT( coverRemoved( const QString&, const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( similarArtistsFetched( const QString& ) ),
             this,                       SLOT( similarArtistsFetched( const QString& ) ) );

    //the stylesheet will be set up and home will be shown later due to engine signals and doodaa
    //if we call it here setStyleSheet is called 3 times during startup!!
}


ContextBrowser::~ContextBrowser()
{
    delete m_bgGradientImage;
    delete m_headerGradientImage;
    delete m_shadowGradientImage;
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::setFont( const QFont &newFont )
{
//    if( newFont != font() ) {
        QWidget::setFont( newFont );
        setStyleSheet();
//    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::openURLRequest( const KURL &url )
{
    QString artist, album;
    albumArtistFromUrl( url.path(), artist, album );

    if ( url.protocol() == "album" )
    {
        QString sql = "SELECT DISTINCT url FROM tags WHERE artist = %1 AND album = %2 ORDER BY track;";
        QStringList values = CollectionDB::instance()->query( sql.arg( artist, album ) );
        KURL::List urls;
        KURL url;

        for( QStringList::ConstIterator it = values.begin(), end = values.end(); it != end; ++it ) {
            url.setPath( *it );
            urls.append( url );
        }

        Playlist::instance()->insertMedia( urls, Playlist::Unique );

        return;
    }

    if ( url.protocol() == "file" )
        Playlist::instance()->insertMedia( url, Playlist::DirectPlay | Playlist::Unique );

    if ( url.protocol() == "show" )
    {
        if ( url.path() == "home" )
            showHome();
        else if ( url.path() == "context" || url.path() == "stream" )
        {
            // NOTE: not sure if rebuild is needed, just to be safe
            m_dirtyCurrentTrackPage = true;
            showCurrentTrack();
        }
        else if ( url.path().contains( "suggestLyric-" ) )
        {
            m_lyrics = QString::null;
            QString hash = url.path().mid( url.path().find( QString( "-" ) ) +1 );
            m_dirtyLyricsPage = true;
            showLyrics( hash );
        }
        else if ( url.path() == "lyrics" )
            showLyrics();
        else if ( url.path() == "collectionSetup" )
        {
            //TODO if we do move the configuration to the main configdialog change this,
            //     otherwise we need a better solution
            QObject *o = parent()->child( "CollectionBrowser" );
            if ( o )
                static_cast<CollectionBrowser*>( o )->setupDirs();
        }
    }

    // When left-clicking on cover image, open browser with amazon site
    if ( url.protocol() == "fetchcover" )
    {
        QImage img( CollectionDB::instance()->albumImage( artist, album, 0 ) );
        const QString amazonUrl = img.text( "amazon-url" );
        debug() << "Embedded amazon url in cover image: " << amazonUrl << endl;

        if ( amazonUrl.isEmpty() )
            KMessageBox::information( this, i18n( "<p>There is no product information available for this image.<p>Right-click on image for menu." ) );
        else
            kapp->invokeBrowser( amazonUrl );
    }

    /* open konqueror with musicbrainz search result for artist-album */
    if ( url.protocol() == "musicbrainz" )
    {
        const QString url = "http://www.musicbrainz.org/taglookup.html?artist=%1&album=%2";
        kapp->invokeBrowser( url.arg( artist, album ) );
    }

    if ( url.protocol() == "lyricspage" )
    {
        kapp->invokeBrowser( url.url().replace("lyricspage:", "http://lyrc.com.ar/en/add/add.php?") );
    }
}


void ContextBrowser::collectionScanStarted()
{
    if( m_emptyDB )
       showScanning();
}


void ContextBrowser::collectionScanDone()
{
    if ( CollectionDB::instance()->isEmpty() )
    {
        showIntroduction();
        m_emptyDB = true;
    } else if ( m_emptyDB )
    {
        showHome();
        m_emptyDB = false;
    }
}


void ContextBrowser::renderView()
{
    m_dirtyHomePage = true;
    m_dirtyCurrentTrackPage = true;
    m_dirtyLyricsPage = true;

    // TODO: Show CurrentTrack or Lyric tab if they were selected
    if ( CollectionDB::instance()->isEmpty() )
    {
        showIntroduction();
    }
    else
    {
        showHome();
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::engineNewMetaData( const MetaBundle& bundle, bool /*trackChanged*/ )
{
    bool newMetaData = false;
    m_dirtyHomePage = true;
    m_dirtyCurrentTrackPage = true;

    // Add stream metadata history item to list
    if ( !m_metadataHistory.last().contains( bundle.prettyTitle() ) )
    {
        newMetaData = true;
        const QString timeString = QTime::currentTime().toString( "hh:mm" );
        m_metadataHistory << QString( "<td valign='top'>" + timeString + "&nbsp;</td><td align='left'>" + escapeHTML( bundle.prettyTitle() ) + "</td>" );
    }

    if ( currentPage() != m_currentTrackPage->view() || bundle.url() != m_currentURL || newMetaData )
        showCurrentTrack();
    else if ( CollectionDB::instance()->isEmpty() || !CollectionDB::instance()->isValid() )
        showIntroduction();
}


void ContextBrowser::engineStateChanged( Engine::State state )
{
    m_dirtyHomePage = true;
    m_dirtyCurrentTrackPage = true;
    m_dirtyLyricsPage = true;

    switch( state )
    {
        case Engine::Empty:
            m_metadataHistory.clear();
            showHome();
            blockSignals( true );
            setTabEnabled( m_currentTrackPage->view(), false );
            setTabEnabled( m_lyricsPage->view(), false );
            blockSignals( false );
            break;
        case Engine::Playing:
            m_metadataHistory.clear();
            blockSignals( true );
            setTabEnabled( m_currentTrackPage->view(), true );
            setTabEnabled( m_lyricsPage->view(), true );
            blockSignals( false );
            break;
        default:
            ;
    }
}


void ContextBrowser::saveHtmlData()
{
    QFile exportedDocument( amaroK::saveLocation() + "contextbrowser.html" );
    exportedDocument.open(IO_WriteOnly);
    QTextStream stream( &exportedDocument );
    stream << m_HTMLSource // the pure html data..
        .replace("<html>",QString("<html><head><style type=\"text/css\">%1</style></head>").arg(m_styleSheet) ); // and the stylesheet code
    exportedDocument.close();
}


void ContextBrowser::paletteChange( const QPalette& pal )
{
    QTabWidget::paletteChange( pal );
    setStyleSheet();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::tabChanged( QWidget *page )
{
    setFocusProxy( page ); //so focus is given to a sensible widget when the tab is opened
    if ( m_dirtyHomePage && ( page == m_homePage->view() ) )
        showHome();
    else if ( m_dirtyCurrentTrackPage && ( page == m_currentTrackPage->view() ) )
        showCurrentTrack();
    else if ( m_dirtyLyricsPage && ( page == m_lyricsPage->view() ) )
        showLyrics();
}


void ContextBrowser::slotContextMenu( const QString& urlString, const QPoint& point )
{
    enum { SHOW, FETCH, CUSTOM, DELETE, APPEND, ASNEXT, MAKE, MANAGER, TITLE };

    if( urlString.isEmpty() || urlString.startsWith( "musicbrainz" ) || urlString.startsWith( "lyricspage" ) )
        return;

    KURL url( urlString );
    if( url.path().contains( "lyric", FALSE ) )
        return;

    KPopupMenu menu;
    KURL::List urls( url );
    QString artist, album;
    albumArtistFromUrl( url.path(), artist, album );

    if ( url.protocol() == "fetchcover" )
    {
        menu.insertTitle( i18n( "Cover Image" ) );

        menu.insertItem( SmallIconSet( "viewmag" ), i18n( "&Show Fullsize" ), SHOW );
        menu.insertItem( SmallIconSet( "www" ), i18n( "&Fetch From amazon.%1" ).arg(AmarokConfig::amazonLocale()), FETCH );
        menu.insertItem( SmallIconSet( "folder_image" ), i18n( "Set &Custom Cover" ), CUSTOM );
        menu.insertSeparator();

        menu.insertItem( SmallIconSet( "editdelete" ), i18n("&Unset Cover"), DELETE );
        menu.insertSeparator();
        menu.insertItem( QPixmap( locate( "data", "amarok/images/covermanager.png" ) ), i18n( "Cover Manager" ), MANAGER );

        #ifndef AMAZON_SUPPORT
        menu.setItemEnabled( FETCH, false );
        #endif
        menu.setItemEnabled( SHOW, !CollectionDB::instance()->albumImage( artist, album, 0 ).contains( "nocover" ) );
    }
    else {
        //TODO it would be handy and more usable to have this menu under the cover one too

        menu.insertTitle( i18n("Track"), TITLE );

        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( SmallIconSet( "player_playlist_2" ), i18n( "&Make Playlist" ), MAKE );
        menu.insertItem( SmallIconSet( "2rightarrow" ), i18n( "&Queue After Current Track" ), ASNEXT );

        if ( url.protocol() == "album" )
        {
            QString sql = "select distinct url from tags where artist = '%1' and album = '%2' order by track;";
            QStringList values = CollectionDB::instance()->query( sql.arg( artist, album ) );

            urls.clear(); //remove urlString
            KURL url;
            for( QStringList::ConstIterator it = values.begin(); it != values.end(); ++it )
            {
                url.setPath( *it );
                urls.append( url );
            }

            menu.changeTitle( TITLE, i18n("Album") );
        }
    }

    //Not all these are used in the menu, it depends on the context
    switch( menu.exec( point ) )
    {
    case SHOW:
        CoverManager::viewCover( artist, album, this );
        break;

    case DELETE:
    {
        const int button = KMessageBox::warningContinueCancel( this,
            i18n( "Are you sure you want to remove this cover from the Collection?" ),
            QString::null,
            i18n("&Remove") );

        if ( button == KMessageBox::Continue )
        {
            CollectionDB::instance()->removeAlbumImage( artist, album );
            showCurrentTrack();
        }
        break;
    }

    case ASNEXT:
        Playlist::instance()->insertMedia( urls, Playlist::Queue );
        break;

    case MAKE:
        Playlist::instance()->clear();

        //FALL_THROUGH

    case APPEND:
        Playlist::instance()->insertMedia( urls, Playlist::Unique );
        break;

    case FETCH:
    #ifdef AMAZON_SUPPORT
        CollectionDB::instance()->fetchCover( this, artist, album, false );
        break;
    #endif

    case CUSTOM:
    {
        QString artist_id; artist_id.setNum( CollectionDB::instance()->artistID( artist ) );
        QString album_id; album_id.setNum( CollectionDB::instance()->albumID( album ) );
        QStringList values = CollectionDB::instance()->albumTracks( artist_id, album_id );
        QString startPath = ":homedir";

        if ( !values.isEmpty() ) {
            KURL url;
            url.setPath( values.first() );
            startPath = url.directory();
        }

        KURL file = KFileDialog::getImageOpenURL( startPath, this, i18n( "Select Cover Image File" ) );
        if ( !file.isEmpty() ) {
            CollectionDB::instance()->setAlbumImage( artist, album, file );
            m_dirtyCurrentTrackPage = true;
            showCurrentTrack();
        }
        break;
    }

    case MANAGER:
        CoverManager::showOnce( album );
        break;
    }
}


static QString
verboseTimeSince( const QDateTime &datetime )
{
    const QDate date = datetime.date();
    const QDate now  = QDate::currentDate();
    if( date > now )
        return i18n( "The future" );

    if( date.year() == now.year() ) {
        if( date.month() == now.month() ) {
            if( date.day() == now.day() ) {
                //the date is today, let's get more resolution
                const QTime time = datetime.time();
                const QTime now  = QTime::currentTime();
                if( time > now )
                    return i18n( "The future" );

                int
                minutes  = now.hour() - time.hour();
                minutes *= 60;
                minutes += QABS(now.minute() - time.minute());

                if( minutes < 90 )
                    return i18n( "Within the last minute", "%n minutes ago", minutes == 0 ? 1 : minutes );
                else
                    return i18n( "Within the last hour", "%n hours ago", now.hour() - time.hour() );
            }
            else
                /*continue to days calculation section*/;
        }

        const int days = date.daysTo( now );

        if ( days > 28 )
            return i18n( "Last month", "%n months ago", now.month() - date.month() );
        else if( days < 7 )
            return i18n( "Yesterday", "%n days ago", days );
        else
            return i18n( "Last week", "%n weeks ago", days / 7 );
    }

    // it was played last year, but that could still be yesterday
    //TODO it could still be yesterday, this whole function needs adaption

    if( now.year() - date.year() == 1 && QABS(now.month() - date.month()) < 7 )
        return i18n( "Last month", "%n months ago", now.month() - date.month() );
    else
        return i18n( "Last year", "%n years ago", now.year() - date.year() );
}


void ContextBrowser::showHome() //SLOT
{
    if ( currentPage() != m_homePage->view() )
    {
        blockSignals( true );
        showPage( m_homePage->view() );
        blockSignals( false );
    }

    // Do we have to rebuild the page?
    if ( !m_dirtyHomePage ) return;

    QStringList fave = CollectionDB::instance()->query(
        "SELECT tags.title, tags.url, round( statistics.percentage + 0.4 ), artist.name, album.name "
        "FROM tags, artist, album, statistics "
        "WHERE artist.id = tags.artist AND album.id = tags.album AND statistics.url = tags.url "
        "ORDER BY statistics.percentage DESC "
        "LIMIT 0,10;" );

    QStringList recent = CollectionDB::instance()->query(
        "SELECT tags.title, tags.url, artist.name, album.name "
        "FROM tags, artist, album "
        "WHERE artist.id = tags.artist AND album.id = tags.album "
        "ORDER BY tags.createdate DESC "
        "LIMIT 0,5;" );

    QStringList least = CollectionDB::instance()->query(
        "SELECT tags.title, tags.url, artist.name, album.name, statistics.accessdate "
        "FROM tags, artist, album, statistics "
        "WHERE artist.id = tags.artist AND album.id = tags.album AND tags.url = statistics.url "
        "ORDER BY statistics.accessdate "
        "LIMIT 0,5;" );

    m_homePage->begin();
    m_HTMLSource="";
    m_homePage->setUserStyleSheet( m_styleSheet );

    // <Favorite Tracks Information>
    m_HTMLSource.append(
            "<html>"
            "<div id='favorites_box' class='box'>"
                "<div id='favorites_box-header' class='box-header'>"
                    "<span id='favorites_box-header-title' class='box-header-title'>"
                    + i18n( "Your Favorite Tracks" ) +
                    "</span>"
                "</div>"
                "<div id='favorites_box-body' class='box-body'>"
                       );

    for( uint i = 0; i < fave.count(); i = i + 5 )
    {
        m_HTMLSource.append(
                    "<div class='" + QString( (i % 10) ? "box-row-alt" : "box-row" ) + "'>"
                        "<div class='song'>"
                            "<a href=\"file:" + fave[i+1].replace( '"', QCString( "%22" ) ) + "\">"
                            "<span class='song-title'>" + fave[i] + "</span> "
                            "<span class='song-score'>(" + i18n( "Score: %1" ).arg( fave[i+2] ) + ")</span><br />"
                            "<span class='song-artist'>" + fave[i+3] + "</span>"
                           );

        if ( !fave[i+4].isEmpty() )
            m_HTMLSource.append(
                                "<span class='song-separator'> - </span>"
                                "<span class='song-album'>"+ fave[i+4] +"</span>"
                               );

        m_HTMLSource.append(
                            "</a>"
                        "</div>"
                    "</div>"
                           );
    }
    m_HTMLSource.append(
                "</div>"
            "</div>"

    // </Favorite Tracks Information>

    // <Recent Tracks Information>

            "<div id='newest_box' class='box'>"
                "<div id='newest_box-header' class='box-header'>"
                    "<span id='newest_box-header-title' class='box-header-title'>"
                    + i18n( "Your Newest Tracks" ) +
                    "</span>"
                "</div>"
                "<div id='newest_box-body' class='box-body'>" );

    for( uint i = 0; i < recent.count(); i = i + 4 )
    {
        m_HTMLSource.append(
                    "<div class='" + QString( (i % 8) ? "box-row-alt" : "box-row" ) + "'>"
                        "<div class='song'>"
                            "<a href=\"file:" + recent[i+1].replace( '"', QCString( "%22" ) ) + "\">"
                            "<span class='song-title'>" + recent[i] + "</span><br />"
                            "<span class='song-artist'>" + recent[i+2] + "</span>"
                           );

        if ( !recent[i+3].isEmpty() )
            m_HTMLSource.append(
                                "<span class='song-separator'> - </span>"
                                "<span class='song-album'>" + recent[i+3] + "</span>"
                               );

        m_HTMLSource.append(
                            "</a>"
                        "</div>"
                    "</div>"
                           );
    }

    m_HTMLSource.append(
                "</div>"
            "</div>"

    // </Recent Tracks Information>

    // <Songs least listened Information>

            "<div id='least_box' class='box'>"
                "<div id='least_box-header' class='box-header'>"
                    "<span id='least_box-header-title' class='box-header-title'>"
                    + i18n( "Least Played Tracks" ) +
                    "</span>"
                "</div>"
                "<div id='least_box-body' class='box-body'>" );

    QDateTime lastPlay = QDateTime();
    for( uint i = 0; i < least.count(); i = i + 5 )
    {
        lastPlay.setTime_t( least[i+4].toUInt() );
        m_HTMLSource.append(
                    "<div class='" + QString( (i % 8) ? "box-row-alt" : "box-row" ) + "'>"
                        "<div class='song'>"
                            "<a href=\"file:" + least[i+1].replace( '"', QCString( "%22" ) ) + "\">"
                            "<span class='song-title'>" + least[i] + "</span><br />"
                            "<span class='song-artist'>" + least[i+2] + "</span>"
                           );

        if ( !least[i+3].isEmpty() )
            m_HTMLSource.append(
                                "<span class='song-separator'> - </span>"
                                "<span class='song-album'>" + least[i+3] + "</span>"
                               );

        m_HTMLSource.append(
                            "<br /><span class='song-time'>" + i18n( "Last played: %1" ).arg( verboseTimeSince( lastPlay ) ) + "</span>"
                            "</a>"
                        "</div>"
                    "</div>"
                           );
    }

    m_HTMLSource.append(
                "</div>"
            "</div>"
            "</html>"
                       );

    // </Songs least listened Information>

    m_homePage->write( m_HTMLSource );
    m_homePage->end();
    m_dirtyHomePage = false;
    saveHtmlData(); // Send html code to file
}


void ContextBrowser::showCurrentTrack() //SLOT
{
    if ( currentPage() != m_currentTrackPage->view() )
    {
        blockSignals( true );
        showPage( m_currentTrackPage->view() );
        blockSignals( false );
    }

    // Do we have to rebuild the page?
    if ( !m_dirtyCurrentTrackPage ) return;

    const MetaBundle &currentTrack = EngineController::instance()->bundle();
    m_currentURL = EngineController::instance()->bundle().url();

    m_currentTrackPage->begin();
    m_HTMLSource="";
    m_currentTrackPage->setUserStyleSheet( m_styleSheet );

    m_HTMLSource.append( "<html>"
                    "<script type='text/javascript'>"
                      //Toggle visibility of a block. NOTE: if the block ID starts with the T
                      //letter, 'Table' display will be used instead of the 'Block' one.
                      "function toggleBlock(ID) {"
                        "if ( document.getElementById(ID).style.display != 'none' ) {"
                          "document.getElementById(ID).style.display = 'none';"
                        "} else {"
                          "if ( ID[0] != 'T' ) {"
                            "document.getElementById(ID).style.display = 'block';"
                          "} else {"
                            "document.getElementById(ID).style.display = 'table';"
                          "}"
                        "}"
                      "}"
                    "</script>" );

    if ( EngineController::engine()->isStream() )
    {
        m_HTMLSource.append( QStringx(
                "<div id='current_box' class='box'>"
                    "<div id='current_box-header' class='box-header'>"
                        "<span id='current_box-header-stream' class='box-header-title'>%1</span> "
                    "</div>"
                    "<table class='box-body' width='100%' border='0' cellspacing='0' cellpadding='1'>"
                        "<tr class='box-row'>"
                            "<td height='42' valign='top' width='90%'>"
                                "<b>%2</b>"
                                "<br />"
                                "<br />"
                                "%3"
                                "<br />"
                                "<br />"
                                "%4"
                                "<br />"
                                "%5"
                                "<br />"
                                "%6"
                                "<br />"
                                "%7"
                            "</td>"
                        "</tr>"
                    "</table>"
                "</div>" )
            .args( QStringList()
                << i18n( "Stream Details" )
                << escapeHTML( currentTrack.prettyTitle() )
                << escapeHTML( currentTrack.streamName() )
                << escapeHTML( currentTrack.genre() )
                << escapeHTML( currentTrack.prettyBitrate() )
                << escapeHTML( currentTrack.streamUrl() )
                << escapeHTML( currentTrack.prettyURL() ) ) );

        if ( m_metadataHistory.count() > 2 )
        {
            m_HTMLSource.append(
                "<div class='box'>"
                 "<div class='box-header'>" + i18n( "Metadata History" ) + "</div>"
                 "<table class='box-body' width='100%' border='0' cellspacing='0' cellpadding='1'>" );

            QStringList::const_iterator it;
            // Ignore first two items, as they don't belong in the history
            for ( it = m_metadataHistory.at( 2 ); it != m_metadataHistory.end(); ++it )
            {
                m_HTMLSource.append( QStringx( "<tr class='box-row'><td>%1</td></tr>" ).arg( *it ) );
            }

            m_HTMLSource.append(
                 "</table>"
                "</div>" );
        }

        m_HTMLSource.append("</html>" );
        m_currentTrackPage->write( m_HTMLSource );
        m_currentTrackPage->end();
        saveHtmlData(); // Send html code to file
        return;
    }

    const uint artist_id = CollectionDB::instance()->artistID( currentTrack.artist() );
    const uint album_id  = CollectionDB::instance()->albumID ( currentTrack.album() );

    // <Current Track Information>
    QStringList values = CollectionDB::instance()->query( QString(
        "SELECT statistics.createdate, statistics.accessdate, "
        "statistics.playcounter, round( statistics.percentage + 0.4 ) "
        "FROM  statistics "
        "WHERE url = '%1';" )
            .arg( CollectionDB::instance()->escapeString( currentTrack.url().path() ) ) );

    //making 2 tables is most probably not the cleanest way to do it, but it works.
    m_HTMLSource.append( QStringx(
        "<div id='current_box' class='box'>"
            "<div id='current_box-header' class='box-header'>"
                "<span id='current_box-header-songname' class='box-header-title'>%1</span> "
                "<span id='current_box-header-separator' class='box-header-title'>-</span> "
                "<span id='current_box-header-artist' class='box-header-title'>%2</span>"
                "<br />"
                "<span id='current_box-header-album' class='box-header-title'>%3</span>"
            "</div>"
            "<table id='current_box-table' class='box-body' width='100%' cellpadding='0' cellspacing='0'>"
                "<tr>"
                    "<td id='current_box-largecover-td'>"
                        "<a id='current_box-largecover-a' href='fetchcover:%4 @@@ %5'>"
                            "<img id='current_box-largecover-image' src='%6' title='%7'>"
                        "</a>"
                    "</td>"
                    "<td id='current_box-information-td' align='right'>"
                        "<div id='musicbrainz-div'>"
                            "<a id='musicbrainz-a' title='%8' href='musicbrainz:%9 @@@ %10'>"
                            "<img id='musicbrainz-image' src='%11' />"
                            "</a>"
                        "</div>"
                                 )
        .args( QStringList()
            << escapeHTML( currentTrack.title() )
            << escapeHTML( currentTrack.artist() )
            << escapeHTML( currentTrack.album() )
            << escapeHTMLAttr( currentTrack.artist() )
            << escapeHTMLAttr( currentTrack.album() )
            << escapeHTMLAttr( CollectionDB::instance()->albumImage( currentTrack ) )
            << i18n( "Click for information from amazon.%1, right-click for menu." ).arg( AmarokConfig::amazonLocale() )
            << i18n( "Look up this track at musicbrainz.com" )
            << escapeHTMLAttr( currentTrack.artist() )
            << escapeHTMLAttr( currentTrack.album() )
            << escapeHTML( locate( "data", "amarok/images/musicbrainz.png" ) ) ) );

    if ( !values.isEmpty() )
    {
        QDateTime firstPlay = QDateTime();
        firstPlay.setTime_t( values[0].toUInt() );
        QDateTime lastPlay = QDateTime();
        lastPlay.setTime_t( values[1].toUInt() );

        const uint playtimes = values[2].toInt();
        const uint score = values[3].toInt();

        const QString scoreBox =
                "<table class='scoreBox' border='0' cellspacing='0' cellpadding='0' title='" + i18n("Score") + " %2'>"
                    "<tr>"
                        "<td nowrap>%1&nbsp;</td>"
                            "<td>"
                            "<div class='sbouter'>"
                                "<div class='sbinner' style='width: %3px;'></div>"
                            "</div>"
                        "</td>"
                    "</tr>"
                "</table>";

        //SAFE   = .arg( x, y )
        //UNSAFE = .arg( x ).arg( y )
        m_HTMLSource.append( QString(
                "<span>%1</span><br />"
                "%2"
                "<span>%3</span><br />"
                "<span>%4</span>"
                                    )
            .arg( i18n( "Track played once", "Track played %n times", playtimes ),
                  scoreBox.arg( score ).arg( score ).arg( score / 2 ),
                  i18n( "Last played: %1" ).arg( verboseTimeSince( lastPlay ) ),
                  i18n( "First played: %1" ).arg( verboseTimeSince( firstPlay ) ) ) );
   }
   else
        m_HTMLSource.append( i18n( "Never played before" ) );

    m_HTMLSource.append(
                    "</td>"
                "</tr>"
            "</table>"
        "</div>" );
    // </Current Track Information>

    if ( !CollectionDB::instance()->isFileInCollection( currentTrack.url().path() ) )
    {
        m_HTMLSource.append(
        "<div id='notindb_box' class='box'>"
            "<div id='notindb_box-header' class='box-header'>"
                "<span id='notindb_box-header-title' class='box-header-title'>"
                + i18n( "This file is not in your Collection!" ) +
                "</span>"
            "</div>"
            "<div id='notindb_box-body' class='box-body'>"
                "<p>"
                + i18n( "If you would like to see contextual information about this track,"
                        " you should add it to your Collection." ) +
                "</p>"
                "<a href='show:collectionSetup' class='button'>"
                + i18n( "Change Collection Setup" ) +
                "</a>"
            "</div>"
        "</div>"
                           );
    }

    // <Suggested Songs>
    QStringList relArtists;
    relArtists = CollectionDB::instance()->similarArtists( currentTrack.artist(), 10 );
    if ( !relArtists.isEmpty() )
    {
        QString token;
        QueryBuilder qb;

        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
        qb.addMatches( QueryBuilder::tabArtist, relArtists );
        qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valScore, true );
        qb.setLimit( 0, 5 );
        values = qb.run();

        // not enough items returned, let's fill the list with score-less tracks
        if ( values.count() < 8 * qb.countReturnValues() )
        {
            qb.clear();
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
            qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
            qb.addMatches( QueryBuilder::tabArtist, relArtists );
            qb.setOptions( QueryBuilder::optRandomize );
            qb.setLimit( 0, 8 - values.count() / 4 );

            QStringList sl;
            sl = qb.run();
            for ( uint i = 0; i < sl.count(); i += qb.countReturnValues() )
            {
                values << sl[i];
                values << sl[i + 1];
                values << sl[i + 2];
                values << "0";
            }
        }

        if ( !values.isEmpty() )
        {
            m_HTMLSource.append(
            "<div id='suggested_box' class='box'>"
                "<div id='suggested_box-header' class='box-header' onClick=\"toggleBlock('T_SS')\" style='cursor: pointer;'>"
                    "<span id='suggested_box-header-title' class='box-header-title'>"
                    + i18n( "Suggested Songs" ) +
                    "</span>"
                "</div>"
                "<table class='box-body' id='T_SS' width='100%' border='0' cellspacing='0' cellpadding='1'>"
                               );

            for ( uint i = 0; i < values.count(); i += 4 )
                m_HTMLSource.append(
                    "<tr class='" + QString( (i % 8) ? "box-row-alt" : "box-row" ) + "'>"
                        "<td class='song'>"
                            "<a href=\"file:" + values[i].replace( '"', QCString( "%22" ) ) + "\">"
                            "<span class='album-song-title'>"+ values[i + 2] + "</span>"
                            "<span class='song-separator'> - </span>"
                            "<span class='album-song-title'>" + values[i + 1] + "</span>"
                            "</a>"
                        "</td>"
                        "<td class='sbtext' width='1'>" + values[i + 3] + "</td>"
                        "<td width='1' title='" + i18n( "Score" ) + "'>"
                            "<div class='sbouter'>"
                                "<div class='sbinner' style='width: " + QString::number( values[i + 3].toInt() / 2 ) + "px;'></div>"
                            "</div>"
                        "</td>"
                    "</tr>"
                                   );

            m_HTMLSource.append(
                 "</table>"
                "</div>" );
        }
    }
    // </Suggested Songs>

    // <Favourite Tracks Information>
    QString artistName = currentTrack.artist().isEmpty() ? i18n( "This Artist" ) : escapeHTML( currentTrack.artist() );
    values = CollectionDB::instance()->query( QString( "SELECT tags.title, tags.url, round( statistics.percentage + 0.4 ) "
                                   "FROM tags, statistics "
                                   "WHERE tags.artist = %1 AND statistics.url = tags.url "
                                   "ORDER BY statistics.percentage DESC "
                                   "LIMIT 0,5;" )
                          .arg( artist_id ) );

    if ( !values.isEmpty() )
    {
        m_HTMLSource.append(
        "<div id='favoritesby_box' class='box'>"
            "<div id='favoritesby-header' class='box-header' onClick=\"toggleBlock('T_FT')\" style='cursor: pointer;'>"
                "<span id='favoritesby_box-header-title' class='box-header-title'>"
                + i18n( "Favorite Tracks By %1" ).arg( artistName ) +
                "</span>"
            "</div>"
            "<table class='box-body' id='T_FT' width='100%' border='0' cellspacing='0' cellpadding='1'>" );

        for ( uint i = 0; i < values.count(); i += 3 )
            m_HTMLSource.append(
                "<tr class='" + QString( (i % 6) ? "box-row-alt" : "box-row" ) + "'>"
                    "<td class='song'>"
                        "<a href=\"file:" + values[i + 1].replace( '"', QCString( "%22" ) ) + "\">"
                        "<span class='album-song-title'>" + values[i] + "</span>"
                        "</a>"
                    "</td>"
                    "<td class='sbtext' width='1'>" + values[i + 2] + "</td>"
                    "<td width='1' title='" + i18n( "Score" ) + "'>"
                        "<div class='sbouter'>"
                            "<div class='sbinner' style='width: " + QString::number( values[i + 2].toInt() / 2 ) + "px;'></div>"
                        "</div>"
                    "</td>"
                "</tr>"
                               );

        m_HTMLSource.append(
            "</table>"
            "</div>"
                           );
    }
    // </Favourite Tracks Information>

    // <Albums by this artist>
    values = CollectionDB::instance()->query( QString( "SELECT DISTINCT album.name, album.id "
                                   "FROM tags, album, year "
                                   "WHERE album.id = tags.album AND year.id = tags.year AND tags.artist = %1 AND album.name <> '' "
                                   "ORDER BY (year.name+0) DESC, album.name;" )
                          .arg( artist_id ) );

    if ( !values.isEmpty() )
    {
        // write the script to toggle blocks visibility
        m_HTMLSource.append(
        "<div id='albums_box' class='box'>"
            "<div id='albums_box-header' class='box-header'>"
                "<span id='albums_box-header-title' class='box-header-title'>"
                + i18n( "Albums By %1" ).arg( artistName ) +
                "</span>"
            "</div>"
            "<table id='albums_box-body' class='box-body' width='100%' border='0' cellspacing='0' cellpadding='0'>" );

        uint vectorPlace = 0;
	// find album of the current track (if it exists)
        while ( vectorPlace < values.count() && values[ vectorPlace+1 ] != QString::number( album_id ) )
            vectorPlace += 2;
        for ( uint i = 0; i < values.count(); i += 2 )
        {
            QStringList albumValues = CollectionDB::instance()->query( QString(
                "SELECT tags.title, tags.url, tags.track, year.name, tags.length "
                "FROM tags, year "
                "WHERE tags.album = %1 AND "
                "( tags.sampler = 1 OR tags.artist = %2 ) "
                "AND year.id = tags.year "
                "ORDER BY tags.track;" ).arg( values[ i+1 ] ).arg( artist_id ) );

            QString albumYear;
            if ( !albumValues.isEmpty() )
            {
                albumYear = albumValues[ 3 ];
                for ( uint j = 0; j < albumValues.count(); j += 5 )
                {
                    if ( albumValues[j + 3] != albumYear || albumYear == "0" )
                    {
                        albumYear = QString::null;
                        break;
                    }
                }
            }

            m_HTMLSource.append( QStringx (
            "<tr class='" + QString( (i % 4) ? "box-row-alt" : "box-row" ) + "'>"
                "<td>"
                    "<div class='album-header' onClick=\"toggleBlock('IDA%1')\">"
                    "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
                    "<tr>"
                        "<td width='1'>"
                            "<a href='fetchcover:%2 @@@ %3'>"
                            "<img class='album-image' align='left' vspace='2' hspace='2' title='%4' src='%5'/>"
                            "</a>"
                        "</td>"
                        "<td valign='middle' align='left'>"
                            "<span class='album-info'>%6</span> "
                            "<a href='album:%7 @@@ %8'><span class='album-title'>%9</span></a>"
                            "<br />"
                            "<span class='album-year'>%10</span>"
                        "</td>"
                    "</tr>"
                    "</table>"
                    "</div>"
                    "<div class='album-body' style='display:%11;' id='IDA%12'>" )
                .args( QStringList()
                    << values[ i+1 ]
                    << escapeHTMLAttr( currentTrack.artist() ) // artist name
                    << escapeHTMLAttr( values[ i ] ) // album.name
                    << i18n( "Click for information from amazon.com, right-click for menu." )
                    << escapeHTMLAttr( CollectionDB::instance()->albumImage( currentTrack.artist(), values[ i ], 50 ) )
                    << i18n( "Single", "%n Tracks",  albumValues.count() / 5 )
                    << QString::number( artist_id )
                    << values[ i+1 ] //album.id
                    << escapeHTML( values[ i ] )
                    << albumYear
                    << ( i!=vectorPlace ? "none" : "block" ) /* shows it if it's the current track album */
                    << values[ i+1 ] ) );

            if ( !albumValues.isEmpty() )
                for ( uint j = 0; j < albumValues.count(); j += 5 )
                {
                    QString tmp = albumValues[j + 2].stripWhiteSpace().isEmpty() ? "" : albumValues[j + 2];
                    if (tmp.length() > 0)
                    {
                        tmp = tmp.length() == 1 ? "<span class='album-song-trackno'>0"+ tmp + ".&nbsp;</span>" : "<span class='album-song-trackno'>"+ tmp + ".&nbsp;</span>";
                    }
                    QString tmp_time = (albumValues[j + 4] == QString("0")) ? "" :" <span class='album-song-time'>(" + MetaBundle::prettyTime( QString(albumValues[j + 4]).toInt(), false ) + ")</span>";
                    m_HTMLSource.append(
                        "<div class='album-song'>"
                            "<a href=\"file:" + albumValues[j + 1].replace( "\"", QCString( "%22" ) ) + "\">"
                            + tmp +
                            "<span class='album-song-title'>" + albumValues[j] + "</span>"
                            + tmp_time +
                            "</a>"
                        "</div>" );
                }

            m_HTMLSource.append(
                  "</div>"
                 "</td>"
                "</tr>" );
        }
        m_HTMLSource.append(
               "</table>"
              "</div>" );
    }
    // </Albums by this artist>

    m_HTMLSource.append( "</html>" );
    m_currentTrackPage->write( m_HTMLSource );
    m_currentTrackPage->end();
    m_dirtyCurrentTrackPage = false;
    saveHtmlData(); // Send html code to file
}


void ContextBrowser::setStyleSheet()
{
    DEBUG_FUNC_INFO

    QString themeName = AmarokConfig::contextBrowserStyleSheet().latin1();
    if ( QFile::exists( amaroK::saveLocation( "themes/" + themeName + '/' ) + "stylesheet.css" ) )
        setStyleSheet_ExternalStyle( m_styleSheet, themeName );
    else
        setStyleSheet_Default( m_styleSheet );

    m_homePage->setUserStyleSheet( m_styleSheet );
    m_currentTrackPage->setUserStyleSheet( m_styleSheet );
    m_lyricsPage->setUserStyleSheet( m_styleSheet );
}


void ContextBrowser::setStyleSheet_Default( QString& styleSheet )
{
    //colorscheme/font dependant parameters
    int pxSize = fontMetrics().height() - 4;
    const QString fontFamily = AmarokConfig::useCustomFonts() ? AmarokConfig::contextBrowserFont().family() : QApplication::font().family();
    const QString text = colorGroup().text().name();
    const QString fg   = colorGroup().highlightedText().name();
    const QString bg   = colorGroup().highlight().name();
    const QColor baseColor = colorGroup().base();
    const QColor bgColor = colorGroup().highlight();
    const amaroK::Color gradientColor = bgColor;

    delete m_bgGradientImage;
    delete m_headerGradientImage;
    delete m_shadowGradientImage;

    m_bgGradientImage = new KTempFile( locateLocal( "tmp", "gradient" ), ".png", 0600 );
    QImage image = KImageEffect::gradient( QSize( 600, 1 ), gradientColor, gradientColor.light( 130 ), KImageEffect::PipeCrossGradient );
    image.save( m_bgGradientImage->file(), "PNG" );
    m_bgGradientImage->close();

    m_headerGradientImage = new KTempFile( locateLocal( "tmp", "gradient_header" ), ".png", 0600 );
    QImage imageH = KImageEffect::unbalancedGradient( QSize( 1, 10 ), bgColor, gradientColor.light( 130 ), KImageEffect::VerticalGradient, 100, -100 );
    imageH.copy( 0, 1, 1, 9 ).save( m_headerGradientImage->file(), "PNG" );
    m_headerGradientImage->close();

    m_shadowGradientImage = new KTempFile( locateLocal( "tmp", "gradient_shadow" ), ".png", 0600 );
    QImage imageS = KImageEffect::unbalancedGradient( QSize( 1, 10 ), baseColor, Qt::gray, KImageEffect::VerticalGradient, 100, -100 );
    imageS.save( m_shadowGradientImage->file(), "PNG" );
    m_shadowGradientImage->close();

    //unlink the files for us on deletion
    m_bgGradientImage->setAutoDelete( true );
    m_headerGradientImage->setAutoDelete( true );
    m_shadowGradientImage->setAutoDelete( true );

    //we have to set the color for body due to a KHTML bug
    //KHTML sets the base color but not the text color
    styleSheet = QString( "body { margin: 8px; font-size: %1px; color: %2; background-color: %3; background-image: url( %4 ); background-repeat: repeat; font-family: %5; }" )
            .arg( pxSize )
            .arg( text )
            .arg( AmarokConfig::schemeAmarok() ? fg : gradientColor.name() )
            .arg( m_bgGradientImage->name() )
            .arg( fontFamily );

    //text attributes
    styleSheet += QString( "a { font-size: %1px; color: %2; }" ).arg( pxSize ).arg( text );
    styleSheet += QString( ".song a { display: block; padding: 1px 2px; font-weight: normal; text-decoration: none; }" );
    styleSheet += QString( ".song a:hover { color: %1; background-color: %2; }" ).arg( fg ).arg( bg );
    styleSheet += QString( ".song-title { font-weight: bold; }" );
    styleSheet += QString( ".song-score { }" );
    styleSheet += QString( ".song-artist { }" );
    styleSheet += QString( ".song-album { }" );
    styleSheet += QString( ".song-time { } " );

    //box: the base container for every block (border hilighted on hover, 'A' without underlining)
    styleSheet += QString( ".box { border: solid %1 1px; text-align: left; margin-bottom: 10px; }" ).arg( bg );
    styleSheet += QString( ".box a { text-decoration: none; }" );
    styleSheet += QString( ".box:hover { border: solid %1 1px; }" ).arg( text );

    //box contents: header, body, rows and alternate-rows
    styleSheet += QString( ".box-header { color: %1; background-color: %2; background-image: url( %4 ); background-repeat: repeat-x; font-size: %3px; font-weight: bold; padding: 1px 0.5em; border-bottom: 1px solid #000; }" )
            .arg( fg )
            .arg( bg )
            .arg( pxSize + 2 )
            .arg( m_headerGradientImage->name() );
    styleSheet += QString( ".box-header:hover {}" );

    styleSheet += QString( ".box-body { padding: 2px; background-color: %1; background-image: url( %2 ); background-repeat: repeat-x; font-size:%3px; }" )
            .arg( colorGroup().base().name() )
            .arg( m_shadowGradientImage->name() )
            .arg( pxSize );
    styleSheet += QString( ".box-body:hover {}" );

    styleSheet += QString( ".box-row {}" );
    styleSheet += QString( ".box-row:hover {}" );
    styleSheet += QString( ".box-row-alt {}" );
    styleSheet += QString( ".box-row-alt:hover {}" );

    //"Albums by ..." related styles
    styleSheet += QString( ".album-header {}" );
    styleSheet += QString( ".album-header:hover { color: %1; background-color: %2; cursor: pointer; }" ).arg( fg ).arg( bg );
    styleSheet += QString( ".album-header:hover a { color: %1; }" ).arg( fg );
    styleSheet += QString( ".album-body { background-color: %1; border-bottom: solid %2 1px; border-top: solid %3 1px; }" ).arg( colorGroup().base().name() ).arg( bg ).arg( bg );
    styleSheet += QString( ".album-title { font-weight: bold; }" );
    styleSheet += QString( ".album-info { float:right; padding-right:4px; font-size: %1px }" ).arg( pxSize );
    styleSheet += QString( ".album-image { padding-right: 4px; }" );
    styleSheet += QString( ".album-year { }" );
    styleSheet += QString( ".album-song a { display: block; padding: 1px 2px; font-weight: normal; text-decoration: none; }" );
    styleSheet += QString( ".album-song a:hover { color: %1; background-color: %2; }" ).arg( fg ).arg( bg );
    styleSheet += QString( ".album-song-trackno { }" );
    styleSheet += QString( ".album-song-title { } " );
    styleSheet += QString( ".album-song-time { } " );

    styleSheet += QString( ".button { margin: 2px; padding: 2px; display: block; border: 1px solid %1; background-color: %2; }" ).arg( text ).arg( colorGroup().base().name() );
    styleSheet += QString( ".button:hover { border: 1px solid %1; background-color: %2; color: %3; }" ).arg( text ).arg( bg ).arg( colorGroup().base().name() );

    //boxes used to display score (sb: score box)
    styleSheet += QString( ".sbtext { padding: 0px 4px; border-left: solid %1 1px; }" ).arg( colorGroup().base().dark( 120 ).name() );
    styleSheet += QString( ".sbouter { width: 52px; height: 10px; background-color: %1; border: solid %2 1px; }" ).arg( colorGroup().base().dark( 120 ).name() ).arg( bg );
    styleSheet += QString( ".sbinner { height: 8px; background-color: %1; border: solid %2 1px; }" ).arg( bg ).arg( fg );

    styleSheet += QString( "#current_box-header-album { font-weight: normal; }" );
    styleSheet += QString( "#current_box-information-td { text-align: right; vertical-align: bottom; padding: 3px; }" );
    styleSheet += QString( "#current_box-largecover-td { text-align: left; width: 100px; padding: 0; vertical-align: bottom; }" );
    styleSheet += QString( "#current_box-largecover-image { padding: 4px; vertical-align: bottom; }" );
}


void ContextBrowser::setStyleSheet_ExternalStyle( QString& styleSheet, QString& themeName )
{
    //colorscheme/font dependant parameters
    const QString pxSize = QString::number( fontMetrics().height() - 4 );
    const QString fontFamily = AmarokConfig::useCustomFonts() ? AmarokConfig::contextBrowserFont().family() : QApplication::font().family();
    const QString text = colorGroup().text().name();
    const QString fg   = colorGroup().highlightedText().name();
    const QString bg   = colorGroup().highlight().name();
    const QString base   = colorGroup().base().name();
    const QColor bgColor = colorGroup().highlight();
    amaroK::Color gradientColor = bgColor;

    //we have to set the color for body due to a KHTML bug
    //KHTML sets the base color but not the text color
    styleSheet = QString( "body { margin: 8px; font-size: %1px; color: %2; background-color: %3; font-family: %4; }" )
            .arg( pxSize )
            .arg( text )
            .arg( AmarokConfig::schemeAmarok() ? fg : gradientColor.name() )
            .arg( fontFamily );

    QString CSSLocation = amaroK::saveLocation( "themes/" + themeName + '/' );

    QFile ExternalCSS( CSSLocation + "stylesheet.css" );
    if ( !ExternalCSS.open( IO_ReadOnly ) )
        return;

    QTextStream eCSSts( &ExternalCSS );
    QString tmpCSS = eCSSts.read();
    ExternalCSS.close();

    tmpCSS.replace( "./", CSSLocation );
    tmpCSS.replace( "AMAROK_FONTSIZE-2", pxSize );
    tmpCSS.replace( "AMAROK_FONTSIZE", pxSize );
    tmpCSS.replace( "AMAROK_FONTSIZE+2", pxSize );
    tmpCSS.replace( "AMAROK_FONTFAMILY", fontFamily );
    tmpCSS.replace( "AMAROK_TEXTCOLOR", text );
    tmpCSS.replace( "AMAROK_BGCOLOR", bg );
    tmpCSS.replace( "AMAROK_FGCOLOR", fg );
    tmpCSS.replace( "AMAROK_BASECOLOR", base );
    tmpCSS.replace( "AMAROK_DARKBASECOLOR", colorGroup().base().dark( 120 ).name() );
    tmpCSS.replace( "AMAROK_GRADIENTCOLOR", gradientColor.name() );

    styleSheet += tmpCSS;
}


void ContextBrowser::showIntroduction()
{
    if ( currentPage() != m_homePage->view() )
    {
        blockSignals( true );
        showPage( m_homePage->view() );
        blockSignals( false );
    }

    // Do we have to rebuild the page? I don't care
    m_homePage->begin();
    m_HTMLSource="";
    m_homePage->setUserStyleSheet( m_styleSheet );

    m_HTMLSource.append(
            "<html>"
            "<div id='introduction_box' class='box'>"
                "<div id='introduction_box-header' class='box-header'>"
                    "<span id='introduction_box-header-title' class='box-header-title'>"
                    + i18n( "Hello amaroK user!" ) +
                    "</span>"
                "</div>"
                "<div id='introduction_box-body' class='box-body'>"
                    "<p>" +
                    i18n( "This is the Context Browser: "
                          "it shows you contextual information about the currently playing track."
                          "In order to use this feature of amaroK, you need to build a Collection."
                        ) +
                    "</p>"
                    "<a href='show:collectionSetup' class='button'>" + i18n( "Build Collection..." ) + "</a>"
                "</div>"
            "</div>"
            "</html>"
                       );

    m_homePage->write( m_HTMLSource );
    m_homePage->end();
    saveHtmlData(); // Send html code to file
}


void ContextBrowser::showScanning()
{
    if ( currentPage() != m_homePage->view() )
    {
        blockSignals( true );
        showPage( m_homePage->view() );
        blockSignals( false );
    }

    // Do we have to rebuild the page? I don't care
    m_homePage->begin();
    m_HTMLSource="";
    m_homePage->setUserStyleSheet( m_styleSheet );

    m_HTMLSource.append(
            "<html>"
            "<div id='building_box' class='box'>"
                "<div id='building_box-header' class='box-header'>"
                    "<span id='building_box-header-title' class='box-header-title'>"
                    + i18n( "Building Collection Database..." ) +
                    "</span>"
                "</div>"
                "<div id='building_box-body' class='box-body'>"
                    "<p>" + i18n( "Building Collection Database..." ) + "</p>"
                "</div>"
            "</div>"
            "</html>"
                       );

    m_homePage->write( m_HTMLSource );
    m_homePage->end();
    saveHtmlData(); // Send html code to file
}


// THE FOLLOWING CODE IS COPYRIGHT BY
// Christian Muehlhaeuser, Seb Ruiz
// <chris at chris.de>, <seb100 at optusnet.com.au>
// If I'm violating any copyright or such
// please contact / sue me. Thanks.

void ContextBrowser::showLyrics( const QString &hash )
{
    if ( currentPage() != m_lyricsPage->view() )
    {
        blockSignals( true );
        showPage( m_homePage->view() );
        blockSignals( false );
    }

    if ( !m_dirtyLyricsPage ) return;

    //remove all matches to the regExp and the song production type.
    //NOTE: use i18n'd and english equivalents since they are very common int'lly.
    QString replaceMe = " \\([^}]*%1[^}]*\\)";
    QStringList production;
    production << i18n( "live" ) << i18n( "acoustic" ) << i18n( "cover" ) << i18n( "mix" )
               << i18n( "edit" ) << i18n( "medley" ) << i18n( "unplugged" ) << i18n( "bonus" )
               << QString( "live" ) << QString( "acoustic" ) << QString( "cover" ) << QString( "mix" )
               << QString( "edit" ) << QString( "medley" ) << QString( "unplugged" ) << QString( "bonus" );

    QString title  = EngineController::instance()->bundle().title();

    for ( uint x = 0; x < production.count(); ++x )
    {
        QRegExp re = replaceMe.arg( production[x] );
        re.setCaseSensitive( false );
        title.remove( re );
    }

    QString url;
    if ( !hash.isEmpty() )
        url = QString( "http://lyrc.com.ar/en/tema1en.php?hash=%1" )
                  .arg( hash );
    else
        url = QString( "http://lyrc.com.ar/en/tema1en.php?artist=%1&songname=%2" )
                .arg(
                KURL::encode_string_no_slash( EngineController::instance()->bundle().artist() ),
                KURL::encode_string_no_slash( title ) );


    debug() << "Using this url: " << url << endl;

    m_lyrics = QString::null;
    m_lyricUrl = QString( "grupo=%1&tema=%2&disco=%3&ano=%4" ).arg(
            KURL::encode_string_no_slash( EngineController::instance()->bundle().artist() ),
            KURL::encode_string_no_slash( title ),
            KURL::encode_string_no_slash( EngineController::instance()->bundle().album() ),
            KURL::encode_string_no_slash( EngineController::instance()->bundle().year() ) );

    KIO::TransferJob* job = KIO::get( url, false, false );

    connect( job, SIGNAL( result( KIO::Job* ) ),
             this,  SLOT( lyricsResult( KIO::Job* ) ) );
    connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
             this,  SLOT( lyricsData( KIO::Job*, const QByteArray& ) ) );
}


void
ContextBrowser::lyricsData( KIO::Job*, const QByteArray& data ) //SLOT
{
    // Append new chunk of string
    m_lyrics += QString( data );
}


void
ContextBrowser::lyricsResult( KIO::Job* job ) //SLOT
{
    if ( !job->error() == 0 )
    {
        kdWarning() << "[LyricsFetcher] KIO error! errno: " << job->error() << endl;
        return;
    }

    if ( m_lyrics.find( "<font size='2'>" ) != -1 )
    {
        m_lyrics = m_lyrics.mid( m_lyrics.find( "<font size='2'>" ) );
        if ( m_lyrics.find( "<p><hr" ) != -1 )
            m_lyrics = m_lyrics.mid( 0, m_lyrics.find( "<p><hr" ) );
        else
            m_lyrics = m_lyrics.mid( 0, m_lyrics.find( "<br /><br />" ) );
    }
    else if ( m_lyrics.find( "Suggestions : " ) != -1 )
    {
        m_lyrics = m_lyrics.mid( m_lyrics.find( "Suggestions : " ), m_lyrics.find( "<br /><br />" ) );
        showLyricSuggestions();
    }
    else
    {
        m_lyrics = i18n( "Lyrics not found." );
        m_lyrics += QString( "<div id='lyrics_box_addlyrics'><a href='lyricspage:" + m_lyricUrl + "' class='button'>" + i18n("Add Lyrics") + "</a></div>" );
    }


    m_lyricsPage->begin();
    m_HTMLSource="";
    m_lyricsPage->setUserStyleSheet( m_styleSheet );

    m_HTMLSource.append(
            "<html>"
            "<div id='lyrics_box' class='box'>"
                "<div id='lyrics_box-header' class='box-header'>"
                    "<span id='lyrics_box-header-title' class='box-header-title'>"
                    + i18n( "Lyrics" ) +
                    "</span>"
                "</div>"
                "<div id='lyrics_box-body' class='box-body'>"
                    + m_lyrics +
                "</div>"
            "</div>"
            "</html>"
                       );
    m_lyricsPage->write( m_HTMLSource );
    m_lyricsPage->end();
    m_dirtyLyricsPage = false;
    saveHtmlData(); // Send html code to file
}


void
ContextBrowser::showLyricSuggestions()
{
    m_lyricHashes.clear();
    m_lyricSuggestions.clear();

    m_lyrics.replace( QString( "<font color='white'>" ), QString::null );
    m_lyrics.replace( QString( "</font>" ), QString::null );
    m_lyrics.replace( QString( "<br /><br />" ), QString::null );

    while ( !m_lyrics.isEmpty() )
    {
        m_lyrics = m_lyrics.mid( m_lyrics.find( "hash=" ) );
        m_lyricHashes << m_lyrics.mid( 5, m_lyrics.find( ">" ) - 6 );
        m_lyrics = m_lyrics.mid( m_lyrics.find( ">" ) );
        m_lyricSuggestions << m_lyrics.mid( 1, m_lyrics.find( "</a>" ) - 1 );
    }
    m_lyrics = i18n( "Lyrics for track not found, here are some suggestions:" ) + QString("<br /><br />");

    for ( uint i=0; i < m_lyricHashes.count() - 1; ++i )
    {
        m_lyrics += QString( "<a href='show:suggestLyric-%1'>" ).arg( m_lyricHashes[i] );
        m_lyrics += QString( "%1</a><br />" ).arg( m_lyricSuggestions[i] );
    }
    m_lyrics += QString( "<div id='lyrics_box_addlyrics'><a href='lyricspage:" + m_lyricUrl + "' class='button'>" + i18n("Add Lyrics") + "</a></div>" );


}


void
ContextBrowser::coverFetched( const QString &artist, const QString &album )
{
    const MetaBundle &currentTrack = EngineController::instance()->bundle();

    if ( currentTrack.artist() == artist ||
         currentTrack.album() == album ) // this is for compilations or artist == ""
    {
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
    }
}


void
ContextBrowser::coverRemoved( const QString &artist, const QString &album )
{
    const MetaBundle &currentTrack = EngineController::instance()->bundle();

    if ( currentTrack.artist() == artist ||
         currentTrack.album() == album ) // this is for compilations or artist == ""
    {
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
    }
}


void
ContextBrowser::similarArtistsFetched( const QString &artist )
{
    const MetaBundle &currentTrack = EngineController::instance()->bundle();

    if ( currentTrack.artist() == artist )
    {
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
    }
}


#include "contextbrowser.moc"
