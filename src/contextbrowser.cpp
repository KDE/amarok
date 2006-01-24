// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Reigo Reinmets <xatax@hot.ee>
// (c) 2005 Mark Kretschmann <markey@web.de>
// License: GNU General Public License V2


#define DEBUG_PREFIX "ContextBrowser"

#include "amarok.h"
#include "amarokconfig.h"
#include "browserToolBar.h"
#include "debug.h"
#include "collectiondb.h"
#include "collectionbrowser.h"
#include "colorgenerator.h"
#include "config.h"        //for AMAZON_SUPPORT
#include "contextbrowser.h"
#include "coverfetcher.h"
#include "covermanager.h"
#include "cuefile.h"
#include "enginecontroller.h"
#include "mediabrowser.h"
#include "metabundle.h"
#include "playlist.h"      //appendMedia()
#include "qstringx.h"
#include "statusbar.h"
#include "tagdialog.h"
#include "threadweaver.h"

#include <qdatetime.h>
#include <qfile.h> // External CSS opening
#include <qimage.h>
#include <qregexp.h>
#include <qtextstream.h>  // External CSS reading
#include <qvbox.h> //wiki tab

#include <kapplication.h> //kapp
#include <kcalendarsystem.h>  // for verboseTimeSince()
#include <kfiledialog.h>
#include <kglobal.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kiconloader.h>
#include <kimageeffect.h> // gradient background image
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <klocale.h>
#include <kmdcodec.h> // for data: URLs
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h> //locate file
#include <ktempfile.h>
#include <ktoolbarbutton.h>
#include <kurl.h>

#define escapeHTML(s)     QString(s).replace( "&", "&amp;" ).replace( "<", "&lt;" ).replace( ">", "&gt;" )
// .replace( "%", "%25" ) has to be the first(!) one, otherwise we would do things like converting spaces into %20 and then convert them into %25%20
#define escapeHTMLAttr(s) QString(s).replace( "%", "%25" ).replace( "'", "%27" ).replace( "\"", "%22" ).replace( "#", "%23" ).replace( "?", "%3F" )
#define unEscapeHTMLAttr(s) QString(s).replace( "%3F", "?" ).replace( "%23", "#" ).replace( "%22", "\"" ).replace( "%27", "'" ).replace( "%25", "%" )

using amaroK::QStringx;

/**
 * Function that must be used when separating contextBrowser escaped urls
 */
static inline
void albumArtistTrackFromUrl( QString url, QString &artist, QString &album, QString &track )
{
    if ( !url.contains("@@@") ) return;
    //KHTML removes the trailing space!
    if ( url.endsWith( " @@@" ) )
        url += ' ';

    const QStringList list = QStringList::split( " @@@ ", url, true );

    Q_ASSERT( !list.isEmpty() );

    artist = unEscapeHTMLAttr( list[0] );
    album  = unEscapeHTMLAttr( list[1] );
    track  = unEscapeHTMLAttr( list[2] );
}


ContextBrowser *ContextBrowser::s_instance = 0;


ContextBrowser::ContextBrowser( const char *name )
        : KTabWidget( 0, name )
        , EngineObserver( EngineController::instance() )
        , m_dirtyHomePage( true )
        , m_dirtyCurrentTrackPage( true )
        , m_dirtyLyricsPage( true )
        , m_dirtyWikiPage( true )
        , m_emptyDB( CollectionDB::instance()->isEmpty() )
        , m_lyricJob( NULL )
        , m_wikiBackPopup( new KPopupMenu( this ) )
        , m_wikiForwardPopup( new KPopupMenu( this ) )
        , m_wikiJob( NULL )
        , m_bgGradientImage( 0 )
        , m_headerGradientImage( 0 )
        , m_shadowGradientImage( 0 )
        , m_suggestionsOpen( true )
        , m_favouritesOpen( true )
        , m_cuefile( NULL )
{
    s_instance = this;

    m_homePage = new KHTMLPart( this, "home_page" );
    m_homePage->setDNDEnabled( true );
    m_currentTrackPage = new KHTMLPart( this, "current_track_page" );
    m_currentTrackPage->setJScriptEnabled( true );
    m_currentTrackPage->setDNDEnabled( true );

    m_lyricsTab = new QVBox(this, "lyrics_tab");

    m_lyricsToolBar = new Browser::ToolBar( m_lyricsTab );
    m_lyricsToolBar->insertButton( "edit_add", LYRICS_ADD, true, i18n("Add Lyrics") );
    m_lyricsToolBar->insertButton( "find", LYRICS_SEARCH, true, i18n("Search For Lyrics") );
    m_lyricsToolBar->insertButton( "reload", LYRICS_REFRESH, true, i18n("Refresh") );
    m_lyricsToolBar->insertLineSeparator();
    m_lyricsToolBar->insertButton( "exec", LYRICS_BROWSER, true, i18n("Open in external browser") );

    m_lyricsPage = new KHTMLPart( m_lyricsTab, "lyrics_page" );
    m_lyricsPage->setJavaEnabled( false );
    m_lyricsPage->setJScriptEnabled( false );
    m_lyricsPage->setPluginsEnabled( false );
    m_lyricsPage->setDNDEnabled( true );

    m_wikiTab = new QVBox(this, "wiki_tab");

    m_wikiToolBar = new Browser::ToolBar( m_wikiTab );
    m_wikiToolBar->insertButton( "back", WIKI_BACK, false, i18n("Back") );
    m_wikiToolBar->insertButton( "forward", WIKI_FORWARD, false, i18n("Forward") );
    m_wikiToolBar->insertLineSeparator();
    m_wikiToolBar->insertButton( "personal", WIKI_ARTIST, false, i18n("Artist Page") );
    m_wikiToolBar->insertButton( "cd", WIKI_ALBUM, false, i18n("Album Page") );
    m_wikiToolBar->insertButton( "contents", WIKI_TITLE, false, i18n("Title Page") );
    m_wikiToolBar->insertLineSeparator();
    m_wikiToolBar->insertButton( "exec", WIKI_BROWSER, true, i18n("Open in external browser") );

    m_wikiToolBar->setDelayedPopup( WIKI_BACK, m_wikiBackPopup );
    m_wikiToolBar->setDelayedPopup( WIKI_FORWARD, m_wikiForwardPopup );

    m_wikiPage = new KHTMLPart( m_wikiTab, "wiki_page" );
    m_wikiPage->setJavaEnabled( false );
    m_wikiPage->setPluginsEnabled( false );
    m_wikiPage->setDNDEnabled( true );

    m_cuefile = CueFile::instance();
    connect( m_cuefile, SIGNAL(metaData( const MetaBundle& )),
             EngineController::instance(), SLOT(slotStreamMetaData( const MetaBundle& )) );

    addTab( m_homePage->view(),         SmallIconSet( "gohome" ),   i18n( "Home" ) );
    addTab( m_currentTrackPage->view(), SmallIconSet( "today" ),    i18n( "Current" ) );
    addTab( m_lyricsTab,                SmallIconSet( "document" ), i18n( "Lyrics" ) );
    addTab( m_wikiTab,                  SmallIconSet( "personal" ),     i18n( "Artist" ) );

    setTabEnabled( m_currentTrackPage->view(), false );
    setTabEnabled( m_lyricsTab, false );
    setTabEnabled( m_wikiTab, false );

    // Delete folder with the cached coverimage shadow pixmaps
    KIO::del( KURL::fromPathOrURL( amaroK::saveLocation( "covershadow-cache/" ) ), false, false );


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
    connect( m_wikiPage->browserExtension(), SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs & ) ),
             this,                               SLOT( openURLRequest( const KURL & ) ) );
    connect( m_wikiPage,                     SIGNAL( popupMenu( const QString&, const QPoint& ) ),
             this,                               SLOT( slotContextMenu( const QString&, const QPoint& ) ) );

    connect( m_lyricsToolBar->getButton( LYRICS_ADD    ), SIGNAL(clicked( int )), SLOT(lyricsAdd()) );
    connect( m_lyricsToolBar->getButton( LYRICS_SEARCH    ), SIGNAL(clicked( int )), SLOT(lyricsSearch()) );
    connect( m_lyricsToolBar->getButton( LYRICS_REFRESH    ), SIGNAL(clicked( int )), SLOT(lyricsRefresh()) );
    connect( m_lyricsToolBar->getButton( LYRICS_BROWSER ), SIGNAL(clicked( int )), SLOT(lyricsExternalPage()) );

    connect( m_wikiToolBar->getButton( WIKI_BACK    ), SIGNAL(clicked( int )), SLOT(wikiHistoryBack()) );
    connect( m_wikiToolBar->getButton( WIKI_FORWARD ), SIGNAL(clicked( int )), SLOT(wikiHistoryForward()) );
    connect( m_wikiToolBar->getButton( WIKI_ARTIST  ), SIGNAL(clicked( int )), SLOT(wikiArtistPage()) );
    connect( m_wikiToolBar->getButton( WIKI_ALBUM   ), SIGNAL(clicked( int )), SLOT(wikiAlbumPage()) );
    connect( m_wikiToolBar->getButton( WIKI_TITLE   ), SIGNAL(clicked( int )), SLOT(wikiTitlePage()) );
    connect( m_wikiToolBar->getButton( WIKI_BROWSER ), SIGNAL(clicked( int )), SLOT(wikiExternalPage()) );

    connect( m_wikiBackPopup,    SIGNAL(activated( int )), SLOT(wikiBackPopupActivated( int )) );
    connect( m_wikiForwardPopup, SIGNAL(activated( int )), SLOT(wikiForwardPopupActivated( int )) );

    connect( CollectionDB::instance(), SIGNAL( scanStarted() ), SLOT( collectionScanStarted() ) );
    connect( CollectionDB::instance(), SIGNAL( scanDone( bool ) ), SLOT( collectionScanDone() ) );
    connect( CollectionDB::instance(), SIGNAL( databaseEngineChanged() ), SLOT( renderView() ) );
    connect( CollectionDB::instance(), SIGNAL( coverFetched( const QString&, const QString& ) ),
             this,                       SLOT( coverFetched( const QString&, const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( coverRemoved( const QString&, const QString& ) ),
             this,                       SLOT( coverRemoved( const QString&, const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( similarArtistsFetched( const QString& ) ),
             this,                       SLOT( similarArtistsFetched( const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( tagsChanged( const MetaBundle& ) ),
             this,                       SLOT( tagsChanged( const MetaBundle& ) ) );

    //the stylesheet will be set up and home will be shown later due to engine signals and doodaa
    //if we call it here setStyleSheet is called 3 times during startup!!
}


ContextBrowser::~ContextBrowser()
{
    delete m_bgGradientImage;
    delete m_headerGradientImage;
    delete m_shadowGradientImage;
    m_cuefile->clear();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::setFont( const QFont &newFont )
{
    QWidget::setFont( newFont );
    setStyleSheet();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::openURLRequest( const KURL &url )
{
    QString artist, album, track;
    albumArtistTrackFromUrl( url.path(), artist, album, track );

    if ( url.protocol() == "album" )
    {
//         QueryBuilder qb;
//         qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
//         qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, artist );
//         qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, album );
//         qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
//         QStringList values = qb.run();
//
//         KURL::List urls;
//         KURL url;
//
//         for( QStringList::ConstIterator it = values.begin(), end = values.end(); it != end; ++it ) {
//             url.setPath( *it );
//             urls.append( url );
//         }
//
//         Playlist::instance()->insertMedia( urls, Playlist::Unique );

        return;
    }

    if ( url.protocol() == "compilation" )
    {
//         QueryBuilder qb;
//         qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
//         qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, url.path() );
//         qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
//         qb.setOptions( QueryBuilder::optOnlyCompilations );
//         QStringList values = qb.run();
//
//         KURL::List urls;
//         KURL url;
//         foreach( values ) {
//             url.setPath( *it );
//             urls.append( url );
//         }
//
//         Playlist::instance()->insertMedia( urls, Playlist::Unique );

        return;
    }
       // here, http urls are streams. For webpages we use externalurl
       // NOTE there have been no links to streams! http now used for wiki tab.
    if ( url.protocol() == "file" )
        Playlist::instance()->insertMedia( url, Playlist::DirectPlay | Playlist::Unique );

    // All http links should be loaded inside wikipedia tab, as that is the only tab that should contain them.
    // Streams should use stream:// protocol.
    if ( url.protocol() == "http" )
    {
        debug() << "Received openURLRequest for: " << url.url() << endl;
        if ( url.hasHTMLRef() )
        {
            KURL base = url;
            base.setRef(QString::null);
            // Wikipedia also has links to otherpages with Anchors, so we have to check if it's for the current one
            if ( m_wikiCurrentUrl == base.url() ) {
                m_wikiPage->gotoAnchor( url.htmlRef() );
                return;
            }
        }
        // new page
        m_dirtyWikiPage = true;
        showWikipedia( url.url() );
    }


    if ( url.protocol() == "show" )
    {
        if ( url.path().contains( "suggestLyric-" ) )
        {
            m_lyrics = QString::null;
            QString hash = url.path().mid( url.path().find( QString( "-" ) ) +1 );
            m_dirtyLyricsPage = true;
            showLyrics( hash );
        }
        else if ( url.path() == "collectionSetup" )
        {
            CollectionView::instance()->setupDirs();
        }
        // Konqueror sidebar needs these
        if( url.path() == "home") { m_dirtyHomePage=true; showHome(); saveHtmlData(); }
        if (url.path() == "context") { m_dirtyCurrentTrackPage=true; showCurrentTrack(); saveHtmlData(); }
        if (url.path() == "wiki") { m_dirtyLyricsPage=true; showWikipedia(); saveHtmlData(); }
        if (url.path() == "lyrics") { m_dirtyWikiPage=true; m_wikiJob=false; showLyrics(); saveHtmlData(); }
    }

    // When left-clicking on cover image, open browser with amazon site
    if ( url.protocol() == "fetchcover" )
    {
        if ( CollectionDB::instance()->findImageByArtistAlbum (artist, album, 0 )
           == CollectionDB::instance()->notAvailCover( 0 ) ) {
            CollectionDB::instance()->fetchCover( this, artist, album, false );
            return;
        }

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
        const QString url = "http://www.musicbrainz.org/taglookup.html?artist=%1&album=%2&track=%3";
        kapp->invokeBrowser( url.arg( KURL::encode_string_no_slash( artist, 106 /*utf-8*/ ),
        KURL::encode_string_no_slash( album, 106 /*utf-8*/ ),
        KURL::encode_string_no_slash( track, 106 /*utf-8*/ ) ) );
    }

    if ( url.protocol() == "externalurl" )
        kapp->invokeBrowser( url.url().replace("externalurl:", "http:") );

    if ( url.protocol() == "togglebox" )
    {
        if ( url.path() == "ss" ) m_suggestionsOpen ^= true;
        if ( url.path() == "ft" ) m_favouritesOpen ^= true;
    }

    if ( url.protocol() == "seek" )
    {
        debug() << "[ContextBrowser] Seek requested to pos " << url.path().toLong() << endl;
        EngineController::engine()->seek(url.path().toLong());
    }
}


void ContextBrowser::collectionScanStarted()
{
    if( m_emptyDB && !AmarokConfig::collectionFolders().isEmpty() )
       showScanning();
}


void ContextBrowser::collectionScanDone()
{
    if ( CollectionDB::instance()->isEmpty() )
    {
        showIntroduction();
        m_emptyDB = true;
    }
    else if ( m_emptyDB )
    {
        PlaylistWindow::self()->showBrowser("CollectionBrowser");
        m_emptyDB = false;
    }
}


void ContextBrowser::renderView()
{
    m_dirtyHomePage = true;
    m_dirtyCurrentTrackPage = true;
    m_dirtyLyricsPage = true;
    m_dirtyWikiPage = true;

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

void ContextBrowser::engineNewMetaData( const MetaBundle& bundle, bool trackChanged )
{
    bool newMetaData = false;
    m_dirtyHomePage = true;
    m_dirtyCurrentTrackPage = true;
    m_dirtyLyricsPage = true;
    m_dirtyWikiPage = true;
    m_lyricJob = 0; //New metadata, so let's forget previous lyric-fetching jobs
    m_wikiJob = 0; //New metadata, so let's forget previous wiki-fetching jobs

    // Prepend stream metadata history item to list
    if ( !m_metadataHistory.first().contains( bundle.prettyTitle() ) )
    {
        newMetaData = true;
        const QString timeString = KGlobal::locale()->formatTime( QTime::currentTime() );
        m_metadataHistory.prepend( QString( "<td valign='top'>" + timeString + "&nbsp;</td><td align='left'>" + escapeHTML( bundle.prettyTitle() ) + "</td>" ) );
    }

    if ( currentPage() == m_homePage->view() )
        showCurrentTrack();
    else if ( currentPage() == m_currentTrackPage->view() && ( bundle.url() != m_currentURL || newMetaData || !trackChanged ) )
        showCurrentTrack();
    if ( currentPage() == m_lyricsTab )
        showLyrics();
    else if ( CollectionDB::instance()->isEmpty() || !CollectionDB::instance()->isValid() )
        showIntroduction();


    if (trackChanged && bundle.url().isLocalFile())
    {
        // look if there is a cue-file
        QString path    = bundle.url().path();
        QString cueFile = path.left( path.findRev('.') ) + ".cue";

        m_cuefile->setCueFileName( cueFile );

        if( m_cuefile->load() )
            debug() << "[CUEFILE]: " << cueFile << " found and loaded." << endl;
    }
}


void ContextBrowser::engineStateChanged( Engine::State state, Engine::State oldState )
{
    DEBUG_BLOCK

    if( state != Engine::Paused /*pause*/ && oldState != Engine::Paused /*resume*/)
    {
        // Pause shouldn't clear everything
        m_dirtyHomePage = true;
        m_dirtyCurrentTrackPage = true;
        m_dirtyLyricsPage = true;
        m_lyricJob = 0; //let's forget previous lyric-fetching jobs
        m_wikiJob = 0; //let's forget previous wiki-fetching jobs
    }

    switch( state )
    {
        case Engine::Empty:
            m_metadataHistory.clear();
            if ( currentPage() == m_currentTrackPage->view() || currentPage() == m_lyricsTab )
                showHome();
            blockSignals( true );
            setTabEnabled( m_currentTrackPage->view(), false );
            setTabEnabled( m_lyricsTab, false );
            if ( currentPage() != m_wikiTab ) {
                setTabEnabled( m_wikiTab, false );
                m_dirtyWikiPage = true;
            }
            else // current tab is wikitab, disable some buttons.
            {
                m_wikiToolBar->setItemEnabled( WIKI_ARTIST, false );
                m_wikiToolBar->setItemEnabled( WIKI_ALBUM, false );
                m_wikiToolBar->setItemEnabled( WIKI_TITLE, false );
            }
            blockSignals( false );
            break;

        case Engine::Playing:
            if ( oldState != Engine::Paused )
                m_metadataHistory.clear();
            blockSignals( true );
            setTabEnabled( m_currentTrackPage->view(), true );
            setTabEnabled( m_lyricsTab, true );
            setTabEnabled( m_wikiTab, true );
            m_wikiToolBar->setItemEnabled( WIKI_ARTIST, true );
            m_wikiToolBar->setItemEnabled( WIKI_ALBUM, true );
            m_wikiToolBar->setItemEnabled( WIKI_TITLE, true );
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
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << m_HTMLSource // the pure html data..
        .replace("<html>",QString("<html><head><style type=\"text/css\">%1</style></head>").arg(m_styleSheet) ); // and the stylesheet code
    exportedDocument.close();
}


void ContextBrowser::paletteChange( const QPalette& pal )
{
    KTabWidget::paletteChange( pal );
    setStyleSheet();
}

//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

//parts of this function from ktabwidget.cpp, copyright (C) 2003 Zack Rusin and Stephan Binner
//fucking setCurrentTab() isn't virtual so we have to override this instead =(
void ContextBrowser::wheelDelta( int delta )
{
    if ( count() < 2 || delta == 0 )
        return;

    int index = currentPageIndex(), start = index;
    do
    {
        if( delta < 0 )
            index = (index + 1) % count();
        else
        {
            index = index - 1;
            if( index < 0 )
                index = count() - 1;
        }
        if( index == start ) // full circle, none enabled
            return;
    } while( !isTabEnabled( page( index ) ) );
    setCurrentPage( index );
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
    else if ( m_dirtyLyricsPage && ( page == m_lyricsTab ) )
        showLyrics();
    else if ( m_dirtyWikiPage && ( page == m_wikiTab ) )
        showWikipedia();
}


void ContextBrowser::slotContextMenu( const QString& urlString, const QPoint& point )
{
    enum { SHOW, FETCH, CUSTOM, DELETE, APPEND, ASNEXT, MAKE, MEDIA_DEVICE, INFO, MANAGER, TITLE };

    if( urlString.isEmpty() ||
        urlString.startsWith( "musicbrainz" ) ||
        urlString.startsWith( "externalurl" ) ||
        urlString.startsWith( "show:suggest" ) ||
        urlString.startsWith( "http" ) ||
        urlString.startsWith( "seek" )
        )
        return;

    KURL url( urlString );

    KPopupMenu menu;
    KURL::List urls( url );
    QString artist, album, track; // track unused here
    albumArtistTrackFromUrl( url.path(), artist, album, track );

    if ( url.protocol() == "fetchcover" )
    {
        menu.insertTitle( i18n( "Cover Image" ) );

        menu.insertItem( SmallIconSet( "viewmag" ), i18n( "&Show Fullsize" ), SHOW );
        menu.insertItem( SmallIconSet( "www" ), i18n( "&Fetch From amazon.%1" ).arg(CoverManager::amazonTld()), FETCH );
        menu.insertItem( SmallIconSet( "folder_image" ), i18n( "Set &Custom Cover" ), CUSTOM );
        menu.insertSeparator();

        menu.insertItem( SmallIconSet( "editdelete" ), i18n("&Unset Cover"), DELETE );
        menu.insertSeparator();
        menu.insertItem( SmallIconSet( "covermanager" ), i18n( "Cover Manager" ), MANAGER );

        #ifndef AMAZON_SUPPORT
        menu.setItemEnabled( FETCH, false );
        #endif
        bool disable = !CollectionDB::instance()->albumImage( artist, album, 0 ).contains( "nocover" );
        menu.setItemEnabled( SHOW, disable );
        menu.setItemEnabled( DELETE, disable );
    }
    else if ( url.protocol() == "file" || url.protocol() == "album" || url.protocol() == "compilation" )
    {
        //TODO it would be handy and more usable to have this menu under the cover one too

        menu.insertTitle( i18n("Track"), TITLE );
        menu.insertItem( SmallIconSet( "fileopen" ), i18n( "&Load" ), MAKE );
        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( SmallIconSet( "2rightarrow" ), i18n( "&Queue Track" ), ASNEXT );
        if( MediaDevice::instance()->isConnected() )
            menu.insertItem( SmallIconSet( "usbpendrive_unmount" ), i18n( "Add to Media Device &Transfer Queue" ), MEDIA_DEVICE );

        menu.insertSeparator();
        menu.insertItem( SmallIconSet( "info" ), i18n( "Edit Track &Information..." ), INFO );

        if ( url.protocol() == "album" )
        {
            QueryBuilder qb;
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
            qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, artist );
            qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, album );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
            QStringList values = qb.run();

            urls.clear(); //remove urlString
            KURL url;
            QStringList::ConstIterator end ( values.end() );
            for( QStringList::ConstIterator it = values.begin(); it != end; ++it )
            {
                url.setPath( *it );
                urls.append( url );
            }

            menu.changeTitle( TITLE, i18n("Album") );
            menu.changeItem( INFO,   i18n("Edit Album &Information..." ) );
            menu.changeItem( ASNEXT, i18n("&Queue Album") );
        }
        if ( url.protocol() == "compilation" )
        {
            QueryBuilder qb;
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
            qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, url.path() );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
            qb.setOptions( QueryBuilder::optOnlyCompilations );
            QStringList values = qb.run();

            urls.clear();
            KURL url;
            foreach( values ) {
                url.setPath( *it );
                urls.append( url );
            }

            menu.changeTitle( TITLE, i18n("Compilation") );
            menu.changeItem( INFO,   i18n("Edit Album &Information..." ) );
            menu.changeItem( ASNEXT, i18n("&Queue Album") );
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
            KStdGuiItem::del() );

        if ( button == KMessageBox::Continue )
        {
            CollectionDB::instance()->removeAlbumImage( artist, album );
            if( currentPage() == m_homePage->view() )
            {
                m_dirtyHomePage = true;
                showHome();
            }
            else if( currentPage() == m_currentTrackPage->view() )
            {
                m_dirtyCurrentTrackPage = true;
                showCurrentTrack();
            }
        }
        break;
    }

    case ASNEXT:
        Playlist::instance()->insertMedia( urls, Playlist::Queue );
        break;

    case INFO:
    {
        if ( urls.count() > 1 )
        {
            TagDialog* dialog = new TagDialog( urls, instance() );
            dialog->show();
        }
        else if ( !urls.isEmpty() )
        {
            TagDialog* dialog = new TagDialog( urls.first() );
            dialog->show();
        }
        break;
    }
    case MAKE:
        Playlist::instance()->clear();

        //FALL_THROUGH

    case APPEND:
        Playlist::instance()->insertMedia( urls, Playlist::Unique );
        break;

    case MEDIA_DEVICE:
        MediaDevice::instance()->addURLs( urls );
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

        KURL file = KFileDialog::getImageOpenURL( startPath, this, i18n("Select Cover Image File") );
        if ( !file.isEmpty() ) {
            CollectionDB::instance()->setAlbumImage( artist, album, file );

            if( currentPage() == m_homePage->view() )
            {
                m_dirtyHomePage = true;
                showHome();
            }
            else if( currentPage() == m_currentTrackPage->view() )
            {
                m_dirtyCurrentTrackPage = true;
                showCurrentTrack();
            }
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
    const QDateTime now = QDateTime::currentDateTime();
    const int datediff = datetime.daysTo( now );

    if( datediff >= 6*7 /*six weeks*/ ) {  // return absolute month/year
        const KCalendarSystem *cal = KGlobal::locale()->calendar();
        const QDate date = datetime.date();
        return i18n( "monthname year", "%1 %2" ).arg( cal->monthName(date), cal->yearString(date, false) );
    }

    //TODO "last week" = maybe within 7 days, but prolly before last sunday

    if( datediff >= 7 )  // return difference in weeks
        return i18n( "One week ago", "%n weeks ago", (datediff+3)/7 );

    const int timediff = datetime.secsTo( now );

    if( timediff >= 24*60*60 /*24 hours*/ )  // return difference in days
        return datediff == 1 ?
                i18n( "Yesterday" ) :
                i18n( "One day ago", "%n days ago", (timediff+12*60*60)/(24*60*60) );

    if( timediff >= 90*60 /*90 minutes*/ )  // return difference in hours
        return i18n( "One hour ago", "%n hours ago", (timediff+30*60)/(60*60) );

    //TODO are we too specific here? Be more fuzzy? ie, use units of 5 minutes, or "Recently"

    if( timediff >= 0 )  // return difference in minutes
        return timediff/60 ?
                i18n( "One minute ago", "%n minutes ago", (timediff+30)/60 ) :
                i18n( "Within the last minute" );

    return i18n( "The future" );
}


void
ContextBrowser::ContructHTMLAlbums(const QStringList & reqResult, QString & htmlCode, QString stID, T_SHOW_ALBUM_TYPE showAlbumType)
{
    // This function create the html code used to display a list of albums. Each album
    // is a 'toggleable' block.
    // Parameter stID is used to diff�entiate same albums in different album list. So if this function
    // is called multiple time in the same HTML code, stID must be different.
    if ( !reqResult.isEmpty() )
    {
        for ( uint i = 0; i < reqResult.count(); i += 3 )
        {
            QueryBuilder qb;
            qb.clear();
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTrack );
            qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valLength );
            qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
            qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valID );
            qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, reqResult[i+1] );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
            //qb.setOptions( QueryBuilder::optNoCompilations );
            QStringList albumValues = qb.run();

            QString albumYear;
            if ( !albumValues.isEmpty() )
            {
                albumYear = albumValues[ 3 ];
                for ( uint j = 0; j < albumValues.count(); j += 7 )
                    if ( albumValues[j + 3] != albumYear || albumYear == "0" )
                    {
                        albumYear = QString::null;
                        break;
                    }
            }

            htmlCode.append( QStringx (
                                 "<tr class='" + QString( (i % 4) ? "box-row-alt" : "box-row" ) + "'>"
                                    "<td>"
                                    "<div class='album-header' onClick=\"toggleBlock('IDA%1')\">"
                                        "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
                                        "<tr>")
                             .args( QStringList()
                                    << stID + reqResult[i+1] ));

            if (showAlbumType == SHOW_ALBUM_SCORE )
            {
                htmlCode.append(
                                            "<td width='30' align='center' class='song-place'>" +
                                                QString::number( ( i / 3 ) + 1 ) +
                                            "</td>");
            }

            QString albumName = escapeHTML( reqResult[ i ].isEmpty() ? i18n( "Unknown album" ) : reqResult[ i ] );

            if (CollectionDB::instance()->albumIsCompilation(reqResult[ i + 1 ]))
            {
                QString albumImage = CollectionDB::instance()->albumImage( albumValues[5], reqResult[ i ], 50 );
                if ( albumImage != CollectionDB::instance()->notAvailCover( 50 ) )
                    albumImage = ContextBrowser::makeShadowedImage( albumImage );

                // Compilation image
                htmlCode.append( QStringx (
                                            "<td width='1'>"
                                                "<a href='fetchcover: @@@ %1'>"
                                                    "<img class='album-image' align='left' vspace='2' hspace='2' title='%2' src='%3'/>"
                                                "</a>"
                                            "</td>"
                                            "<td valign='middle' align='left'>"
                                                "<a href='compilation:%4'><span class='album-title'>%5</span></a>" )
                                 .args( QStringList()
                                        << escapeHTMLAttr( reqResult[ i ].isEmpty() ? i18n( "Unknown" ) : reqResult[ i ] ) // album.name
                                        << i18n( "Click for information from amazon.com, right-click for menu." )
                                        << escapeHTMLAttr( albumImage )
                                        << reqResult[ i + 1 ] //album.id
                                        << albumName ) );
            }
            else
            {
                QString artistName = escapeHTML( albumValues[5].isEmpty() ? i18n( "Unknown artist" ) : albumValues[5] );

                QString albumImage = CollectionDB::instance()->albumImage( albumValues[5], reqResult[ i ], 50 );
                if ( albumImage != CollectionDB::instance()->notAvailCover( 50 ) )
                    albumImage = ContextBrowser::makeShadowedImage( albumImage );

                // Album image
                htmlCode.append( QStringx (
                                            "<td width='1'>"
                                                "<a href='fetchcover:%1 @@@ %2'>"
                                                    "<img class='album-image' align='left' vspace='2' hspace='2' title='%3' src='%4'/>"
                                                "</a>"
                                            "</td>"
                                            "<td valign='middle' align='left'>"
                                                "<a href='album:%5 @@@ %6'>"
                                                    "<span class='album-title'>%7</span>"
                                                    "<span class='song-separator'> - </span>"
                                                    "<span class='album-title'>%8</span>"
                                                "</a>" )
                                 .args( QStringList()
                                        << escapeHTMLAttr( albumValues[5] ) // artist name
                                        << escapeHTMLAttr( reqResult[ i ].isEmpty() ? i18n( "Unknown" ) : reqResult[ i ] ) // album.name
                                        << i18n( "Click for information from amazon.com, right-click for menu." )
                                        << escapeHTMLAttr( albumImage )
                                        << albumValues[6]
                                        << reqResult[ i + 1 ] //album.id
                                        << artistName
                                        << albumName ) );
            }

            switch (showAlbumType)
            {
                case SHOW_ALBUM_NORMAL :
                {
                    // Tracks number and year
                    htmlCode.append( QStringx (
                                                "<span class='album-info'>%1</span> "
                                                "<br />"
                                                "<span class='album-year'>%2</span>"
                                            "</td>")
                                    .args( QStringList()
                                            << i18n( "Single", "%n Tracks",  albumValues.count() / qb.countReturnValues() )
                                            << albumYear) );
                    break;
                }
                case SHOW_ALBUM_SCORE:
                {
                    htmlCode.append( QStringx (
                                                "<br />"
                                            "</td>"
                                            "<td class='sbtext' width='1'>%1</td>"
                                            "<td width='1' title='" + i18n( "Score" ) + "'>"
                                                "<div class='sbouter'>"
                                                    "<div class='sbinner' style='width: %2px;'></div>"
                                                "</div>"
                                            "</td>"
                                            "<td width='1'></td>")
                                    .args( QStringList()
                                            << QString::number( (int)(reqResult[i + 2].toFloat()) )
                                            << QString::number( (int)(reqResult[i + 2].toFloat() / 2 ) ) ));
                    break;
                }
                case SHOW_ALBUM_LEAST_PLAY:
                {
                    QDateTime lastPlay = QDateTime();
                    lastPlay.setTime_t( reqResult[i + 2].toUInt() );

                    htmlCode.append( QStringx (
                                                "<br />"
                                                "<span class='song-time'>%1</span>"
                                            "</td>")
                                      .args( QStringList()
                                              << i18n( "Last played: %1" ).arg( verboseTimeSince( lastPlay ) ) ) );
                    break;
                }
            }

            // Begining of the 'toggleable div' that contains the songs
            htmlCode.append( QStringx (
                                        "</tr>"
                                 "</table>"
                             "</div>"
                             "<div class='album-body' style='display:%1;' id='IDA%2'>" )
                             .args( QStringList()
                                    << "none" /* shows it if it's the current track album */
                                    << stID + reqResult[ i + 1 ] ) );

            if ( !albumValues.isEmpty() )
            {
                for ( uint j = 0; j < albumValues.count(); j += 7 )
                {
                    QString track = albumValues[j + 2].stripWhiteSpace();
                    if( track.length() > 0 )
                    {
                        if( track.length() == 1 )
                            track.prepend( "0" );

                        track = "<span class='album-song-trackno'>" + track + "&nbsp;</span>";
                    }

                    QString length;
                    if( albumValues[j + 4] != "0" )
                        length = "<span class='album-song-time'>(" + MetaBundle::prettyTime( QString(albumValues[j + 4]).toInt(), false ) + ")</span>";

                    htmlCode.append(
                                "<div class='album-song'>"
                                    "<a href=\"file:" + escapeHTMLAttr( albumValues[j + 1] ) + "\">"
                                        + track +
                                        "<span class='album-song-title'>" + escapeHTML( albumValues[j] ) + "</span>&nbsp;"
                                        + length +
                                    "</a>"
                                "</div>" );
                }
            }

            htmlCode.append(
                            "</div>"
                        "</td>"
                    "</tr>" );
        }
        htmlCode.append(
                "</table>"
            "</div>" );
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// Home-Tab
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::showHome() //SLOT
{
    DEBUG_BLOCK

    if ( currentPage() != m_homePage->view() )
    {
        blockSignals( true );
        showPage( m_homePage->view() );
        blockSignals( false );
    }

    if ( CollectionDB::instance()->isEmpty() || !CollectionDB::instance()->isValid() )
    {
        //TODO show scanning message if scanning, not the introduction
        showIntroduction();
        return;
    }

    // Do we have to rebuild the page?
    if ( !m_dirtyHomePage ) return;

    if (!AmarokConfig::showStatByAlbums())
    {
        ContextBrowser::showHomeBySongs();
    }
    else
    {
        ContextBrowser::showHomeByAlbums();
    }

    m_homePage->write( m_HTMLSource );
    m_homePage->end();

    m_dirtyHomePage = false;
    saveHtmlData(); // Send html code to file

}


void ContextBrowser::showHomeBySongs()
{
    QueryBuilder qb;
    qb.clear();
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
    qb.setLimit( 0, 10 );
    QStringList fave = qb.run();

    qb.clear();
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valAccessDate, true );
    qb.setLimit( 0, 10 );
    QStringList lastplayed = qb.run();

    qb.clear();
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valCreateDate, true );
    qb.setLimit( 0, 10 );
    QStringList newest = qb.run();

    m_homePage->begin();
    m_HTMLSource="";
    m_homePage->setUserStyleSheet( m_styleSheet );

    // <Recent Tracks Information>
    m_HTMLSource.append(
            "<html>"
            "<div id='newest_box' class='box'>"
                "<div id='newest_box-header' class='box-header'>"
                    "<span id='newest_box-header-title' class='box-header-title'>"
                    + i18n( "Recently Played Tracks" ) +
                    "</span>"
                "</div>" );

    if ( lastplayed.count() == 0 )
    {
        m_HTMLSource.append(
                "<div id='newest_box-body' class='box-body'><p>" +
                i18n( "A list of your recently played tracks will appear here, once you have played a few of your songs." ) +
                "</p></div>" );
    }
    else
    {
        m_HTMLSource.append(
                "<div id='newest_box-body' class='box-body'>"
                             "<table border='0' cellspacing='0' cellpadding='0' width='100%' class='song'>" );

        for( uint i = 0; i < lastplayed.count(); i = i + 4 )
        {
            m_HTMLSource.append(
                    "<div class='" + QString( (i % 8) ? "box-row-alt" : "box-row" ) + "'>"
                        "<div class='song'>"
                            "<a href=\"file:" + escapeHTMLAttr( lastplayed[i + 1] ) + "\">"
                            "<span class='song-title'>" + escapeHTML( lastplayed[i] ) + "</span><br />"
                            "<span class='song-artist'>" + escapeHTML( lastplayed[i + 2] ) + "</span>"
                        );

            if ( !lastplayed[i + 3].isEmpty() )
                m_HTMLSource.append(
                    "<span class='song-separator'>"
                    + i18n("&#xa0;&#8211 ") +
                    "</span><span class='song-album'>" + escapeHTML( lastplayed[i + 3] ) + "</span>"
                            );

            m_HTMLSource.append(
                            "</a>"
                        "</div>"
                        "</div>");
        }
        m_HTMLSource.append("</table> </div>");
    }
    m_HTMLSource.append(
            "</div>");

    // </Recent Tracks Information>


    // <Favorite Tracks Information>
    m_HTMLSource.append(
            "<html>"
            "<div id='favorites_box' class='box'>"
                "<div id='favorites_box-header' class='box-header'>"
                    "<span id='favorites_box-header-title' class='box-header-title'>"
                    + i18n( "Your Favorite Tracks" ) +
                    "</span>"
                "</div>" );

    if ( fave.count() == 0 )
    {
        m_HTMLSource.append(
                "<div id='favorites_box-body' class='box-body'><p>" +
                i18n( "A list of your favorite tracks will appear here, once you have played a few of your songs." ) +
                "</p></div>" );
    }
    else
    {
        m_HTMLSource.append(
                "<div id='favorites_box-body' class='box-body'>"
                             "<table border='0' cellspacing='0' cellpadding='0' width='100%' class='song'>" );

        for( uint i = 0; i < fave.count(); i = i + 5 )
        {
            m_HTMLSource.append(
                        "<tr class='" + QString( (i % 10) ? "box-row-alt" : "box-row" ) + "'>"
                                    "<td width='30' align='center' class='song-place'>" + QString::number( ( i / 5 ) + 1 ) + "</td>"
                                    "<td>"
                                        "<a href=\"file:" + escapeHTMLAttr( fave[i + 1] ) + "\">"
                                            "<span class='song-title'>" + escapeHTML( fave[i] ) + "</span><br /> "
                                            "<span class='song-artist'>" + escapeHTML( fave[i + 3] ) + "</span>" );

            if ( !fave[i + 4].isEmpty() )
            m_HTMLSource.append(        "<span class='song-separator'>"
                                            + i18n("&#xa0;&#8211; ") +
                                         "</span><span class='song-album'>"+ escapeHTML( fave[i + 4] ) +"</span>" );

            m_HTMLSource.append(
                                "</a>"
                                    "</td>"
                                    "<td class='sbtext' width='1'>" + fave[i + 2] + "</td>"
                                    "<td width='1' title='" + i18n( "Score" ) + "'>"
                                        "<div class='sbouter'>"
                                            "<div class='sbinner' style='width: " + QString::number( fave[i + 2].toInt() / 2 ) + "px;'></div>"
                                        "</div>"
                                    "</td>"
                                    "<td width='1'></td>"
                                "</tr>" );
        }

        m_HTMLSource.append(
                    "</table>"
                             "</div>" );
    }
    m_HTMLSource.append(
            "</div>");

    // </Favorite Tracks Information>

    // <Newest Tracks Information>
    m_HTMLSource.append(
            "<div id='newest_box' class='box'>"
                "<div id='newest_box-header' class='box-header'>"
                    "<span id='newest_box-header-title' class='box-header-title'>"
                    + i18n( "Your Newest Tracks" ) +
                    "</span>"
                "</div>"
                "<div id='newest_box-body' class='box-body'>" );

    for( uint i = 0; i < newest.count(); i = i + 4 )
    {
        m_HTMLSource.append(
                 "<div class='" + QString( (i % 8) ? "box-row-alt" : "box-row" ) + "'>"
                    "<div class='song'>"
                        "<a href=\"file:" + escapeHTMLAttr( newest[i + 1] ) + "\">"
                        "<span class='song-title'>" + escapeHTML( newest[i] ) + "</span><br />"
                        "<span class='song-artist'>" + escapeHTML( newest[i + 2] ) + "</span>"
                    );

        if ( !newest[i + 3].isEmpty() )
            m_HTMLSource.append(
                "<span class='song-separator'>"
                + i18n("&#xa0;&#8211 ") +
                "</span><span class='song-album'>" + escapeHTML( newest[i + 3] ) + "</span>"
                        );

        m_HTMLSource.append(
                        "</a>"
                    "</div>"
                    "</div>");
    }
    m_HTMLSource.append(
                "</div>"
            "</div>"
            "</html>" );

    // </Newest Tracks Information>
}


void ContextBrowser::showHomeByAlbums()
{

    QueryBuilder qb;
    qb.clear();
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valID );
    qb.addReturnFunctionValue(QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valPercentage);
    qb.sortByFunction( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
    qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
    qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valID);
    qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valName);
    qb.setLimit( 0, 5 );
    QStringList faveAlbums = qb.run();

    qb.clear();
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valID );
    qb.addReturnFunctionValue(QueryBuilder::funcMax, QueryBuilder::tabSong, QueryBuilder::valCreateDate);
    qb.sortByFunction( QueryBuilder::funcMax, QueryBuilder::tabSong, QueryBuilder::valCreateDate, true );
    qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
    qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valID);
    qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valName);
    qb.setLimit( 0, 5 );
    QStringList recentAlbums = qb.run();

    qb.clear();
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valID );
    qb.addReturnFunctionValue(QueryBuilder::funcMax, QueryBuilder::tabStats, QueryBuilder::valAccessDate);
    qb.sortByFunction( QueryBuilder::funcMax, QueryBuilder::tabStats, QueryBuilder::valAccessDate, false );
    qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
    qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valID);
    qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valName);
    qb.setLimit( 0, 5 );
    QStringList leastAlbums = qb.run();

    m_homePage->begin();
    m_HTMLSource="";
    m_homePage->setUserStyleSheet( m_styleSheet );

    m_HTMLSource.append("<html>");

    // write the script to toggle blocks visibility
    m_HTMLSource.append( "<script type='text/javascript'>"
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

    // <Favorite Albums Information>
    m_HTMLSource.append(
        "<div id='albums_box' class='box'>"
        "<div id='albums_box-header' class='box-header'>"
        "<span id='albums_box-header-title' class='box-header-title'>"
        + i18n( "Favorite Albums" ) +
        "</span>"
        "</div>"
        "<table id='albums_box-body' class='box-body' width='100%' border='0' cellspacing='0' cellpadding='0'>" );

    if ( faveAlbums.count() == 0 )
    {
        m_HTMLSource.append(
            "<div id='favorites_box-body' class='box-body'><p>" +
            i18n( "A list of your favorite albums will appear here, once you have played a few of your songs." ) +
            "</p></div>" );
    }
    else
    {
        ContructHTMLAlbums(faveAlbums, m_HTMLSource, "1", SHOW_ALBUM_SCORE);
    }

    m_HTMLSource.append("</div>");

    // </Favorite Tracks Information>

    m_HTMLSource.append(
        "<div id='least_box' class='box'>"
        "<div id='least_box-header' class='box-header'>"
        "<span id='least_box-header-title' class='box-header-title'>"
        + i18n( "Your Newest Albums" ) +
        "</span>"
        "</div>"
        "<div id='least_box-body' class='box-body'>" );
    ContructHTMLAlbums(recentAlbums, m_HTMLSource, "2", SHOW_ALBUM_NORMAL);

    m_HTMLSource.append(
        "</div>"
        "</div>");

    // </Recent Tracks Information>

    // <Songs least listened Information>
    m_HTMLSource.append(
        "<div id='least_box' class='box'>"
        "<div id='least_box-header' class='box-header'>"
        "<span id='least_box-header-title' class='box-header-title'>"
        + i18n( "Least Played Albums" ) +
        "</span>"
        "</div>"
        "<div id='least_box-body' class='box-body'>" );

    if (leastAlbums.count() == 0)
    {
        m_HTMLSource.append(
            "<div class='info'><p>" +
            i18n( "A list of albums, which you have not played for a long time, will appear here." ) +
            "</p></div>"
        );
    }
    else
    {
        ContructHTMLAlbums(leastAlbums, m_HTMLSource, "3", SHOW_ALBUM_LEAST_PLAY);
    }

    m_HTMLSource.append(
                "</div>"
            "</div>"
            "</html>"
                       );
    // </Songs least listened Information>
}


//////////////////////////////////////////////////////////////////////////////////////////
// Current-Tab
//////////////////////////////////////////////////////////////////////////////////////////

/** This is the slowest part of track change, so we thread it */
class CurrentTrackJob : public ThreadWeaver::DependentJob
{
public:
    CurrentTrackJob( ContextBrowser *parent )
            : ThreadWeaver::DependentJob( parent, "CurrentTrackJob" )
            , b( parent ) {}

private:
    virtual bool doJob();
    virtual void completeJob()
    {
        // are we still showing the currentTrack page?
        if( b->currentPage() != b->m_currentTrackPage->view() )
            return;

        b->m_currentTrackPage->begin();
        b->m_HTMLSource = m_HTMLSource;
        b->m_currentTrackPage->setUserStyleSheet( b->m_styleSheet );
        b->m_currentTrackPage->write( m_HTMLSource );
        b->m_currentTrackPage->end();

        b->m_dirtyCurrentTrackPage = false;
        b->saveHtmlData(); // Send html code to file
    }

    QString m_HTMLSource;

    ContextBrowser *b;
};


void ContextBrowser::showCurrentTrack() //SLOT
{
    if ( currentPage() != m_currentTrackPage->view() ) {
        blockSignals( true );
        showPage( m_currentTrackPage->view() );
        blockSignals( false );
    }

    if( !m_dirtyCurrentTrackPage )
        return;

    m_currentURL = EngineController::instance()->bundle().url();
    m_currentTrackPage->write( QString::null );

    ThreadWeaver::instance()->onlyOneJob( new CurrentTrackJob( this ) );
}


bool CurrentTrackJob::doJob()
{
    DEBUG_BLOCK

    const MetaBundle &currentTrack = EngineController::instance()->bundle();

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
                    "<table id='current_box-body' class='box-body' width='100%' border='0' cellspacing='0' cellpadding='1'>"
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

        if ( b->m_metadataHistory.count() > 2 )
        {
            m_HTMLSource.append(
                "<div id='stream-history_box' class='box'>"
                 "<div id='stream-history_box-header' class='box-header'>" + i18n( "Metadata History" ) + "</div>"
                 "<table id='stream-history_box-body' class='box-body' width='100%' border='0' cellspacing='0' cellpadding='1'>" );

            // Ignore last two items, as they don't belong in the history
            for ( uint i = 0; i < b->m_metadataHistory.count() - 2; ++i )
            {
                const QString str = b->m_metadataHistory[i];
                m_HTMLSource.append( QStringx( "<tr class='box-row'><td>%1</td></tr>" ).arg( str ) );
            }

            m_HTMLSource.append(
                 "</table>"
                "</div>" );
        }

        m_HTMLSource.append("</html>" );
        return true;
    }

    const uint artist_id = CollectionDB::instance()->artistID( currentTrack.artist() );
    const uint album_id  = CollectionDB::instance()->albumID ( currentTrack.album() );

    QueryBuilder qb;
    // <Current Track Information>
    qb.clear();
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valCreateDate );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valAccessDate );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valPlayCounter );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
    qb.addMatch( QueryBuilder::tabStats, QueryBuilder::valURL, currentTrack.url().path() );
    QStringList values = qb.run();

    //making 2 tables is most probably not the cleanest way to do it, but it works.
    QString albumImageTitleAttr;
    QString albumImage = CollectionDB::instance()->albumImage( currentTrack );

    if ( albumImage == CollectionDB::instance()->notAvailCover( 0 ) )
        albumImageTitleAttr = i18n( "Click to fetch cover from amazon.%1, right-click for menu." ).arg( CoverManager::amazonTld() );
    else {
        albumImageTitleAttr = i18n( "Click for information from amazon.%1, right-click for menu." ).arg( CoverManager::amazonTld() );
        albumImage = ContextBrowser::makeShadowedImage( albumImage );
    }

    m_HTMLSource.append(
            "<div id='current_box' class='box'>"
            "<div id='current_box-header' class='box-header'>"
            // Show "Title - Artist \n Album", or only "PrettyTitle" if there's no title tag
            + ( !currentTrack.title().isEmpty()
            ? QStringx(
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
                            "<a id='musicbrainz-a' title='%8' href='musicbrainz:%9 @@@ %10 @@@ %11'>"
                            "<img id='musicbrainz-image' src='%12' />"
                            "</a>"
                        "</div>"
                )
                .args( QStringList()
                << escapeHTML( currentTrack.title() )
                << escapeHTML( currentTrack.artist() )
                << escapeHTML( currentTrack.album() )
                << escapeHTMLAttr( currentTrack.artist() )
                << escapeHTMLAttr( currentTrack.album() )
                << escapeHTMLAttr( albumImage )
                << albumImageTitleAttr
                << i18n( "Look up this track at musicbrainz.org" )
                << escapeHTMLAttr( currentTrack.artist() )
                << escapeHTMLAttr( currentTrack.album() )
                << escapeHTMLAttr( currentTrack.title() )
                << escapeHTML( locate( "data", "amarok/images/musicbrainz.png" ) ) )
            : QString ( //no title
                "<span id='current_box-header-prettytitle' class='box-header-prettytitle'>%1</span> "
                "</div>"
                "<table id='current_box-table' class='box-body' width='100%' cellpadding='0' cellspacing='0'>"
                    "<tr>"
                        "<td id='current_box-largecover-td'>"
                            "<a id='current_box-largecover-a' href='fetchcover:%2 @@@ %3'>"
                                "<img id='current_box-largecover-image' src='%4' title='%5'>"
                            "</a>"
                        "</td>"
                        "<td id='current_box-information-td' align='right'>"
                )
                .arg( escapeHTML( currentTrack.prettyTitle() ) )
                .arg( escapeHTMLAttr( currentTrack.artist() ) )
                .arg( escapeHTMLAttr( currentTrack.album() ) )
                .arg( escapeHTMLAttr( albumImage ) )
                .arg( albumImageTitleAttr )
            ) );

    if ( !values.isEmpty() && values[2].toInt() )
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

    if ( currentTrack.url().isLocalFile() && !CollectionDB::instance()->isFileInCollection( currentTrack.url().path() ) )
    {
        m_HTMLSource.append(
        "<div id='notindb_box' class='box'>"
            "<div id='notindb_box-header' class='box-header'>"
                "<span id='notindb_box-header-title' class='box-header-title'>"
                + i18n( "This file is not in your Collection!" ) +
                "</span>"
            "</div>"
            "<div id='notindb_box-body' class='box-body'>"
                "<div class='info'><p>"
                + i18n( "If you would like to see contextual information about this track,"
                        " you should add it to your Collection." ) +
                "</p></div>"
                "<div align='center'>"
                "<input type='button' onClick='window.location.href=\"show:collectionSetup\";' value='"
                + i18n( "Change Collection Setup..." ) +
                "' class='button' /></div><br />"
            "</div>"
        "</div>"
                           );
    }

    /* cue file code */
    if ( b->m_cuefile && (b->m_cuefile->count() > 0) ) {
        m_HTMLSource.append(
        "<div id='cue_box' class='box'>"
            "<div id='cue_box-header' class='box-header'>"
                    "<span id='cue_box-header-title' class='box-header-title' onClick=\"toggleBlock('T_CC'); window.location.href='togglebox:cc';\" style='cursor: pointer;'>"
                    + i18n( "Cue File" ) +
                    "</span>"
            "</div>"
            "<table id='cue_box-body' class='box-body' id='T_CC' width='100%' border='0' cellspacing='0' cellpadding='1'>" );
                CueFile::Iterator it;
                uint i = 0;
                for ( it = b->m_cuefile->begin(); it != b->m_cuefile->end(); ++it ) {
                    m_HTMLSource.append(
                    "<tr class='" + QString( (i++ % 2) ? "box-row-alt" : "box-row" ) + "'>"
                        "<td class='song'>"
                            "<a href=\"seek:" + QString::number(it.key()) + "\">"
                            "<span class='album-song-trackno'>" + QString::number(it.data().getTrackNumber()) + "&nbsp;</span>"
                            "<span class='album-song-title'>" + escapeHTML( it.data().getTitle() ) + "</span>"
                            "<span class='song-separator'>"
                            + i18n("&#xa0;&#8211; ") +
                            "</span>"
                            "<span class='album-song-title'>" + escapeHTML( it.data().getArtist() ) + "</span>"
                            "</a>"
                        "</td>"
                    ""
                    );
                }
            m_HTMLSource.append(
            "</table>"
        "</div>"
        );
    }



    // <Suggested Songs>
    QStringList relArtists;
    relArtists = CollectionDB::instance()->similarArtists( currentTrack.artist(), 10 );
    if ( !relArtists.isEmpty() ) {
        QString token;

        qb.clear();
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
        qb.addMatches( QueryBuilder::tabArtist, relArtists );
        qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valScore, true );
        qb.setLimit( 0, 5 );
        values = qb.run();

        // not enough items returned, let's fill the list with score-less tracks
        if ( values.count() < 10 * qb.countReturnValues() )
        {
            qb.clear();
            qb.exclusiveFilter( QueryBuilder::tabSong, QueryBuilder::tabStats, QueryBuilder::valURL );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
            qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
            qb.addMatches( QueryBuilder::tabArtist, relArtists );
            qb.setOptions( QueryBuilder::optRandomize );
            qb.setLimit( 0, 10 - values.count() / 4 );

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
                "<div id='suggested_box-header' class='box-header' onClick=\"toggleBlock('T_SS'); window.location.href='togglebox:ss';\" style='cursor: pointer;'>"
                    "<span id='suggested_box-header-title' class='box-header-title'>"
                    + i18n( "Suggested Songs" ) +
                    "</span>"
                "</div>"
                "<table class='box-body' id='T_SS' width='100%' border='0' cellspacing='0' cellpadding='1'>" );

            for ( uint i = 0; i < values.count(); i += 4 )
                m_HTMLSource.append(
                    "<tr class='" + QString( (i % 8) ? "box-row-alt" : "box-row" ) + "'>"
                        "<td class='song'>"
                            "<a href=\"file:" + escapeHTMLAttr( values[i] ) + "\">"
                            "<span class='album-song-title'>"+ escapeHTML( values[i + 2] ) + "</span>"
                "<span class='song-separator'>"
                + i18n("&#xa0;&#8211; ") +
                "</span><span class='album-song-title'>" + escapeHTML( values[i + 1] ) + "</span>"
                            "</a>"
                        "</td>"
                        "<td class='sbtext' width='1'>" + values[i + 3] + "</td>"
                        "<td width='1' title='" + i18n( "Score" ) + "'>"
                            "<div class='sbouter'>"
                                "<div class='sbinner' style='width: " + QString::number( values[i + 3].toInt() / 2 ) + "px;'></div>"
                            "</div>"
                        "</td>"
                        "<td width='1'></td>"
                    "</tr>" );

            m_HTMLSource.append(
                 "</table>"
             "</div>" );

            if ( !b->m_suggestionsOpen )
                m_HTMLSource.append( "<script language='JavaScript'>toggleBlock('T_SS');</script>" );
        }
    }
    // </Suggested Songs>

    QString artistName = currentTrack.artist().isEmpty() ? i18n( "This Artist" ) : escapeHTML( currentTrack.artist() );
    if ( !currentTrack.artist().isEmpty() ) {
    // <Favourite Tracks Information>
        qb.clear();
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
        qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, QString::number( artist_id ) );
        qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
        qb.setLimit( 0, 10 );
        values = qb.run();

        if ( !values.isEmpty() )
        {
            m_HTMLSource.append(
            "<div id='favoritesby_box' class='box'>"
                "<div id='favoritesby-header' class='box-header' onClick=\"toggleBlock('T_FT'); window.location.href='togglebox:ft';\" style='cursor: pointer;'>"
                    "<span id='favoritesby_box-header-title' class='box-header-title'>"
                    + i18n( "Favorite Tracks by %1" ).arg( artistName ) +
                    "</span>"
                "</div>"
                "<table class='box-body' id='T_FT' width='100%' border='0' cellspacing='0' cellpadding='1'>" );

            for ( uint i = 0; i < values.count(); i += 3 )
                m_HTMLSource.append(
                    "<tr class='" + QString( (i % 6) ? "box-row-alt" : "box-row" ) + "'>"
                        "<td class='song'>"
                            "<a href=\"file:" + escapeHTMLAttr( values[i + 1] ) + "\">"
                            "<span class='album-song-title'>" + escapeHTML( values[i] ) + "</span>"
                            "</a>"
                        "</td>"
                        "<td class='sbtext' width='1'>" + values[i + 2] + "</td>"
                        "<td width='1' title='" + i18n( "Score" ) + "'>"
                            "<div class='sbouter'>"
                                "<div class='sbinner' style='width: " + QString::number( values[i + 2].toInt() / 2 ) + "px;'></div>"
                            "</div>"
                        "</td>"
                        "<td width='1'></td>"
                    "</tr>"
                                );

            m_HTMLSource.append(
                    "</table>"
                "</div>" );

            if ( !b->m_favouritesOpen )
                m_HTMLSource.append( "<script language='JavaScript'>toggleBlock('T_FT');</script>" );

        }
    // </Favourite Tracks Information>

    // <Albums by this artist>
        qb.clear();
        qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valID );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, QString::number( artist_id ) );
        qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName, true );
        qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.setOptions( QueryBuilder::optRemoveDuplicates );
        qb.setOptions( QueryBuilder::optNoCompilations );
        values = qb.run();

        if ( !values.isEmpty() )
        {
            // write the script to toggle blocks visibility
            m_HTMLSource.append(
            "<div id='albums_box' class='box'>"
                "<div id='albums_box-header' class='box-header'>"
                    "<span id='albums_box-header-title' class='box-header-title'>"
                    + i18n( "Albums by %1" ).arg( artistName ) +
                    "</span>"
                "</div>"
                "<table id='albums_box-body' class='box-body' width='100%' border='0' cellspacing='0' cellpadding='0'>" );

            uint vectorPlace = 0;
            // find album of the current track (if it exists)
            while ( vectorPlace < values.count() && values[ vectorPlace+1 ] != QString::number( album_id ) )
                vectorPlace += 2;
            for ( uint i = 0; i < values.count(); i += 2 )
            {
                qb.clear();
                qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
                qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
                qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTrack );
                qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName );
                qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valLength );
                qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, values[ i + 1 ] );
                qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, QString::number( artist_id ) );
                qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
                qb.setOptions( QueryBuilder::optNoCompilations );
                QStringList albumValues = qb.run();

                QString albumYear;
                if ( !albumValues.isEmpty() )
                {
                    albumYear = albumValues[ 3 ];
                    for ( uint j = 0; j < albumValues.count(); j += 5 )
                        if ( albumValues[j + 3] != albumYear || albumYear == "0" )
                        {
                            albumYear = QString::null;
                            break;
                        }
                }

                QString albumImage = CollectionDB::instance()->albumImage( currentTrack.artist(), values[ i ], 50 );
                if ( albumImage != CollectionDB::instance()->notAvailCover( 50 ) )
                    albumImage = ContextBrowser::makeShadowedImage( albumImage );

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
                        << values[ i + 1 ]
                        << escapeHTMLAttr( currentTrack.artist() ) // artist name
                        << escapeHTMLAttr( values[ i ].isEmpty() ? i18n( "Unknown" ) : values[ i ] ) // album.name
                        << i18n( "Click for information from amazon.com, right-click for menu." )
                        << escapeHTMLAttr( albumImage )
                        << i18n( "Single", "%n Tracks",  albumValues.count() / qb.countReturnValues() )
                        << QString::number( artist_id )
                        << values[ i + 1 ] //album.id
                        << escapeHTML( values[ i ].isEmpty() ? i18n( "Unknown" ) : values[ i ] )
                        << albumYear
                        << ( i!=vectorPlace ? "none" : "block" ) /* shows it if it's the current track album */
                        << values[ i + 1 ] ) );

                if ( !albumValues.isEmpty() )
                    for ( uint j = 0; j < albumValues.count(); j += 5 )
                    {
                        QString track = albumValues[j + 2].stripWhiteSpace();
                        if( track.length() > 0 ) {
                            if( track.length() == 1 )
                                track.prepend( "0" );

                            track = "<span class='album-song-trackno'>" + track + "&nbsp;</span>";
                        }

                        QString length;
                        if( albumValues[j + 4] != "0" )
                            length = "<span class='album-song-time'>(" + MetaBundle::prettyTime( QString(albumValues[j + 4]).toInt(), false ) + ")</span>";

                        m_HTMLSource.append(
                            "<div class='album-song'>"
                                "<a href=\"file:" + escapeHTMLAttr( albumValues[j + 1] ) + "\">"
                                + track +
                                "<span class='album-song-title'>" + escapeHTML( albumValues[j] ) + "</span>&nbsp;"
                                + length +
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

    // <Compilations with this artist>
        qb.clear();
        qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valID );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, QString::number( artist_id ) );
        qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName, true );
        qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.setOptions( QueryBuilder::optRemoveDuplicates );
        qb.setOptions( QueryBuilder::optOnlyCompilations );
        values = qb.run();

        if ( !values.isEmpty() )
        {
            // write the script to toggle blocks visibility
            m_HTMLSource.append(
            "<div id='albums_box' class='box'>"
                "<div id='albums_box-header' class='box-header'>"
                    "<span id='albums_box-header-title' class='box-header-title'>"
                    + i18n( "Compilations with %1" ).arg( artistName ) +
                    "</span>"
                "</div>"
                "<table id='albums_box-body' class='box-body' width='100%' border='0' cellspacing='0' cellpadding='0'>" );

            uint vectorPlace = 0;
            // find album of the current track (if it exists)
            while ( vectorPlace < values.count() && values[ vectorPlace+1 ] != QString::number( album_id ) )
                vectorPlace += 2;
            for ( uint i = 0; i < values.count(); i += 2 )
            {
                qb.clear();
                qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
                qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
                qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTrack );
                qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName );
                qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valLength );
                qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
                qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, values[ i + 1 ] );
                qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
                qb.setOptions( QueryBuilder::optOnlyCompilations );
                QStringList albumValues = qb.run();

                QString albumYear;
                if ( !albumValues.isEmpty() )
                {
                    albumYear = albumValues[ 3 ];
                    for ( uint j = 0; j < albumValues.count(); j += qb.countReturnValues() )
                        if ( albumValues[j + 3] != albumYear || albumYear == "0" )
                        {
                            albumYear = QString::null;
                            break;
                        }
                }

                QString albumImage = CollectionDB::instance()->albumImage( currentTrack.artist(), values[ i ], 50 );
                if ( albumImage != CollectionDB::instance()->notAvailCover( 50 ) )
                    albumImage = ContextBrowser::makeShadowedImage( albumImage );

                m_HTMLSource.append( QStringx (
                "<tr class='" + QString( (i % 4) ? "box-row-alt" : "box-row" ) + "'>"
                    "<td>"
                        "<div class='album-header' onClick=\"toggleBlock('IDA%1')\">"
                        "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
                        "<tr>"
                            "<td width='1'>"
                                "<a href='fetchcover: @@@ %2'>"
                                "<img class='album-image' align='left' vspace='2' hspace='2' title='%3' src='%4'/>"
                                "</a>"
                            "</td>"
                            "<td valign='middle' align='left'>"
                                "<span class='album-info'>%5</span> "
                                "<a href='compilation:%6'><span class='album-title'>%7</span></a>"
                                "<br />"
                                "<span class='album-year'>%8</span>"
                            "</td>"
                        "</tr>"
                        "</table>"
                        "</div>"
                        "<div class='album-body' style='display:%9;' id='IDA%10'>" )
                    .args( QStringList()
                        << values[ i + 1 ]
                        << escapeHTMLAttr( values[ i ].isEmpty() ? i18n( "Unknown" ) : values[ i ] ) // album.name
                        << i18n( "Click for information from amazon.com, right-click for menu." )
                        << escapeHTMLAttr( albumImage )
                        << i18n( "Single", "%n Tracks",  albumValues.count() / qb.countReturnValues() )
                        << values[ i + 1 ] //album.id
                        << escapeHTML( values[ i ].isEmpty() ? i18n( "Unknown" ) : values[ i ] )
                        << albumYear
                        << ( i!=vectorPlace ? "none" : "block" ) /* shows it if it's the current track album */
                        << values[ i + 1 ] ) );

                if ( !albumValues.isEmpty() )
                    for ( uint j = 0; j < albumValues.count(); j += qb.countReturnValues() )
                    {
                        QString track = albumValues[j + 2].stripWhiteSpace();
                        if( track.length() > 0 ) {
                            if( track.length() == 1 )
                                track.prepend( "0" );

                            track = "<span class='album-song-trackno'>" + track + "&nbsp;</span>";
                        }

                        QString length;
                        if( albumValues[j + 4] != "0" )
                            length = "<span class='album-song-time'>(" + MetaBundle::prettyTime( QString(albumValues[j + 4]).toInt(), false ) + ")</span>";
                        m_HTMLSource.append(
                            "<div class='album-song'>"
                                "<a href=\"file:" + escapeHTMLAttr( albumValues[j + 1] ) + "\">"
                                + track +
                        "<span class='album-song-title'>" + escapeHTML( albumValues[j + 5] ) + i18n(" - ") + escapeHTML( albumValues[j] ) + "</span>&nbsp;"
                                + length +
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
    // </Compilations with this artist>
    }
    m_HTMLSource.append( "</html>" );

    return true;
}


void ContextBrowser::setStyleSheet()
{
    DEBUG_FUNC_INFO

    QString themeName = AmarokConfig::contextBrowserStyleSheet().latin1();
    const QString file = kapp->dirs()->findResource( "data","amarok/themes/" + themeName + "/stylesheet.css" );

    if ( themeName != "Default" && QFile::exists( file ) )
        setStyleSheet_ExternalStyle( m_styleSheet, themeName );
    else
        setStyleSheet_Default( m_styleSheet );

    m_homePage->setUserStyleSheet( m_styleSheet );
    m_currentTrackPage->setUserStyleSheet( m_styleSheet );
    m_lyricsPage->setUserStyleSheet( m_styleSheet );
    m_wikiPage->setUserStyleSheet( m_styleSheet );
}


void ContextBrowser::setStyleSheet_Default( QString& styleSheet )
{
    //colorscheme/font dependant parameters
    int pxSize = fontMetrics().height() - 4;
    const QString fontFamily = AmarokConfig::useCustomFonts() ? AmarokConfig::contextBrowserFont().family() : QApplication::font().family();
    const QString text = colorGroup().text().name();
    const QString link = colorGroup().link().name();
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
    styleSheet = QString( "body { margin: 4px; font-size: %1px; color: %2; background-color: %3; background-image: url( %4 ); background-repeat: repeat; font-family: %5; }" )
            .arg( pxSize )
            .arg( text )
            .arg( AmarokConfig::schemeAmarok() ? fg : gradientColor.name() )
            .arg( m_bgGradientImage->name() )
            .arg( fontFamily );

    //text attributes
    styleSheet += QString( "h1 { font-size: %1px; }" ).arg( pxSize + 8 );
    styleSheet += QString( "h2 { font-size: %1px; }" ).arg( pxSize + 6 );
    styleSheet += QString( "h3 { font-size: %1px; }" ).arg( pxSize + 4 );
    styleSheet += QString( "h4 { font-size: %1px; }" ).arg( pxSize + 3 );
    styleSheet += QString( "h5 { font-size: %1px; }" ).arg( pxSize + 2 );
    styleSheet += QString( "h6 { font-size: %1px; }" ).arg( pxSize + 1 );
    styleSheet += QString( "a { font-size: %1px; color: %2; }" ).arg( pxSize ).arg( text );
    styleSheet += QString( ".info { display: block; margin-left: 4px; font-weight: normal; }" );

    styleSheet += QString( ".song a { display: block; padding: 1px 2px; font-weight: normal; text-decoration: none; }" );
    styleSheet += QString( ".song a:hover { color: %1; background-color: %2; }" ).arg( fg ).arg( bg );
    styleSheet += QString( ".song-title { font-weight: bold; }" );
    styleSheet += QString( ".song-place { font-size: %1px; font-weight: bold; }" ).arg( pxSize + 3 );

    //box: the base container for every block (border hilighted on hover, 'A' without underlining)
    styleSheet += QString( ".box { border: solid %1 1px; text-align: left; margin-bottom: 10px; overflow: hidden;}" ).arg( bg );
    styleSheet += QString( ".box a { text-decoration: none; }" );
    styleSheet += QString( ".box:hover { border: solid %1 1px; }" ).arg( text );

    //box contents: header, body, rows and alternate-rows
    styleSheet += QString( ".box-header { color: %1; background-color: %2; background-image: url( %4 ); background-repeat: repeat-x; font-size: %3px; font-weight: bold; padding: 1px 0.5em; border-bottom: 1px solid #000; }" )
            .arg( fg )
            .arg( bg )
            .arg( pxSize + 2 )
            .arg( m_headerGradientImage->name() );

    styleSheet += QString( ".box-body { padding: 2px; background-color: %1; background-image: url( %2 ); background-repeat: repeat-x; font-size:%3px; }" )
            .arg( colorGroup().base().name() )
            .arg( m_shadowGradientImage->name() )
            .arg( pxSize );

    //"Albums by ..." related styles
    styleSheet += QString( ".album-header:hover { color: %1; background-color: %2; cursor: pointer; }" ).arg( fg ).arg( bg );
    styleSheet += QString( ".album-header:hover a { color: %1; }" ).arg( fg );
    styleSheet += QString( ".album-body { background-color: %1; border-bottom: solid %2 1px; border-top: solid %3 1px; }" ).arg( colorGroup().base().name() ).arg( bg ).arg( bg );
    styleSheet += QString( ".album-title { font-weight: bold; }" );
    styleSheet += QString( ".album-info { float:right; padding-right:4px; font-size: %1px }" ).arg( pxSize );
    styleSheet += QString( ".album-image { padding-right: 4px; }" );
    styleSheet += QString( ".album-song a { display: block; padding: 1px 2px; font-weight: normal; text-decoration: none; }" );
    styleSheet += QString( ".album-song a:hover { color: %1; background-color: %2; }" ).arg( fg ).arg( bg );
    styleSheet += QString( ".album-song-trackno { font-weight: bold; }" );

    styleSheet += QString( ".button { width: 100%; }" );

    //boxes used to display score (sb: score box)
    styleSheet += QString( ".sbtext { text-align: center; padding: 0px 4px; border-left: solid %1 1px; }" ).arg( colorGroup().base().dark( 120 ).name() );
    // New score-bar style from Tightcode
    styleSheet += QString( ".sbinner { width: 40px; height: 10px; background: transparent url(%1) no-repeat top left; border: 0px; border-right: 1px solid transparent; }" )
                           .arg( locate( "data", "amarok/images/sbinner_stars.png" ) );
    styleSheet += QString( ".sbouter { border: 0px; background: transparent url(%1) no-repeat top left; width: 54px; height: 10px; text-align: right; }" )
                           .arg( locate( "data", "amarok/images/back_stars_grey.png" ) );

//     styleSheet += QString( ".sbinner { height: 8px; background-color: %1; border: solid %2 1px; }" ).arg( bg ).arg( fg );
//     styleSheet += QString( ".sbouter { width: 52px; height: 10px; background-color: %1; border: solid %2 1px; }" ).arg( colorGroup().base().dark( 120 ).name() ).arg( bg );


    styleSheet += QString( "#current_box-header-album { font-weight: normal; }" );
    styleSheet += QString( "#current_box-information-td { text-align: right; vertical-align: bottom; padding: 3px; }" );
    styleSheet += QString( "#current_box-largecover-td { text-align: left; width: 100px; padding: 0; vertical-align: bottom; }" );
    styleSheet += QString( "#current_box-largecover-image { padding: 4px; vertical-align: bottom; }" );

    styleSheet += QString( "#wiki_box-body a { color: %1; }" ).arg( link );
    styleSheet += QString( "#wiki_box-body a:hover { text-decoration: underline; }" );
}


void ContextBrowser::setStyleSheet_ExternalStyle( QString& styleSheet, QString& themeName )
{
    //colorscheme/font dependant parameters
    const QString pxSize = QString::number( fontMetrics().height() - 4 );
    const QString fontFamily = AmarokConfig::useCustomFonts() ? AmarokConfig::contextBrowserFont().family() : QApplication::font().family();
    const QString text = colorGroup().text().name();
    const QString link = colorGroup().link().name();
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

    const QString CSSLocation = kapp->dirs()->findResource( "data","amarok/themes/" + themeName + "/stylesheet.css" );

    QFile ExternalCSS( CSSLocation );
    if ( !ExternalCSS.open( IO_ReadOnly ) )
        return;

    QTextStream eCSSts( &ExternalCSS );
    QString tmpCSS = eCSSts.read();
    ExternalCSS.close();

    tmpCSS.replace( "./", KURL::fromPathOrURL( CSSLocation ).directory( false ) );
    tmpCSS.replace( "AMAROK_FONTSIZE-2", pxSize );
    tmpCSS.replace( "AMAROK_FONTSIZE", pxSize );
    tmpCSS.replace( "AMAROK_FONTSIZE+2", pxSize );
    tmpCSS.replace( "AMAROK_FONTFAMILY", fontFamily );
    tmpCSS.replace( "AMAROK_TEXTCOLOR", text );
    tmpCSS.replace( "AMAROK_LINKCOLOR", link );
    tmpCSS.replace( "AMAROK_BGCOLOR", bg );
    tmpCSS.replace( "AMAROK_FGCOLOR", fg );
    tmpCSS.replace( "AMAROK_BASECOLOR", base );
    tmpCSS.replace( "AMAROK_DARKBASECOLOR", colorGroup().base().dark( 120 ).name() );
    tmpCSS.replace( "AMAROK_GRADIENTCOLOR", gradientColor.name() );

    styleSheet += tmpCSS;
}


void ContextBrowser::showIntroduction()
{
    DEBUG_BLOCK

    if ( currentPage() != m_homePage->view() )
    {
        blockSignals( true );
        showPage( m_homePage->view() );
        blockSignals( false );
    }

    // Do we have to rebuild the page? I don't care
    m_homePage->begin();
    m_HTMLSource = QString::null;
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
                    "<div class='info'><p>" +
                    i18n( "This is the Context Browser: "
                          "it shows you contextual information about the currently playing track. "
                          "In order to use this feature of amaroK, you need to build a Collection."
                        ) +
                    "</p></div>"
                    "<div align='center'>"
                    "<input type='button' onClick='window.location.href=\"show:collectionSetup\";' value='" +
                    i18n( "Build Collection..." ) +
                    "'></div><br />"
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
                    "<div class='info'><p>" + i18n( "Please be patient while amaroK scans your music collection. You can watch the progress of this activity in the statusbar." ) + "</p></div>"
                "</div>"
            "</div>"
            "</html>"
                       );

    m_homePage->write( m_HTMLSource );
    m_homePage->end();
    saveHtmlData(); // Send html code to file
}


//////////////////////////////////////////////////////////////////////////////////////////
// Lyrics-Tab
//////////////////////////////////////////////////////////////////////////////////////////

// THE FOLLOWING CODE IS COPYRIGHT BY
// Christian Muehlhaeuser, Seb Ruiz
// <chris at chris.de>, <me at sebruiz.net>
// If I'm violating any copyright or such
// please contact / sue me. Thanks.

void ContextBrowser::showLyrics( const QString &hash )
{
    if ( currentPage() != m_lyricsTab )
    {
        blockSignals( true );
        showPage( m_lyricsTab );
        blockSignals( false );
    }

    m_lyrics = CollectionDB::instance()->getLyrics( EngineController::instance()->bundle().url().path() );

    if ( !m_dirtyLyricsPage || m_lyricJob ) return;

    m_lyricsPage->begin();
    m_lyricsPage->setUserStyleSheet( m_styleSheet );


    //remove all matches to the regExp and the song production type.
    //NOTE: use i18n'd and english equivalents since they are very common int'lly.
    QString replaceMe = " \\([^}]*%1[^}]*\\)";
    QStringList production;
    production << i18n( "live" ) << i18n( "acoustic" ) << i18n( "cover" ) << i18n( "mix" )
               << i18n( "edit" ) << i18n( "medley" ) << i18n( "unplugged" ) << i18n( "bonus" )
               << QString( "live" ) << QString( "acoustic" ) << QString( "cover" ) << QString( "mix" )
               << QString( "edit" ) << QString( "medley" ) << QString( "unplugged" ) << QString( "bonus" );

    QString title  = EngineController::instance()->bundle().title();

    if ( title.isEmpty() ) {
        /* If title is empty, try to use pretty title.
           The fact that it often (but not always) has artist name together, can be bad,
           but at least the user will get nice suggestions. */
        title = EngineController::instance()->bundle().prettyTitle();
    }

    for ( uint x = 0; x < production.count(); ++x )
    {
        QRegExp re = replaceMe.arg( production[x] );
        re.setCaseSensitive( false );
        title.remove( re );
    }

    if ( !hash.isEmpty() && hash != QString( "reload" ) )
        m_lyricCurrentUrl = QString( "http://lyrc.com.ar/en/tema1en.php?hash=%1" )
                  .arg( hash );
    else
        m_lyricCurrentUrl = QString( "http://lyrc.com.ar/en/tema1en.php?artist=%1&songname=%2" )
                .arg(
                KURL::encode_string_no_slash( EngineController::instance()->bundle().artist() ),
                KURL::encode_string_no_slash( title ) );

    debug() << "Using this url: " << m_lyricCurrentUrl << endl;

    m_lyricAddUrl = QString( "http://lyrc.com.ar/en/add/add.php?grupo=%1&tema=%2&disco=%3&ano=%4" ).arg(
            KURL::encode_string_no_slash( EngineController::instance()->bundle().artist() ),
            KURL::encode_string_no_slash( title ),
            KURL::encode_string_no_slash( EngineController::instance()->bundle().album() ),
            KURL::encode_string_no_slash( EngineController::instance()->bundle().year() ) );
    m_lyricSearchUrl = QString( "http://www.google.com/search?ie=UTF-8&q=lyrics %1 %2" )
        .arg( KURL::encode_string_no_slash( '"'+EngineController::instance()->bundle().artist()+'"', 106 /*utf-8*/ ),
              KURL::encode_string_no_slash( '"'+title+'"', 106 /*utf-8*/ ) );

    m_lyricsToolBar->getButton( LYRICS_BROWSER )->setEnabled(false);

    if ( !m_lyrics.isEmpty() && hash.isEmpty() )
    {
        m_HTMLSource = QString (
            "<html>"
            "<div id='lyrics_box' class='box'>"
                "<div id='lyrics_box-header' class='box-header'>"
                    "<span id='lyrics_box-header-title' class='box-header-title'>"
                    + i18n( "Cached Lyrics" ) +
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
        m_lyricJob = NULL;
        saveHtmlData(); // Send html code to file
    }
    else
    {
        m_HTMLSource = QString (
            "<html>"
            "<div id='lyrics_box' class='box'>"
                "<div id='lyrics_box-header' class='box-header'>"
                    "<span id='lyrics_box-header-title' class='box-header-title'>"
                    + i18n( "Fetching Lyrics" ) +
                    "</span>"
                "</div>"
                "<div id='lyrics_box-body' class='box-body'>"
                    "<div class='info'><p>" + i18n( "Fetching Lyrics" ) + "</p></div>"
                "</div>"
            "</div>"
            "</html>"
            );
        m_lyricsPage->write( m_HTMLSource );
        m_lyricsPage->end();
        m_lyricJob = KIO::storedGet( m_lyricCurrentUrl, false, false );

        amaroK::StatusBar::instance()->newProgressOperation( m_lyricJob )
            .setDescription( i18n( "Fetching Lyrics" ) );

        connect( m_lyricJob, SIGNAL( result( KIO::Job* ) ), SLOT( lyricsResult( KIO::Job* ) ) );
    }
}


void
ContextBrowser::lyricsResult( KIO::Job* job ) //SLOT
{
    if ( job != m_lyricJob )
        return; //not the right job, so let's ignore it


    if ( !job->error() == 0 )
    {
        m_lyricsPage->begin();
        m_HTMLSource="";
        m_lyricsPage->setUserStyleSheet( m_styleSheet );

        m_HTMLSource.append(
                "<html>"
                "<div id='lyrics_box' class='box'>"
                    "<div id='lyrics_box-header' class='box-header'>"
                        "<span id='lyrics_box-header-title' class='box-header-title'>"
                        + i18n( "Error" ) +
                        "</span>"
                    "</div>"
                    "<div id='lyrics_box-body' class='box-body'><p>"
                        + i18n( "Lyrics could not be retrieved because the server was not reachable." ) +
                    "</p></div>"
                "</div>"
                "</html>"
                        );
        m_lyricsPage->write( m_HTMLSource );
        m_lyricsPage->end();
        m_dirtyLyricsPage = false;
        m_lyricJob = NULL;
        saveHtmlData(); // Send html code to file

        warning() << "[LyricsFetcher] KIO error! errno: " << job->error() << endl;
        return;
    }

    m_lyrics = QString( static_cast<KIO::StoredTransferJob*>( job )->data() );

    /* We don't want to display any links or images in our lyrics */
    m_lyrics.replace( QRegExp("<[aA][^>]*>[^<]*</[aA]>"), QString::null );
    m_lyrics.replace( QRegExp("<[iI][mM][gG][^>]*>"), QString::null );
    m_lyrics.replace( QRegExp("<[sS][cC][rR][iI][pP][tT][^>]*>[^<]*(<!--[^>]*>)*[^<]*</[sS][cC][rR][iI][pP][tT]>"), QString::null );

    int lyricsPos = m_lyrics.find( QRegExp( "<[fF][oO][nN][tT][ ]*[sS][iI][zZ][eE][ ]*='2'[ ]*>" ) );
    if ( lyricsPos != -1 )
    {
        m_lyrics = m_lyrics.mid( lyricsPos );
        if ( m_lyrics.find( "<p><hr" ) != -1 )
            m_lyrics = m_lyrics.mid( 0, m_lyrics.find( "<p><hr" ) );
        else
            m_lyrics = m_lyrics.mid( 0, m_lyrics.find( "<br /><br />" ) );
        if ( CollectionDB::instance()->isFileInCollection( EngineController::instance()->bundle().url().path() ) )
        {
            debug() << "Writing Lyrics..." << endl;
            CollectionDB::instance()->setLyrics( EngineController::instance()->bundle().url().path(), m_lyrics );
        }
    }
    else if ( m_lyrics.find( "Suggestions : " ) != -1 )
    {
        m_lyrics = m_lyrics.mid( m_lyrics.find( "Suggestions : " ), m_lyrics.find( "<br /><br />" ) );
        showLyricSuggestions();
    }
    else
    {
        m_lyrics = "<div class='info'><p>" + i18n( "Lyrics not found." ) + "</p></div>";
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
    m_lyricsToolBar->getButton( LYRICS_BROWSER )->setEnabled(true);
    m_dirtyLyricsPage = false;
    m_lyricJob = NULL;
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
}


void
ContextBrowser::lyricsExternalPage() //SLOT
{
    kapp->invokeBrowser( m_lyricCurrentUrl );
}


void
ContextBrowser::lyricsAdd() //SLOT
{
    kapp->invokeBrowser( m_lyricAddUrl );
}


void
ContextBrowser::lyricsSearch() //SLOT
{
    kapp->invokeBrowser( m_lyricSearchUrl );
}


void
ContextBrowser::lyricsRefresh() //SLOT
{
    m_dirtyLyricsPage = true;
    showLyrics( "reload" );
}


//////////////////////////////////////////////////////////////////////////////////////////
// Wikipedia-Tab
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::showWikipedia( const QString &url, bool fromHistory )
{
    if ( currentPage() != m_wikiTab )
    {
        blockSignals( true );
        showPage( m_wikiTab );
        blockSignals( false );
    }
    if ( !m_dirtyWikiPage || m_wikiJob ) return;

    // Disable the Open in a Brower button, because while loading it would open wikipedia main page.
    m_wikiToolBar->setItemEnabled( WIKI_BROWSER, false );

    m_wikiPage->begin();
    m_HTMLSource="";
    m_wikiPage->setUserStyleSheet( m_styleSheet );

    m_HTMLSource.append(
            "<html>"
            "<div id='wiki_box' class='box'>"
                "<div id='wiki_box-header' class='box-header'>"
                    "<span id='wiki_box-header-title' class='box-header-title'>"
                    + i18n( "Wikipedia" ) +
                    "</span>"
                "</div>"
                "<div id='wiki_box-body' class='box-body'>"
                    "<div class='info'><p>" + i18n( "Fetching Wikipedia Information" ) + " ...</p></div>"
                "</div>"
            "</div>"
            "</html>"
                    );

    m_wikiPage->write( m_HTMLSource );
    m_wikiPage->end();
    saveHtmlData(); // Send html code to file

    if ( url.isEmpty() )
    {
        QString tmpWikiStr;

        if ( !EngineController::engine()->isStream() )
        {
            if ( !EngineController::instance()->bundle().artist().isEmpty() )
            {
                tmpWikiStr = KURL::encode_string( EngineController::instance()->bundle().artist() );
            }
            else if ( !EngineController::instance()->bundle().title().isEmpty() )
            {
                tmpWikiStr = KURL::encode_string( EngineController::instance()->bundle().title() );
            }
            else
            {
                tmpWikiStr = KURL::encode_string( EngineController::instance()->bundle().prettyTitle() );
            }
        }
        else
        {
            tmpWikiStr = KURL::encode_string( EngineController::instance()->bundle().prettyTitle() );
        }

        m_wikiCurrentUrl = QString( "http://en.wikipedia.org/wiki/%1" ).arg( tmpWikiStr );
    }
    else
    {
        m_wikiCurrentUrl = url;
    }

    // Remove all items from the button-menus
    m_wikiBackPopup->clear();
    m_wikiForwardPopup->clear();

    // Populate button menus with URLs from the history
    QStringList::ConstIterator it;
    uint count;
    // Reverse iterate over both lists
    count = m_wikiBackHistory.count();
    for ( it = m_wikiBackHistory.fromLast(); count > 0; --count, --it )
        m_wikiBackPopup->insertItem( SmallIconSet( "wiki" ), *it, count - 1 );
    count = m_wikiForwardHistory.count();
    for ( it = m_wikiForwardHistory.fromLast(); count > 0; --count, --it )
        m_wikiForwardPopup->insertItem( SmallIconSet( "wiki" ), *it, count - 1 );

    // Append new URL to history
    if ( !fromHistory ) {
        m_wikiBackHistory += m_wikiCurrentUrl;
        m_wikiForwardHistory.clear();
    }
    // Limit number of items in history
    if ( m_wikiBackHistory.count() > WIKI_MAX_HISTORY )
        m_wikiBackHistory.pop_front();

    debug() << "WIKI BACK-HISTORY SIZE   : " << m_wikiBackHistory.size() << endl;
    debug() << "WIKI FORWARD-HISTORY SIZE: " << m_wikiForwardHistory.size() << endl;
    m_wikiToolBar->setItemEnabled( WIKI_BACK, m_wikiBackHistory.size() > 1 );
    m_wikiToolBar->setItemEnabled( WIKI_FORWARD, m_wikiForwardHistory.size() > 0 );

    m_wikiBaseUrl = m_wikiCurrentUrl.mid(0 , m_wikiCurrentUrl.find("wiki/"));
    m_wikiJob = KIO::storedGet( m_wikiCurrentUrl, false, false );

    amaroK::StatusBar::instance()->newProgressOperation( m_wikiJob )
            .setDescription( i18n( "Fetching Wikipedia Information" ) );

    connect( m_wikiJob, SIGNAL( result( KIO::Job* ) ), SLOT( wikiResult( KIO::Job* ) ) );
}


void
ContextBrowser::wikiHistoryBack() //SLOT
{
    m_wikiForwardHistory += m_wikiBackHistory.last();
    m_wikiBackHistory.pop_back();

    m_dirtyWikiPage = true;
    showWikipedia( m_wikiBackHistory.last(), true );
}


void
ContextBrowser::wikiHistoryForward() //SLOT
{
    const QString url = m_wikiForwardHistory.last();
    m_wikiBackHistory += url;
    m_wikiForwardHistory.pop_back();

    m_dirtyWikiPage = true;
    showWikipedia( url, true );
}


void
ContextBrowser::wikiBackPopupActivated( int id ) //SLOT
{
    m_dirtyWikiPage = true;
    showWikipedia( m_wikiBackHistory[id] );
}


void
ContextBrowser::wikiForwardPopupActivated( int id ) //SLOT
{
    m_dirtyWikiPage = true;
    showWikipedia( m_wikiForwardHistory[id] );
}


void
ContextBrowser::wikiArtistPage() //SLOT
{
    m_dirtyWikiPage = true;
    showWikipedia(); // Will fall back to title, if artist is empty(streams!).
}


void
ContextBrowser::wikiAlbumPage() //SLOT
{
    m_dirtyWikiPage = true;
    showWikipedia( QString( "http://en.wikipedia.org/wiki/%1" )
        .arg( KURL::encode_string_no_slash( EngineController::instance()->bundle().album() ) ) );
}


void
ContextBrowser::wikiTitlePage() //SLOT
{
    m_dirtyWikiPage = true;
    showWikipedia( QString( "http://en.wikipedia.org/wiki/%1" )
        .arg( KURL::encode_string_no_slash( EngineController::instance()->bundle().title() ) ) );
}


void
ContextBrowser::wikiExternalPage() //SLOT
{
    kapp->invokeBrowser( m_wikiCurrentUrl );
}


void
ContextBrowser::wikiResult( KIO::Job* job ) //SLOT
{
    DEBUG_BLOCK

    if ( !job->error() == 0 )
    {
        m_wikiPage->begin();
        m_HTMLSource="";
        m_wikiPage->setUserStyleSheet( m_styleSheet );

    m_HTMLSource.append(
            "<div id='wiki_box' class='box'>"
                "<div id='wiki_box-header' class='box-header'>"
                    "<span id='wiki_box-header-title' class='box-header-title'>"
                    + i18n( "Error" ) +
                    "</span>"
                "</div>"
                "<div id='wiki_box-body' class='box-body'><p>"
                    + i18n( "Artist information could not be retrieved because the server was not reachable." ) +
                "</p></div>"
            "</div>"
                        );
        m_wikiPage->write( m_HTMLSource );
        m_wikiPage->end();
        m_dirtyWikiPage = false;
        m_wikiPage = NULL;
        saveHtmlData(); // Send html code to file

        warning() << "[WikiFetcher] KIO error! errno: " << job->error() << endl;
        return;
    }
    if ( job != m_wikiJob )
        return; //not the right job, so let's ignore it

    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    m_wiki = QString( storedJob->data() );

    // Enable the Open in a Brower button, Disabled while loading, guz it would open wikipedia main page.
    m_wikiToolBar->setItemEnabled( WIKI_BROWSER, true );

    // FIXME: Get a safer Regexp here, to match only inside of <head> </head> at least.
    if ( m_wiki.contains( "charset=utf-8"  ) ) {
         m_wiki = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );
    }

    //remove the new-lines and tabs(replace with spaces IS needed).
    m_wiki.replace( "\n", " " );
    m_wiki.replace( "\t", " " );

    m_wikiLanguages = QString::null;
    // Get the avivable language list
    if ( m_wiki.find("<div id=\"p-lang\" class=\"portlet\">") != -1 )
    {
        m_wikiLanguages = m_wiki.mid( m_wiki.find("<div id=\"p-lang\" class=\"portlet\">") );
        m_wikiLanguages = m_wikiLanguages.mid( m_wikiLanguages.find("<ul>") );
        m_wikiLanguages = m_wikiLanguages.mid( 0, m_wikiLanguages.find( "</div>" ) );
    }

    QString copyright;
    QString copyrightMark = "<li id=\"f-copyright\">";
    if ( m_wiki.find( copyrightMark ) != -1 )
    {
        copyright = m_wiki.mid( m_wiki.find(copyrightMark) + copyrightMark.length() );
        copyright = copyright.mid( 0, copyright.find( "</li>" ) );
        copyright.replace( "<br />", QString::null );
        //only one br at the beginning
        copyright.prepend( "<br />" );
    }

    // Ok lets remove the top and bottom parts of the page
    m_wiki = m_wiki.mid( m_wiki.find( "<h1 class=\"firstHeading\">" ) );
    m_wiki = m_wiki.mid( 0, m_wiki.find( "<div class=\"printfooter\">" ) );
    // Adding back license information
    m_wiki += copyright;
    m_wiki.append( "</div>" );
    m_wiki.replace( QRegExp("<h3 id=\"siteSub\">[^<]*</h3>"), QString::null );

    m_wiki.replace( QRegExp( "<div class=\"editsection\"[^>]*>[^<]*<[^>]*>[^<]*<[^>]*>[^<]*</div>" ), QString::null );

    m_wiki.replace( QRegExp( "<a href=\"[^\"]*\" class=\"new\"[^>]*>([^<]*)</a>" ), "\\1" );

    // Remove anything inside of a class called urlexpansion, as it's pointless for us
    m_wiki.replace( QRegExp( "<span class= *'urlexpansion'>[^(]*[(][^)]*[)]</span>" ), QString::null );

    // we want to keep our own style (we need to modify the stylesheet a bit to handle things nicely)
    m_wiki.replace( QRegExp( "style= *\"[^\"]*\"" ), QString::null );
    m_wiki.replace( QRegExp( "class= *\"[^\"]*\"" ), QString::null );
    // let's remove the form elements, we don't want them.
    m_wiki.replace( QRegExp( "<input[^>]*>" ), QString::null );
    m_wiki.replace( QRegExp( "<select[^>]*>" ), QString::null );
    m_wiki.replace( "</select>" , QString::null );
    m_wiki.replace( QRegExp( "<option[^>]*>" ), QString::null );
    m_wiki.replace( "</option>" , QString::null );
    m_wiki.replace( QRegExp( "<textarea[^>]*>" ), QString::null );
    m_wiki.replace( "</textarea>" , QString::null );

    //first we convert all the links with protocol to external, as they should all be External Links.
    m_wiki.replace( QRegExp( "href= *\"http:" ), "href=\"externalurl:" );
    m_wiki.replace( QRegExp( "href= *\"/" ), "href=\"" +m_wikiBaseUrl );
    m_wiki.replace( QRegExp( "href= *\"#" ), "href=\"" +m_wikiCurrentUrl + "#" );

    m_wikiPage->begin();
    m_wikiPage->setUserStyleSheet( m_styleSheet );

    m_HTMLSource = "<html><body>";
    m_HTMLSource.append(
            "<div id='wiki_box' class='box'>"
                "<div id='wiki_box-header' class='box-header'>"
                    "<span id='wiki_box-header-title' class='box-header-title'>"
                    + i18n( "Wikipedia Information" ) +
                    "</span>"
                "</div>"
                "<div id='wiki_box-body' class='box-body'>"
                    + m_wiki +
                "</div>"
            "</div>"
                       );
    if ( !m_wikiLanguages.isEmpty() )
    {
        m_HTMLSource.append(
                "<div id='wiki_box' class='box'>"
                    "<div id='wiki_box-header' class='box-header'>"
                        "<span id='wiki_box-header-title' class='box-header-title'>"
                        + i18n( "Wikipedia Other Languages" ) +
                        "</span>"
                    "</div>"
                    "<div id='wiki_box-body' class='box-body'>"
                        + m_wikiLanguages +
                    "</div>"
                "</div>"
        );
    }
    m_HTMLSource.append( "</body></html>" );
    m_wikiPage->write( m_HTMLSource );
    m_wikiPage->end();
    m_dirtyWikiPage = false;
    saveHtmlData(); // Send html code to file
    m_wikiJob = NULL;
}


void
ContextBrowser::coverFetched( const QString &artist, const QString &album ) //SLOT
{
    const MetaBundle &currentTrack = EngineController::instance()->bundle();
    if ( currentTrack.artist().isEmpty() && currentTrack.album().isEmpty() )
        return;

    if( currentPage() == m_homePage->view() )
    {
        m_dirtyHomePage = true;
        showHome();
    }
    else if ( currentPage() == m_currentTrackPage->view() &&
       ( currentTrack.artist() == artist || currentTrack.album() == album ) ) // this is for compilations or artist == ""
    {
        if( currentPage() == m_currentTrackPage->view() )
        {
            m_dirtyCurrentTrackPage = true;
            showCurrentTrack();
        }
    }
}


void
ContextBrowser::coverRemoved( const QString &artist, const QString &album ) //SLOT
{
    const MetaBundle &currentTrack = EngineController::instance()->bundle();
    if ( currentTrack.artist().isEmpty() && currentTrack.album().isEmpty() )
        return;

    if( currentPage() == m_homePage->view() )
    {
        m_dirtyHomePage = true;
        showHome();
    }
    else if ( currentPage() == m_currentTrackPage->view() &&
       ( currentTrack.artist() == artist || currentTrack.album() == album ) ) // this is for compilations or artist == ""
    {
        if( currentPage() == m_currentTrackPage->view() )
        {
            m_dirtyCurrentTrackPage = true;
            showCurrentTrack();
        }
    }
}


void
ContextBrowser::similarArtistsFetched( const QString &artist ) //SLOT
{
    if ( EngineController::instance()->bundle().artist() == artist ) {
        m_dirtyCurrentTrackPage = true;
        if ( currentPage() == m_currentTrackPage->view() )
            showCurrentTrack();
    }
}

void ContextBrowser::tagsChanged( const MetaBundle &bundle ) //SLOT
{
    const MetaBundle &currentTrack = EngineController::instance()->bundle();

    if( currentTrack.artist().isEmpty() && currentTrack.album().isEmpty() )
        return;

    if( bundle.artist() != currentTrack.artist() && bundle.album() != currentTrack.album() )
        return;

    if( currentPage() == m_homePage->view() )
    {
        m_dirtyHomePage = true;
        showHome();
    }
    else if ( currentPage() == m_currentTrackPage->view() ) // this is for compilations or artist == ""
    {
        if( currentPage() == m_currentTrackPage->view() )
        {
            m_dirtyCurrentTrackPage = true;
            showCurrentTrack();
        }
    }
}


QString
ContextBrowser::makeShadowedImage( const QString& albumImage ) //static
{
    const QImage original( albumImage );
    QImage shadow;
    const uint shadowSize = static_cast<uint>( original.width() / 100.0 * 6.0 );

    const QString folder = amaroK::saveLocation( "covershadow-cache/" );
    const QString file = QString( "shadow_albumcover%1x%2.png" ).arg( original.width() + shadowSize ).arg( original.height() + shadowSize  );
    if ( QFile::exists( folder + file ) )
        shadow.load( folder + file );
    else {
        shadow.load( locate( "data", "amarok/images/shadow_albumcover.png" ) );
        shadow = shadow.smoothScale( original.width() + shadowSize, original.height() + shadowSize );
        shadow.save( folder + file, "PNG" );
    }

    QImage target( shadow );
    bitBlt( &target, 0, 0, &original );

    QByteArray ba;
    QBuffer buffer( ba );
    buffer.open( IO_WriteOnly );
    target.save( &buffer, "PNG" ); // writes image into ba in PNG format

    return QString("data:image/png;base64,%1").arg( KCodecs::base64Encode( ba ) );
}


#include "contextbrowser.moc"
