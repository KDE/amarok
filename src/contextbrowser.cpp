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
#include "contextbrowser.h"
#include "coverfetcher.h"
#include "covermanager.h"
#include "cuefile.h"
#include "enginecontroller.h"
#include "htmlview.h"
#include "mediabrowser.h"
#include "metabundle.h"
#include "playlist.h"      //appendMedia()
#include "qstringx.h"
#include "statusbar.h"
#include "statistics.h"
#include "tagdialog.h"
#include "threadweaver.h"

#include <qdatetime.h>
#include <qimage.h>
#include <qregexp.h>
#include <qtextstream.h>  // External CSS reading
#include <qvbox.h> //wiki tab

#include <kapplication.h> //kapp
#include <kcalendarsystem.h>  // for amaroK::verboseTimeSince()
#include <kconfig.h> // suggested/related/favourite box visibility
#include <kfiledialog.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <klocale.h>
#include <kmdcodec.h> // for data: URLs
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h>
#include <ktoolbarbutton.h>
#include <kurl.h>

#define escapeHTML(s)     QString(s).replace( "&", "&amp;" ).replace( "<", "&lt;" ).replace( ">", "&gt;" )
// .replace( "%", "%25" ) has to be the last one, otherwise we would do things like converting spaces into %20 and then convert them into %25%20
#define escapeHTMLAttr(s) QString(s).replace( "'", "%27" ).replace( "#", "%23" ).replace( "?", "%3F" ).replace( "%", "%25" )
#define unEscapeHTMLAttr(s) QString(s).replace( "%25", "%" ).replace( "%3F", "?" ).replace( "%23", "#" ).replace( "%27", "'" )

namespace amaroK
{

    QString verboseTimeSince( const QDateTime &datetime )
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

        if( datediff == -1 )
            return i18n( "Tomorrow" );

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

    QString verboseTimeSince( uint time_t )
    {
        if( !time_t )
            return i18n( "Never" );

        QDateTime dt;
        dt.setTime_t( time_t );
        return verboseTimeSince( dt );
    }

    extern KConfig *config( const QString& );
}


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
        , m_dirtyCurrentTrackPage( true )
        , m_dirtyLyricsPage( true )
        , m_dirtyWikiPage( true )
        , m_emptyDB( CollectionDB::instance()->isEmpty() )
        , m_lyricJob( NULL )
        , m_wikiBackPopup( new KPopupMenu( this ) )
        , m_wikiForwardPopup( new KPopupMenu( this ) )
        , m_wikiJob( NULL )
        , m_relatedOpen( true )
        , m_suggestionsOpen( true )
        , m_favouritesOpen( true )
        , m_browseArtists( false )
        , m_cuefile( NULL )
{
    s_instance = this;

    m_currentTrackPage = new HTMLView( this, "current_track_page", true /* DNDEnabled */ );

    m_lyricsTab = new QVBox(this, "lyrics_tab");

    m_lyricsToolBar = new Browser::ToolBar( m_lyricsTab );
    m_lyricsToolBar->insertButton( "edit_add", LYRICS_ADD, true, i18n("Add Lyrics") );
    m_lyricsToolBar->insertButton( "find", LYRICS_SEARCH, true, i18n("Search For Lyrics") );
    m_lyricsToolBar->insertButton( "reload", LYRICS_REFRESH, true, i18n("Refresh") );
    m_lyricsToolBar->insertLineSeparator();
    m_lyricsToolBar->insertButton( "exec", LYRICS_BROWSER, true, i18n("Open in external browser") );

    m_lyricsPage = new HTMLView( m_lyricsTab, "lyrics_page", true /* DNDEnabled */, false /* JScriptEnabled */ );

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

    m_wikiPage = new HTMLView( m_wikiTab, "wiki_page", true /* DNDEnabled */ );

    m_cuefile = CueFile::instance();
    connect( m_cuefile, SIGNAL(metaData( const MetaBundle& )),
             EngineController::instance(), SLOT(slotStreamMetaData( const MetaBundle& )) );

    addTab( m_currentTrackPage->view(), SmallIconSet( "today" ),    i18n( "Music" ) );
    addTab( m_lyricsTab,                SmallIconSet( "document" ), i18n( "Lyrics" ) );
    addTab( m_wikiTab,                  SmallIconSet( "personal" ),     i18n( "Artist" ) );

    setTabEnabled( m_lyricsTab, false );
    setTabEnabled( m_wikiTab, false );

    m_showRelated   = amaroK::config( "ContextBrowser" )->readBoolEntry( "ShowRelated", true );
    m_showSuggested = amaroK::config( "ContextBrowser" )->readBoolEntry( "ShowSuggested", true );
    m_showFaves     = amaroK::config( "ContextBrowser" )->readBoolEntry( "ShowFaves", true );

    // Delete folder with the cached coverimage shadow pixmaps
    KIO::del( KURL::fromPathOrURL( amaroK::saveLocation( "covershadow-cache/" ) ), false, false );

    connect( this, SIGNAL( currentChanged( QWidget* ) ), SLOT( tabChanged( QWidget* ) ) );

    connect( m_currentTrackPage->browserExtension(), SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs & ) ),
             this,                                   SLOT( openURLRequest( const KURL & ) ) );
    connect( m_lyricsPage->browserExtension(),       SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs & ) ),
             this,                                   SLOT( openURLRequest( const KURL & ) ) );
    connect( m_wikiPage->browserExtension(),         SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs & ) ),
             this,                                   SLOT( openURLRequest( const KURL & ) ) );

    connect( m_currentTrackPage, SIGNAL( popupMenu( const QString&, const QPoint& ) ),
             this,               SLOT( slotContextMenu( const QString&, const QPoint& ) ) );
    connect( m_lyricsPage,       SIGNAL( popupMenu( const QString&, const QPoint& ) ),
             this,               SLOT( slotContextMenu( const QString&, const QPoint& ) ) );
    connect( m_wikiPage,         SIGNAL( popupMenu( const QString&, const QPoint& ) ),
             this,               SLOT( slotContextMenu( const QString&, const QPoint& ) ) );

    connect( m_lyricsToolBar->getButton( LYRICS_ADD ),     SIGNAL(clicked( int )), SLOT(lyricsAdd()) );
    connect( m_lyricsToolBar->getButton( LYRICS_SEARCH ),  SIGNAL(clicked( int )), SLOT(lyricsSearch()) );
    connect( m_lyricsToolBar->getButton( LYRICS_REFRESH ), SIGNAL(clicked( int )), SLOT(lyricsRefresh()) );
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
             this, SLOT( coverFetched( const QString&, const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( coverChanged( const QString&, const QString& ) ),
             this, SLOT( coverRemoved( const QString&, const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( similarArtistsFetched( const QString& ) ),
             this, SLOT( similarArtistsFetched( const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( tagsChanged( const MetaBundle& ) ),
             this, SLOT( tagsChanged( const MetaBundle& ) ) );
}


ContextBrowser::~ContextBrowser()
{
    m_cuefile->clear();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::setFont( const QFont &newFont )
{
    QWidget::setFont( newFont );
    reloadStyleSheet();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::openURLRequest( const KURL &url )
{
    QString artist, album, track;
    albumArtistTrackFromUrl( url.path(), artist, album, track );

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

    else if ( url.protocol() == "show" )
    {
        if ( url.path().contains( "suggestLyric-" ) )
        {
            m_lyrics = QString::null;
            QString hash = url.path().mid( url.path().find( QString( "-" ) ) +1 );
            m_dirtyLyricsPage = true;
            showLyrics( hash );
        }
        else if ( url.path() == "statistics" )
        {
            if( Statistics::instance() ) {
                Statistics::instance()->raise();
                return;
            }
            Statistics dialog;
            dialog.exec();
        }
        else if ( url.path() == "collectionSetup" )
        {
            CollectionView::instance()->setupDirs();
        }
        // Konqueror sidebar needs these
        if (url.path() == "context") { m_dirtyCurrentTrackPage=true; showCurrentTrack(); saveHtmlData(); }
        if (url.path() == "wiki") { m_dirtyLyricsPage=true; showWikipedia(); saveHtmlData(); }
        if (url.path() == "lyrics") { m_dirtyWikiPage=true; m_wikiJob=false; showLyrics(); saveHtmlData(); }
    }

    // When left-clicking on cover image, open browser with amazon site
    else if ( url.protocol() == "fetchcover" )
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
    else if ( url.protocol() == "musicbrainz" )
    {
        const QString url = "http://www.musicbrainz.org/taglookup.html?artist=%1&album=%2&track=%3";
        kapp->invokeBrowser( url.arg( KURL::encode_string_no_slash( artist, 106 /*utf-8*/ ),
        KURL::encode_string_no_slash( album, 106 /*utf-8*/ ),
        KURL::encode_string_no_slash( track, 106 /*utf-8*/ ) ) );
    }

    else if ( url.protocol() == "externalurl" )
        kapp->invokeBrowser( url.url().replace("externalurl:", "http:") );

    else if ( url.protocol() == "togglebox" )
    {
        if ( url.path() == "ra" ) m_relatedOpen ^= true;
        if ( url.path() == "ss" ) m_suggestionsOpen ^= true;
        if ( url.path() == "ft" ) m_favouritesOpen ^= true;
    }

    else if ( url.protocol() == "seek" )
    {
        EngineController::engine()->seek(url.path().toLong());
    }

    // browse albums of a related artist
    else if ( url.protocol() == "artist" )
    {
        m_browseArtists = true;
        m_artist = unEscapeHTMLAttr( url.path() );
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
    }

    // back to context for current track
    else if ( url.protocol() == "current" )
    {
        m_browseArtists = false;
        m_artist = QString::null;
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
    }

    else
        HTMLView::openURLRequest( url );
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


void ContextBrowser::lyricsChanged( const QString &url ) {
    if ( url == EngineController::instance()->bundle().url().path() ) {
        m_dirtyLyricsPage = true;
        if ( currentPage() == m_lyricsTab )
            showLyrics();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::engineNewMetaData( const MetaBundle& bundle, bool trackChanged )
{
    bool newMetaData = false;
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

    if ( currentPage() == m_currentTrackPage->view() && ( bundle.url() != m_currentURL || newMetaData || !trackChanged ) )
        showCurrentTrack();
    else if ( currentPage() == m_lyricsTab )
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
        .replace("<html>",QString("<html><head><style type=\"text/css\">%1</style></head>").arg( HTMLView::loadStyleSheet() ) ); // and the stylesheet code
    exportedDocument.close();
}


void ContextBrowser::paletteChange( const QPalette& pal )
{
    KTabWidget::paletteChange( pal );
    reloadStyleSheet();
}

void ContextBrowser::reloadStyleSheet()
{
    m_currentTrackPage->setUserStyleSheet( HTMLView::loadStyleSheet() );
    m_lyricsPage->setUserStyleSheet( HTMLView::loadStyleSheet() );
    m_wikiPage->setUserStyleSheet( HTMLView::loadStyleSheet() );
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
    if ( m_dirtyCurrentTrackPage && ( page == m_currentTrackPage->view() ) )
        showCurrentTrack();
    else if ( m_dirtyLyricsPage && ( page == m_lyricsTab ) )
        showLyrics();
    else if ( m_dirtyWikiPage && ( page == m_wikiTab ) )
        showWikipedia();
}


void ContextBrowser::slotContextMenu( const QString& urlString, const QPoint& point )
{
    enum { APPEND, ASNEXT, MAKE, MEDIA_DEVICE, INFO, TITLE, RELATED, SUGGEST, FAVES };

    if( urlString.startsWith( "musicbrainz" ) ||
        urlString.startsWith( "externalurl" ) ||
        urlString.startsWith( "show:suggest" ) ||
        urlString.startsWith( "http" ) ||
        urlString.startsWith( "seek" ) ||
        urlString.startsWith( "artist" ) ||
        urlString.startsWith( "current" )
        )
        return;

    KURL url( urlString );

    KPopupMenu menu;
    KURL::List urls( url );
    QString artist, album, track; // track unused here
    albumArtistTrackFromUrl( url.path(), artist, album, track );

    if( urlString.isEmpty() )
    {
        menu.setCheckable( true );
        menu.insertItem( i18n("Show Related Artists"), RELATED );
        menu.insertItem( i18n("Show Suggested Songs"), SUGGEST );
        menu.insertItem( i18n("Show Favorite Tracks"), FAVES );

        menu.setItemChecked( RELATED, m_showRelated );
        menu.setItemChecked( SUGGEST, m_showSuggested );
        menu.setItemChecked( FAVES,   m_showFaves );
    }
    else if( url.protocol() == "fetchcover" )
    {
        amaroK::coverContextMenu( this, point, artist, album );
        return;
    }
    else if( url.protocol() == "file" || url.protocol() == "album" || url.protocol() == "compilation" )
    {
        //TODO it would be handy and more usable to have this menu under the cover one too

        menu.insertTitle( i18n("Track"), TITLE );
        menu.insertItem( SmallIconSet( "fileopen" ), i18n( "&Load" ), MAKE );
        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( SmallIconSet( "2rightarrow" ), i18n( "&Queue Track" ), ASNEXT );
        if( MediaBrowser::isAvailable() )
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

    case RELATED:
        m_showRelated = !menu.isItemChecked( RELATED );
        amaroK::config( "ContextBrowser" )->writeEntry( "ShowRelated", m_showRelated );
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
        break;

    case SUGGEST:
        m_showSuggested = !menu.isItemChecked( SUGGEST );
        amaroK::config( "ContextBrowser" )->writeEntry( "ShowSuggested", m_showSuggested );
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
        break;

    case FAVES:
        m_showFaves = !menu.isItemChecked( FAVES );
        amaroK::config( "ContextBrowser" )->writeEntry( "ShowFaves", m_showFaves );
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
        break;

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

    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// Shows the statistics summary when no track is playing
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::showHome() //SLOT
{
    DEBUG_BLOCK

    if ( currentPage() != m_currentTrackPage->view() ) {
        blockSignals( true );
        showPage( m_currentTrackPage->view() );
        blockSignals( false );
    }

    QueryBuilder qb;
    qb.clear();
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valAccessDate, true );
    qb.setLimit( 0, 10 );
    QStringList lastplayed = qb.run();

    qb.clear(); //Song count
    qb.addReturnFunctionValue(QueryBuilder::funcCount, QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    QStringList a = qb.run();
    QString songCount = a[0];

    qb.clear(); //Artist count
    qb.addReturnFunctionValue(QueryBuilder::funcCount, QueryBuilder::tabArtist, QueryBuilder::valID );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();
    QString artistCount = a[0];

    qb.clear(); //Album count
    qb.addReturnFunctionValue(QueryBuilder::funcCount, QueryBuilder::tabAlbum, QueryBuilder::valID );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();
    QString albumCount = a[0];

    qb.clear(); //Genre count
    qb.addReturnFunctionValue(QueryBuilder::funcCount, QueryBuilder::tabGenre, QueryBuilder::valID );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();
    QString genreCount = a[0];

    m_HTMLSource="";
    m_HTMLSource.append(
            "<html>"
            "<div id='introduction_box' class='box'>"
                "<div id='introduction_box-header' class='box-header'>"
                    "<span id='introduction_box-header-title' class='box-header-title'>"
                    + i18n( "No Track Playing" ) +
                    "</span>"
                "</div>"
                "<div id='introduction_box-body' class='box-body'>"
                    "<div class='info'><p>" +
                        " " + i18n( "1 Song", "%n Songs", songCount.toInt() ) + "<br>" +
                        " " + i18n( "1 Artist", "%n Artists", artistCount.toInt() ) + "<br>" +
                        " " + i18n( "1 Album", "%n Albums", albumCount.toInt() ) + "<br>" +
                        " " + i18n( "1 Genre", "%n Genres", genreCount.toInt() ) + "<br>" +
                    "</p></div>"
                    "<div align='center'>"
                    "<input type='button' onClick='window.location.href=\"show:statistics\";' value='" +
                    i18n( "Detailed Statistics..." ) +
                    "'></div><br />"
                "</div>"
            "</div>"
                       );

    // <Recent Tracks Information>
    m_HTMLSource.append(
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
                            "<a href=\"file:" + lastplayed[i + 1].replace( '"', QCString( "%22" ) ) + "\">"
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
            "</div></html>");

    // </Recent Tracks Information>
    m_currentTrackPage->set( m_HTMLSource );

    saveHtmlData(); // Send html code to file
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

        b->m_HTMLSource = m_HTMLSource;
        b->m_currentTrackPage->set( m_HTMLSource );
        b->m_dirtyCurrentTrackPage = false;
        b->saveHtmlData(); // Send html code to file
    }
    QString m_HTMLSource;

    ContextBrowser *b;

};


void ContextBrowser::showCurrentTrack() //SLOT
{
    if( !EngineController::engine()->loaded() )
    {
        showHome();
        return;
    }

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

    QString artist;
    if( b->m_browseArtists )
    {
        artist = b->m_artist;
        if( artist == currentTrack.artist() )
        {
            b->m_browseArtists = false;
            b->m_artist = QString::null;
        }
    }
    else
        artist = currentTrack.artist();

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

    if ( !b->m_browseArtists && EngineController::engine()->isStream() )
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

    const uint artist_id = CollectionDB::instance()->artistID( artist );
    const uint album_id  = CollectionDB::instance()->albumID ( currentTrack.album() );

    QueryBuilder qb;
    QStringList values;
    if( b->m_browseArtists )
    {
        // <Artist>
        m_HTMLSource.append(
                QString(
                    "<div id='current_box' class='box'>"
                    "<div id='current_box-header' class='box-header'>"
                    "<span id='current_box-header-artist' class='box-header-title'>%1</span>"
                    "<br />"
                    "<span id='current_box-header-album' class='box-header-title'>%2</span>"
                    "</div>" )
                .arg( escapeHTMLAttr( artist ) )
                .arg( escapeHTMLAttr( i18n( "Browse Artist" ) ) ) );
        m_HTMLSource.append(
                QString(

                        "<table id='current_box-table' class='box-body' width='100%' cellpadding='0' cellspacing='0'>"
                        "<tr>"
                        "<td id='artist-wikipedia'>"
                        "<a id='artist-wikipedia-a' href='http://en.wikipedia.org/wiki/%1'>"
                        + i18n( "Wikipedia Information for %1" ).arg( artist ) +
                        "</a>"
                        "</td>"
                        "</tr>"

                        "<tr>"
                        "<td id='current-track'>"
                        "<a id='current-track-a' href='current://track'>"
                        "%2"
                        "</a>" )
                .arg( escapeHTMLAttr( artist ) )
                .arg( escapeHTMLAttr( i18n( "Context for Current Track" ) ) )
                );

        m_HTMLSource.append(
                "</td>"
                "</tr>"
                "</table>"
                "</div>" );
        // </Artist>
    }
    else
    {
        // <Current Track Information>
        qb.clear();
        qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valCreateDate );
        qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valAccessDate );
        qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valPlayCounter );
        qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
        qb.addMatch( QueryBuilder::tabStats, QueryBuilder::valURL, currentTrack.url().path() );
        values = qb.run();

        //making 2 tables is most probably not the cleanest way to do it, but it works.
        QString albumImageTitleAttr;
        QString albumImage = CollectionDB::instance()->albumImage( currentTrack, 1 );

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
                        i18n( "Last played: %1" ).arg( amaroK::verboseTimeSince( lastPlay ) ),
                        i18n( "First played: %1" ).arg( amaroK::verboseTimeSince( firstPlay ) ) ) );
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
    }



    QStringList relArtists = CollectionDB::instance()->similarArtists( artist, 10 );
    if ( !relArtists.isEmpty() )
    {
        if( ContextBrowser::instance()->m_showRelated )
        {
            // <Related Artists>
            m_HTMLSource.append(
                    "<div id='related_box' class='box'>"
                    "<div id='related_box-header' class='box-header' onClick=\"toggleBlock('T_RA'); window.location.href='togglebox:ra';\" style='cursor: pointer;'>"
                    "<span id='related_box-header-title' class='box-header-title'>"
                    + i18n( "Artists Related to %1" ).arg( artist ) +
                    "</span>"
                    "</div>"
                    "<table class='box-body' id='T_RA' width='100%' border='0' cellspacing='0' cellpadding='1'>" );

            for ( uint i = 0; i < relArtists.count(); i += 1 )
            {
                qb.clear();
                qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
                bool isInCollection = !CollectionDB::instance()->albumListOfArtist( relArtists[i] ).isEmpty();
                m_HTMLSource.append(
                        "<tr class='" + QString( (i % 2) ? "box-row-alt" : "box-row" ) + "'>"
                        "<td class='artist'>"
                        + ( isInCollection
                            ? "<a href=\"artist:" + relArtists[i] + "\">" + relArtists[i] + "</a>"
                            : "<i><a href=\"artist:" + relArtists[i] + "\">" + relArtists[i] + "</a></i>"
                        ) +
                        "</td>"
                        "<td class='sbtext' width='1'>"
                        "<a href=\"http://en.wikipedia.org/wiki/" + relArtists[i] + "\">Wikipedia</a>"
                        "</td>"
                        "<td width='1'></td>"
                        "</tr>" );
            }

            m_HTMLSource.append(
                    "</table>"
                    "</div>" );

            if ( !b->m_relatedOpen )
                m_HTMLSource.append( "<script language='JavaScript'>toggleBlock('T_RA');</script>" );
            // </Related Artists>
        }

        if( ContextBrowser::instance()->m_showSuggested )
        {
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

            // <Suggested Songs>
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
                            "<a href=\"file:" + values[i].replace( '"', QCString( "%22" ) ) + "\">"
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
            // </Suggested Songs>
        }
    }

    QString artistName = artist.isEmpty() ? i18n( "This Artist" ) : escapeHTML( artist );
    if ( !artist.isEmpty() ) {
    // <Favourite Tracks Information>
    if( ContextBrowser::instance()->m_showFaves )
    {
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
                            "<a href=\"file:" + values[i + 1].replace( '"', QCString( "%22" ) ) + "\">"
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

                QString albumImage = CollectionDB::instance()->albumImage( artist, values[ i ], 50 );
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
                        << escapeHTMLAttr( artist ) // artist name
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
                                "<a href=\"file:" + albumValues[j + 1].replace( "\"", QCString( "%22" ) ) + "\">"
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

                QString albumImage = CollectionDB::instance()->albumImage( artist, values[ i ], 50 );
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

                        QString tracktitle_formated;
                        QString tracktitle;
                        tracktitle = escapeHTML( albumValues[j + 5] ) + i18n(" - ") + escapeHTML( albumValues[j] );
                        tracktitle_formated = "<span class='album-song-title'>";
                        if ( currentTrack.artist() == albumValues[j + 5] )
                             tracktitle_formated += "<b>" + tracktitle + "</b>";
                        else
                             tracktitle_formated += tracktitle;
                        tracktitle_formated += "</span>&nbsp;";
                        m_HTMLSource.append(
                            "<div class='album-song'>"
                                "<a href=\"file:" + albumValues[j + 1].replace( "\"", QCString( "%22" ) ) + "\">"
                                + track
                                + tracktitle_formated
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


void ContextBrowser::showIntroduction()
{
    DEBUG_BLOCK

    if ( currentPage() != m_currentTrackPage->view() )
    {
        blockSignals( true );
        showPage( m_currentTrackPage->view() );
        blockSignals( false );
    }

    // Do we have to rebuild the page? I don't care
    m_HTMLSource = QString::null;
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

    m_currentTrackPage->set( m_HTMLSource );
    saveHtmlData(); // Send html code to file
}


void ContextBrowser::showScanning()
{
    if ( currentPage() != m_currentTrackPage->view() )
    {
        blockSignals( true );
        showPage( m_currentTrackPage->view() );
        blockSignals( false );
    }

    // Do we have to rebuild the page? I don't care
    m_HTMLSource="";
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

    m_currentTrackPage->set( m_HTMLSource );
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

    m_lyrics = CollectionDB::instance()->getHTMLLyrics( EngineController::instance()->bundle().url().path() );

    if ( !m_dirtyLyricsPage || m_lyricJob ) return;

    //remove all matches to the regExp and the song production type.
    //NOTE: use i18n'd and english equivalents since they are very common int'lly.
    QString replaceMe = " \\([^}]*%1[^}]*\\)";
    QStringList production;
    production << i18n( "live" ) << i18n( "acoustic" ) << i18n( "cover" ) << i18n( "mix" )
               << i18n( "edit" ) << i18n( "medley" ) << i18n( "unplugged" ) << i18n( "bonus" )
               << i18n( "version" ) << i18n( "feat" )
               << QString( "live" ) << QString( "acoustic" ) << QString( "cover" ) << QString( "mix" )
               << QString( "edit" ) << QString( "medley" ) << QString( "unplugged" ) << QString( "bonus" )
               << QString( "version" ) << QString( "feat" ) ;

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
            KURL::encode_string_no_slash( QString::number( EngineController::instance()->bundle().year() ) ) );
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
        m_lyricsPage->set( m_HTMLSource );

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
        m_lyricsPage->set( m_HTMLSource );
        m_lyricJob = KIO::storedGet( m_lyricCurrentUrl, false, false );

        amaroK::StatusBar::instance()->newProgressOperation( m_lyricJob )
            .setDescription( i18n( "Fetching Lyrics" ) );

        connect( m_lyricJob, SIGNAL( result( KIO::Job* ) ), SLOT( lyricsResult( KIO::Job* ) ) );
    }
}


void
ContextBrowser::lyricsResult( KIO::Job* job ) //SLOT
{
    if ( !job->error() == 0 )
    {
        m_HTMLSource="";
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
        m_lyricsPage->set( m_HTMLSource );

        m_dirtyLyricsPage = false;
        m_lyricJob = NULL;
        saveHtmlData(); // Send html code to file

        warning() << "[LyricsFetcher] KIO error! errno: " << job->error() << endl;
        return;
    }
    if ( job != m_lyricJob )
        return; //not the right job, so let's ignore it

    m_lyrics = QString( static_cast<KIO::StoredTransferJob*>( job )->data() );

    // remove images, links, scripts, and styles
    m_lyrics.replace( QRegExp("<[iI][mM][gG][^>]*>"), QString::null );
    m_lyrics.replace( QRegExp("<[aA][^>]*>[^<]*</[aA]>"), QString::null );
    m_lyrics.replace( QRegExp("<[sS][cC][rR][iI][pP][tT][^>]*>[^<]*(<!--[^>]*>)*[^<]*</[sS][cC][rR][iI][pP][tT]>"), QString::null );
    m_lyrics.replace( QRegExp("<[sS][tT][yY][lL][eE][^>]*>[^<]*(<!--[^>]*>)*[^<]*</[sS][tT][yY][lL][eE]>"), QString::null );

    if ( m_lyrics.find( "<font size='2'>" ) != -1 )
    {
        m_lyrics = m_lyrics.mid( m_lyrics.find( "<font size='2'>" ) );
        if ( m_lyrics.find( "<p><hr" ) != -1 )
            m_lyrics = m_lyrics.mid( 0, m_lyrics.find( "<p><hr" ) );
        else
            m_lyrics = m_lyrics.mid( 0, m_lyrics.find( "<br /><br />" ) );
        if ( CollectionDB::instance()->isFileInCollection( EngineController::instance()->bundle().url().path() ) )
        {
            debug() << "Writing Lyrics..." << endl;
            CollectionDB::instance()->setHTMLLyrics( EngineController::instance()->bundle().url().path(), m_lyrics );
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


    m_HTMLSource="";
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
    m_lyricsPage->set( m_HTMLSource );

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

    m_HTMLSource="";
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

    m_wikiPage->set( m_HTMLSource );
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
        m_HTMLSource="";
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
        m_wikiPage->set( m_HTMLSource );

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
    m_wikiPage->set( m_HTMLSource );

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

    if ( currentPage() == m_currentTrackPage->view() &&
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

    if ( currentPage() == m_currentTrackPage->view() &&
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
    if( artist == m_artist || EngineController::instance()->bundle().artist() == artist ) {
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

    if ( currentPage() == m_currentTrackPage->view() ) // this is for compilations or artist == ""
    {
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
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
