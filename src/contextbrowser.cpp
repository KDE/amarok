// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Reigo Reinmets <xatax@hot.ee>
// (c) 2005 Mark Kretschmann <markey@web.de>
// (c) 2006 Peter C. Ndikuwera <pndiku@gmail.com>
// (c) 2006 Alexandre Pereira de Oliveira <aleprj@gmail.com>
// (c) 2006 Maximilian Kossick <maximilian.kossick@googlemail.com>
// License: GNU General Public License V2


#define DEBUG_PREFIX "ContextBrowser"

#include "amarok.h"
#include "amarokconfig.h"
#include "app.h"
#include "browserToolBar.h"
#include "debug.h"
#include "clicklineedit.h"
#include "collectiondb.h"
#include "collectionbrowser.h"
#include "colorgenerator.h"
#include "contextbrowser.h"
#include "coverfetcher.h"
#include "covermanager.h"
#include "cuefile.h"
#include "enginecontroller.h"
#include "htmlview.h"
#include "lastfm.h"
#include "mediabrowser.h"
#include "metabundle.h"
#include "mountpointmanager.h"
#include "playlist.h"      //appendMedia()
#include "podcastbundle.h"
#include "qstringx.h"
#include "scriptmanager.h"
#include "starmanager.h"
#include "statusbar.h"
#include "tagdialog.h"
#include "threadmanager.h"

#include <qbuffer.h>
#include <qdatetime.h>
#include <qdeepcopy.h>
#include <qdom.h>
#include <qimage.h>
#include <qregexp.h>
#include <qtextstream.h>  // External CSS reading
#include <qvbox.h> //wiki tab
#include <qhbox.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qtimer.h>
#include <qtooltip.h>

#include <kaction.h>
#include <kapplication.h> //kapp
#include <kcalendarsystem.h>  // for Amarok::verboseTimeSince()
#include <kconfig.h> // suggested/related/favorite box visibility
#include <kdialog.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kmdcodec.h> // for data: URLs
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h>
#include <ktextedit.h>
#include <ktoolbarbutton.h>

#include <unistd.h> //usleep()

namespace Amarok
{
    QString escapeHTML( const QString &s )
    {
        return QString(s).replace( "&", "&amp;" ).replace( "<", "&lt;" ).replace( ">", "&gt;" );
        // .replace( "%", "%25" ) has to be the first(!) one, otherwise we would do things like converting spaces into %20 and then convert them into %25%20
    }

    QString escapeHTMLAttr( const QString &s )
    {
        return QString(s).replace( "%", "%25" ).replace( "'", "%27" ).replace( "\"", "%22" ).replace( "#", "%23" ).replace( "?", "%3F" );
    }

    QString unescapeHTMLAttr( const QString &s )
    {
        return QString(s).replace( "%3F", "?" ).replace( "%23", "#" ).replace( "%22", "\"" ).replace( "%27", "'" ).replace( "%25", "%" );
    }

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

    /**
    * Function that must be used when separating contextBrowser escaped urls
    * detail can contain track/discnumber
    */
    void albumArtistTrackFromUrl( QString url, QString &artist, QString &album, QString &detail )
    {
        if ( !url.contains("@@@") ) return;
        //KHTML removes the trailing space!
        if ( url.endsWith( " @@@" ) )
            url += ' ';

        const QStringList list = QStringList::split( " @@@ ", url, true );

        int size = list.count();

        Q_ASSERT( size>0 );

        artist = size > 0 ? unescapeHTMLAttr( list[0] ) : "";
        album  = size > 1 ? unescapeHTMLAttr( list[1] ) : "";
        detail = size > 2 ? unescapeHTMLAttr( list[2] ) : "";
    }

}


using Amarok::QStringx;
using Amarok::escapeHTML;
using Amarok::escapeHTMLAttr;
using Amarok::unescapeHTMLAttr;


static
QString albumImageTooltip( const QString &albumImage, int size )
{
    if ( albumImage == CollectionDB::instance()->notAvailCover( false, size ) )
        return escapeHTMLAttr( i18n( "Click to fetch cover from amazon.%1, right-click for menu." ).arg( CoverManager::amazonTld() ) );

    return escapeHTMLAttr( i18n( "Click for information from Amazon, right-click for menu." ) );
}


ContextBrowser *ContextBrowser::s_instance = 0;
QString ContextBrowser::s_wikiLocale = "en";


ContextBrowser::ContextBrowser( const char *name )
        : KTabWidget( 0, name )
        , EngineObserver( EngineController::instance() )
        , m_dirtyCurrentTrackPage( true )
        , m_dirtyLyricsPage( true )
        , m_dirtyWikiPage( true )
        , m_emptyDB( CollectionDB::instance()->isEmpty() )
        , m_wikiBackPopup( new KPopupMenu( this ) )
        , m_wikiForwardPopup( new KPopupMenu( this ) )
        , m_wikiJob( NULL )
        , m_wikiConfigDialog( NULL )
        , m_relatedOpen( true )
        , m_suggestionsOpen( true )
        , m_favoritesOpen( true )
        , m_labelsOpen( true )
        , m_showFreshPodcasts( true )
        , m_showFavoriteAlbums( true )
        , m_showNewestAlbums( true )
        , m_browseArtists( false )
        , m_browseLabels( false )
        , m_cuefile( NULL )
{
    s_instance = this;
    s_wikiLocale = AmarokConfig::wikipediaLocale();

    m_contextTab = new QVBox(this, "context_tab");

    m_currentTrackPage = new HTMLView( m_contextTab, "current_track_page", true /* DNDEnabled */,
            true /*JScriptEnabled*/ );

    m_lyricsTab = new QVBox(this, "lyrics_tab");

    m_lyricsToolBar = new Browser::ToolBar( m_lyricsTab );
    m_lyricsToolBar->setIconText( KToolBar::IconTextRight, false );
    m_lyricsToolBar->insertButton( Amarok::icon( "refresh" ), LYRICS_REFRESH, true, i18n("Refresh") );
    m_lyricsToolBar->insertButton( Amarok::icon( "add_lyrics" ), LYRICS_ADD, true, i18n("Add") );
    m_lyricsToolBar->insertButton( Amarok::icon( "edit" ), LYRICS_EDIT, true, i18n("Edit") );
    m_lyricsToolBar->setToggle( LYRICS_EDIT, true );
    m_lyricsToolBar->insertButton( Amarok::icon( "search" ), LYRICS_SEARCH, true, i18n("Search") );
    m_lyricsToolBar->setIconText( KToolBar::IconOnly, false );
    m_lyricsToolBar->insertButton( Amarok::icon( "external" ), LYRICS_BROWSER, true, i18n("Open in external browser") );

    { //Search text inside lyrics. Code inspired/copied from playlistwindow.cpp
       m_lyricsTextBar = new KToolBar( m_lyricsTab, "NotMainToolBar" );
       m_lyricsTextBar->hide();
       m_lyricsTextBarShowed=false;

       m_lyricsTextBar->setIconSize( 22, false ); //looks more sensible
       m_lyricsTextBar->setFlat( true ); //removes the ugly frame
       m_lyricsTextBar->setMovingEnabled( false ); //removes the ugly frame

       m_lyricsTextBar->boxLayout()->addStretch();

       QWidget *button = new KToolBarButton( "locationbar_erase", 1, m_lyricsTextBar );
       QLabel *filter_label = new QLabel( i18n("S&earch:") + ' ', m_lyricsTextBar );
       m_lyricsSearchText = new ClickLineEdit( i18n( "Search in lyrics" ), m_lyricsTextBar );
       filter_label->setBuddy( m_lyricsSearchText );

       m_lyricsTextBar->setStretchableWidget(m_lyricsSearchText );

       m_lyricsSearchText->setFrame( QFrame::Sunken );
       m_lyricsSearchText->installEventFilter( this ); //we intercept keyEvents

       connect( button, SIGNAL(clicked()), m_lyricsSearchText, SLOT(clear()) );

       QToolTip::add( button, i18n( "Clear search" ) );
       QString filtertip = i18n( "Enter text to search for. Press enter to advance to the next match." );

       QToolTip::add( m_lyricsSearchText, filtertip );

       connect ( button, SIGNAL(clicked()), m_lyricsSearchText, SLOT(clear()) );
       connect ( m_lyricsSearchText, SIGNAL(textChanged(const QString &)), this, SLOT(lyricsSearchText(const QString & )) );
       connect ( m_lyricsSearchText, SIGNAL(returnPressed()), this, (SLOT(lyricsSearchTextNext())) );
       Amarok::actionCollection()->setAutoConnectShortcuts ( true );
       new KAction( i18n("Search text in lyrics"), KShortcut("/"), this,SLOT( lyricsSearchTextShow() ), Amarok::actionCollection(), "search_text_lyric");
       Amarok::actionCollection()->setAutoConnectShortcuts ( false );
    }



    m_lyricsPage = new HTMLView( m_lyricsTab, "lyrics_page", true /* DNDEnabled */, false /* JScriptEnabled*/ );
    m_lyricsTextEdit = new KTextEdit ( m_lyricsTab, "lyrics_text_edit");
    m_lyricsTextEdit->setTextFormat( Qt::PlainText );
    m_lyricsTextEdit->hide();

    m_wikiTab = new QVBox(this, "wiki_tab");

    m_wikiToolBar = new Browser::ToolBar( m_wikiTab );
    m_wikiToolBar->insertButton( "back", WIKI_BACK, false, i18n("Back") );
    m_wikiToolBar->insertButton( "forward", WIKI_FORWARD, false, i18n("Forward") );
    m_wikiToolBar->insertLineSeparator();
    m_wikiToolBar->insertButton( Amarok::icon( "artist" ), WIKI_ARTIST, false, i18n("Artist Page") );
    m_wikiToolBar->insertButton( Amarok::icon( "album" ), WIKI_ALBUM, false, i18n("Album Page") );
    m_wikiToolBar->insertButton( Amarok::icon( "track" ), WIKI_TITLE, false, i18n("Title Page") );
    m_wikiToolBar->insertLineSeparator();
    m_wikiToolBar->insertButton( Amarok::icon( "external" ), WIKI_BROWSER, true, i18n("Open in external browser") );
    m_wikiToolBar->insertButton( Amarok::icon( "change_language" ), WIKI_CONFIG, true, i18n("Change Locale") );

    m_wikiToolBar->setDelayedPopup( WIKI_BACK, m_wikiBackPopup );
    m_wikiToolBar->setDelayedPopup( WIKI_FORWARD, m_wikiForwardPopup );

    m_wikiPage = new HTMLView( m_wikiTab, "wiki_page", true /* DNDEnabled */, false /* JScriptEnabled */ );

    m_cuefile = CueFile::instance();
    connect( m_cuefile, SIGNAL(metaData( const MetaBundle& )),
             EngineController::instance(), SLOT(currentTrackMetaDataChanged( const MetaBundle& )) );
    connect( m_cuefile, SIGNAL(newCuePoint( long, long, long )),
             Scrobbler::instance(), SLOT(subTrack( long, long, long )) );

    addTab( m_contextTab, SmallIconSet( Amarok::icon( "music" ) ),  i18n( "Music" ) );
    addTab( m_lyricsTab,  SmallIconSet( Amarok::icon( "lyrics" ) ), i18n( "Lyrics" ) );
    addTab( m_wikiTab,    SmallIconSet( Amarok::icon( "artist" ) ), i18n( "Artist" ) );

    setTabEnabled( m_lyricsTab, false );
    setTabEnabled( m_wikiTab, false );

    m_showRelated   = Amarok::config( "ContextBrowser" )->readBoolEntry( "ShowRelated", true );
    m_showSuggested = Amarok::config( "ContextBrowser" )->readBoolEntry( "ShowSuggested", true );
    m_showFaves     = Amarok::config( "ContextBrowser" )->readBoolEntry( "ShowFaves", true );
    m_showLabels    = Amarok::config( "ContextBrowser" )->readBoolEntry( "ShowLabels", true );

    m_showFreshPodcasts  = Amarok::config( "ContextBrowser" )->readBoolEntry( "ShowFreshPodcasts", true );
    m_showNewestAlbums   = Amarok::config( "ContextBrowser" )->readBoolEntry( "ShowNewestAlbums", true );
    m_showFavoriteAlbums = Amarok::config( "ContextBrowser" )->readBoolEntry( "ShowFavoriteAlbums", true );

    // Delete folder with the cached coverimage shadow pixmaps
    KIO::del( KURL::fromPathOrURL( Amarok::saveLocation( "covershadow-cache/" ) ), false, false );

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
    connect( m_lyricsToolBar->getButton( LYRICS_EDIT ),    SIGNAL(toggled( int )), SLOT(lyricsEditToggle()) );
    connect( m_lyricsToolBar->getButton( LYRICS_SEARCH ),  SIGNAL(clicked( int )), SLOT(lyricsSearch()) );
    connect( m_lyricsToolBar->getButton( LYRICS_REFRESH ), SIGNAL(clicked( int )), SLOT(lyricsRefresh()) );
    connect( m_lyricsToolBar->getButton( LYRICS_BROWSER ), SIGNAL(clicked( int )), SLOT(lyricsExternalPage()) );

    connect( m_wikiToolBar->getButton( WIKI_BACK    ), SIGNAL(clicked( int )), SLOT(wikiHistoryBack()) );
    connect( m_wikiToolBar->getButton( WIKI_FORWARD ), SIGNAL(clicked( int )), SLOT(wikiHistoryForward()) );
    connect( m_wikiToolBar->getButton( WIKI_ARTIST  ), SIGNAL(clicked( int )), SLOT(wikiArtistPage()) );
    connect( m_wikiToolBar->getButton( WIKI_ALBUM   ), SIGNAL(clicked( int )), SLOT(wikiAlbumPage()) );
    connect( m_wikiToolBar->getButton( WIKI_TITLE   ), SIGNAL(clicked( int )), SLOT(wikiTitlePage()) );
    connect( m_wikiToolBar->getButton( WIKI_BROWSER ), SIGNAL(clicked( int )), SLOT(wikiExternalPage()) );
    connect( m_wikiToolBar->getButton( WIKI_CONFIG  ), SIGNAL(clicked( int )), SLOT(wikiConfig()) );

    connect( m_wikiBackPopup,    SIGNAL(activated( int )), SLOT(wikiBackPopupActivated( int )) );
    connect( m_wikiForwardPopup, SIGNAL(activated( int )), SLOT(wikiForwardPopupActivated( int )) );

    connect( CollectionDB::instance(), SIGNAL( scanStarted() ), SLOT( collectionScanStarted() ) );
    connect( CollectionDB::instance(), SIGNAL( scanDone( bool ) ), SLOT( collectionScanDone( bool ) ) );
    connect( CollectionDB::instance(), SIGNAL( databaseEngineChanged() ), SLOT( renderView() ) );
    connect( CollectionDB::instance(), SIGNAL( coverFetched( const QString&, const QString& ) ),
             this, SLOT( coverFetched( const QString&, const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( coverChanged( const QString&, const QString& ) ),
             this, SLOT( coverRemoved( const QString&, const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( similarArtistsFetched( const QString& ) ),
             this, SLOT( similarArtistsFetched( const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( tagsChanged( const MetaBundle& ) ),
             this, SLOT( tagsChanged( const MetaBundle& ) ) );
    connect( CollectionDB::instance(), SIGNAL( tagsChanged( const QString&, const QString& ) ),
             this, SLOT( tagsChanged( const QString&, const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( ratingChanged( const QString&, int ) ),
             this, SLOT( ratingOrScoreOrLabelsChanged( const QString& ) ) );
    connect( StarManager::instance(), SIGNAL( ratingsColorsChanged() ),
             this, SLOT( ratingOrScoreOrLabelsChanged( const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( scoreChanged( const QString&, float ) ),
             this, SLOT( ratingOrScoreOrLabelsChanged( const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( labelsChanged( const QString& ) ),
             this, SLOT( ratingOrScoreOrLabelsChanged( const QString& ) ) );
    connect( CollectionDB::instance(), SIGNAL( imageFetched( const QString& ) ),
             this, SLOT( imageFetched( const QString& ) ) );

    connect( App::instance(), SIGNAL( useScores( bool ) ),
             this, SLOT( refreshCurrentTrackPage() ) );
    connect( App::instance(), SIGNAL( useRatings( bool ) ),
             this, SLOT( refreshCurrentTrackPage() ) );

    connect( MountPointManager::instance(), SIGNAL( mediumConnected( int ) ),
             this, SLOT( renderView() ) );
    connect( MountPointManager::instance(), SIGNAL( mediumRemoved( int ) ),
             this, SLOT( renderView() ) );

    showContext( KURL( "current://track" ) );

//     setMinimumHeight( AmarokConfig::coverPreviewSize() + (fontMetrics().height()+2)*5 + tabBar()->height() );
}


ContextBrowser::~ContextBrowser()
{
    DEBUG_BLOCK

    ThreadManager::instance()->abortAllJobsNamed( "CurrentTrackJob" );

    // Ensure the KHTMLPart dies before its KHTMLView dies,
    // because KHTMLPart's dtoring relies on its KHTMLView still being alive
    // (see bug 130494).
    delete m_currentTrackPage;
    delete m_lyricsPage;
    delete m_wikiPage;
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
    Amarok::albumArtistTrackFromUrl( url.path(), artist, album, track );

    // All http links should be loaded inside wikipedia tab, as that is the only tab that should contain them.
    // Streams should use stream:// protocol.
    if ( url.protocol() == "http" )
    {
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
        m_wikiCurrentEntry = QString::null;
        showWikipedia( url.url() );
    }
    else if ( url.protocol() == "show" )
    {
        if ( url.path().contains( "suggestLyric-" ) )
        {
            QString _url = url.url().mid( url.url().find( QString( "-" ) ) +1 );
            debug() << "Clicked lyrics URL: " << _url << endl;
            m_dirtyLyricsPage = true;
            showLyrics( _url );
        }
        else if ( url.path() == "collectionSetup" )
        {
            CollectionView::instance()->setupDirs();
        }
        else if ( url.path() == "scriptmanager" )
        {
            ScriptManager::instance()->show();
            ScriptManager::instance()->raise();
        }
        else if ( url.path() == "editLabels" )
        {
            showLabelsDialog();
        }
        // Konqueror sidebar needs these
        if (url.path() == "context") { m_dirtyCurrentTrackPage=true; showContext( KURL( "current://track" ) ); saveHtmlData(); }
        if (url.path() == "wiki") { m_dirtyWikiPage=true; showWikipedia(); saveHtmlData(); }
        if (url.path() == "lyrics") { m_dirtyLyricsPage=true; m_wikiJob=false; showLyrics(); saveHtmlData(); }
    }

    else if ( url.protocol() == "runscript" )
    {
        ScriptManager::instance()->runScript( url.path() );
    }

    // When left-clicking on cover image, open browser with amazon site
    else if ( url.protocol() == "fetchcover" )
    {
        QString albumPath = CollectionDB::instance()->albumImage(artist, album, false, 0 );
        if ( albumPath == CollectionDB::instance()->notAvailCover( false, 0 ) )
        {
            CollectionDB::instance()->fetchCover( this, artist, album, false );
            return;
        }

        QImage img( albumPath );
        const QString amazonUrl = img.text( "amazon-url" );

        if ( amazonUrl.isEmpty() )
            KMessageBox::information( this, i18n( "<p>There is no product information available for this image.<p>Right-click on image for menu." ) );
        else
            Amarok::invokeBrowser( amazonUrl );
    }

    /* open konqueror with musicbrainz search result for artist-album */
    else if ( url.protocol() == "musicbrainz" )
    {
        const QString url = "http://www.musicbrainz.org/taglookup.html?artist=%1&album=%2&track=%3";
        Amarok::invokeBrowser( url.arg( KURL::encode_string_no_slash( artist, 106 /*utf-8*/ ),
        KURL::encode_string_no_slash( album, 106 /*utf-8*/ ),
        KURL::encode_string_no_slash( track, 106 /*utf-8*/ ) ) );
    }

    else if ( url.protocol() == "externalurl" )
        Amarok::invokeBrowser( url.url().replace( QRegExp( "^externalurl:" ), "http:") );

    else if ( url.protocol() == "lastfm" )
    {
        LastFm::WebService *lfm = LastFm::Controller::instance()->getService();
        if      ( url.path() == "skip" ) lfm->skip();
        else if ( url.path() == "love" ) lfm->love();
        else if ( url.path() == "ban"  ) lfm->ban();
    }
    else if ( url.protocol() == "togglebox" )
    {
        if      ( url.path() == "ra" ) m_relatedOpen     ^= true;
        else if ( url.path() == "ss" ) m_suggestionsOpen ^= true;
        else if ( url.path() == "ft" ) m_favoritesOpen  ^= true;
        else if ( url.path() == "sl" ) m_labelsOpen ^= true;
    }

    else if ( url.protocol() == "seek" )
    {
        EngineController::instance()->seek(url.path().toLong());
    }

    // browse albums of a related artist.  Don't do this if we are viewing Home tab
    else if ( url.protocol() == "artist" || url.protocol() == "current" || url.protocol() == "showlabel")
    {
        if( EngineController::engine()->loaded() ) // song must be active
            showContext( url );
    }

    else if( url.protocol() == "artistback" )
    {
        contextHistoryBack();
    }

    else if ( url.protocol() == "wikipedia" )
    {
        m_dirtyWikiPage = true;
        QString entry = unescapeHTMLAttr( url.path() );
        showWikipediaEntry( entry );
    }

    else if( url.protocol() == "ggartist" )
    {
        const QString url2 = QString( "http://www.google.com/musicsearch?q=%1&res=artist" )
            .arg( KURL::encode_string_no_slash( unescapeHTMLAttr( url.path() ).replace( " ", "+" ), 106 /*utf-8*/ ) );
        Amarok::invokeBrowser( url2 );
    }

    else if( url.protocol() == "file" )
    {
        Playlist::instance()->insertMedia( url, Playlist::DefaultOptions );
    }

    else if( url.protocol() == "stream" )
    {
        Playlist::instance()->insertMedia( KURL::fromPathOrURL( url.url().replace( QRegExp( "^stream:" ), "http:" ) ), Playlist::DefaultOptions );
    }

    else if( url.protocol() == "compilationdisc" || url.protocol() == "albumdisc" )
    {
        Playlist::instance()->insertMedia( expandURL( url ) , Playlist::DefaultOptions );
    }

    else
        HTMLView::openURLRequest( url );
}


void ContextBrowser::collectionScanStarted()
{
    m_emptyDB = CollectionDB::instance()->isEmpty();
    if( m_emptyDB && currentPage() == m_contextTab )
        showCurrentTrack();
}


void ContextBrowser::collectionScanDone( bool changed )
{
    if ( CollectionDB::instance()->isEmpty() )
    {
        m_emptyDB = true;
        if ( currentPage() == m_contextTab )
            showCurrentTrack();
    }
    else if ( m_emptyDB )
    {
        m_emptyDB = false;
        PlaylistWindow::self()->showBrowser("CollectionBrowser");
    }
    else if( changed && currentPage() == m_contextTab )
    {
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
    }
}


void ContextBrowser::renderView()
{
    m_dirtyCurrentTrackPage = true;
    m_dirtyLyricsPage = true;
    m_dirtyWikiPage = true;
    m_emptyDB = CollectionDB::instance()->isEmpty();

    showCurrentTrack();
}


void ContextBrowser::lyricsChanged( const QString &url ) {
    if ( url == EngineController::instance()->bundle().url().path() ) {
        m_dirtyLyricsPage = true;
        if ( currentPage() == m_lyricsTab )
            showLyrics();
    }
}

void ContextBrowser::lyricsScriptChanged() {
    m_dirtyLyricsPage = true;
    if ( currentPage() == m_lyricsTab )
        showLyrics();
}

//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::engineNewMetaData( const MetaBundle& bundle, bool trackChanged )
{
    bool newMetaData = false;
    m_dirtyCurrentTrackPage = true;
    m_dirtyLyricsPage = true;
    m_wikiJob = 0; //New metadata, so let's forget previous wiki-fetching jobs

    if ( MetaBundle( m_currentURL ).artist() != bundle.artist() )
        m_dirtyWikiPage = true;
    // Prepend stream metadata history item to list
    if ( !m_metadataHistory.first().contains( bundle.prettyTitle() ) )
    {
        newMetaData = true;
        const QString timeString = KGlobal::locale()->formatTime( QTime::currentTime() ).replace(" ", "&nbsp;"); // don't break over lines
        m_metadataHistory.prepend( QString( "<td valign='top'>" + timeString + "&nbsp;</td><td align='left'>" + escapeHTML( bundle.prettyTitle() ) + "</td>" ) );
    }

    if ( currentPage() == m_contextTab && ( bundle.url() != m_currentURL || newMetaData || !trackChanged ) )
        showCurrentTrack();
    else if ( currentPage() == m_lyricsTab )
    {
        EngineController::engine()->isStream() ?
                lyricsRefresh() : // can't call showLyrics() because the url hasn't changed
                showLyrics()    ;
    }
    else if ( CollectionDB::instance()->isEmpty() || !CollectionDB::instance()->isValid() )
        showCurrentTrack();

    if (trackChanged)
    {
        m_cuefile->clear();

        if (bundle.url().isLocalFile())
        {

            /** The cue file that is provided with the media might have different name than the
             * media file itself, hence simply cutting the media extension and adding ".cue"
             * is not always enough to find the matching cue file. In such cases we have
             * to search for all the cue files in the directory and have a look inside them for
             * the matching FILE="" stanza. However the FILE="" stanza does not always
             * point at the corresponding media file (e.g. it is quite often set to the misleading
             * FILE="audio.wav" WAV). Therfore we also have to check blindly if there is a cue
             * file having the same name as the media file played, as described above.
             */

            // look for the cue file that matches the media file played first
            QString path    = bundle.url().path();
            QString cueFile = path.left( path.findRev('.') ) + ".cue";

            m_cuefile->setCueFileName( cueFile );

            if( m_cuefile->load( bundle.length() ) )
                debug() << "[CUEFILE]: " << cueFile << " - Shoot blindly, found and loaded. " << endl;

            // if unlucky, let's have a look inside cue files, if any
            else
            {
                debug() << "[CUEFILE]: " << cueFile << " - Shoot blindly and missed, searching for other cue files." << endl;

                bool foundCueFile = false;
                QDir dir ( bundle.directory() );
                dir.setFilter( QDir::Files ) ;
                dir.setNameFilter( "*.cue *.CUE" ) ;

                QStringList cueFilesList = dir.entryList();

                if ( !cueFilesList.empty() )
                    for ( QStringList::Iterator it = cueFilesList.begin(); it != cueFilesList.end() && !foundCueFile; ++it )
                    {
                        QFile file ( dir.filePath(*it) );
                        if( file.open( IO_ReadOnly ) )
                        {
                            debug() << "[CUEFILE]: " << *it << " - Opened, looking for the matching FILE stanza." << endl;
                            QTextStream stream( &file );
                            QString line;

                            while ( !stream.atEnd() && !foundCueFile)
                            {
                                line = stream.readLine().simplifyWhiteSpace();

                                if( line.startsWith( "file", false ) )
                                {
                                    line = line.mid( 5 ).remove( '"' );

                                    if ( line.contains( bundle.filename(), false ) )
                                    {
                                        cueFile = dir.filePath(*it);
                                        foundCueFile = true;
                                        m_cuefile->setCueFileName( cueFile );
                                        if( m_cuefile->load( bundle.length() ) )
                                            debug() << "[CUEFILE]: " << cueFile << " - Looked inside cue files, found and loaded proper one" << endl;
                                    }
                                }
                            }

                            file.close();
                        }
                    }

                if ( !foundCueFile )
                    debug() << "[CUEFILE]: - Didn't find any matching cue file." << endl;
            }
        }
    }
}


void ContextBrowser::engineStateChanged( Engine::State state, Engine::State oldState )
{
    DEBUG_BLOCK

    if( state != Engine::Paused /*pause*/ && oldState != Engine::Paused /*resume*/
        || state == Engine::Empty )
    {
        // Pause shouldn't clear everything (but stop should, even when paused)
        m_dirtyCurrentTrackPage = true;
        m_dirtyLyricsPage = true;
        m_wikiJob = 0; //let's forget previous wiki-fetching jobs
    }

    switch( state )
    {
        case Engine::Empty:
            m_metadataHistory.clear();
            if ( currentPage() == m_contextTab || currentPage() == m_lyricsTab )
            {
                showCurrentTrack();
            }
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
    QFile exportedDocument( Amarok::saveLocation() + "contextbrowser.html" );
    if ( !exportedDocument.open( IO_WriteOnly ) )
        warning() << "Failed to open file " << exportedDocument.name()
        << " write-only" << endl;
    else {
        QTextStream stream( &exportedDocument );
        stream.setEncoding( QTextStream::UnicodeUTF8 );
        stream << m_HTMLSource // the pure html data..
            .replace( "<html>",
                      QString( "<html><head><style type=\"text/css\">"
                               "%1</style></head>" )
                          .arg( HTMLView::loadStyleSheet() ) ); // and the
                                                                // stylesheet
                                                                // code
        exportedDocument.close();
    }
}


void ContextBrowser::paletteChange( const QPalette& /* pal */ )
{
//     KTabWidget::paletteChange( pal );
    HTMLView::paletteChange();
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
DEBUG_FUNC_INFO
    setFocusProxy( page ); //so focus is given to a sensible widget when the tab is opened

    if ( page == m_contextTab )
        showCurrentTrack();
    else if ( page == m_lyricsTab )
        showLyrics();
    else if ( page == m_wikiTab )
        showWikipedia();
}

void ContextBrowser::slotContextMenu( const QString& urlString, const QPoint& point )
{
    enum { APPEND, ASNEXT, MAKE, MEDIA_DEVICE, INFO, TITLE, RELATED, SUGGEST, FAVES, FRESHPODCASTS, NEWALBUMS, FAVALBUMS, LABELS };
    debug() << "url string: " << urlString << endl;

    if( urlString.startsWith( "musicbrainz"     ) ||
        urlString.startsWith( "externalurl"     ) ||
        urlString.startsWith( "show:suggest"    ) ||
        urlString.startsWith( "http"            ) ||
        urlString.startsWith( "wikipedia"       ) ||
        urlString.startsWith( "seek"            ) ||
        urlString.startsWith( "ggartist"        ) ||
        urlString.startsWith( "artistback"      ) ||
        urlString.startsWith( "current"         ) ||
        urlString.startsWith( "lastfm"          ) ||
        urlString.startsWith( "showlabel"       ) ||
        urlString.startsWith( "show:editLabels" ) ||
        currentPage() != m_contextTab  )
        return;

    KURL url( urlString );

    KPopupMenu menu;
    KURL::List urls( url );
    QString artist, album, track; // track unused here
    Amarok::albumArtistTrackFromUrl( url.path(), artist, album, track );

    if( urlString.isEmpty() )
    {
        debug() << "url string empty. loaded?" << EngineController::engine()->loaded() << endl;
        if( EngineController::engine()->loaded() )
        {
            menu.setCheckable( true );
            menu.insertItem( i18n("Show Labels"), LABELS );
            menu.insertItem( i18n("Show Related Artists"), RELATED );
            menu.insertItem( i18n("Show Suggested Songs"), SUGGEST );
            menu.insertItem( i18n("Show Favorite Tracks"), FAVES );

            menu.setItemChecked( RELATED, m_showRelated );
            menu.setItemChecked( SUGGEST, m_showSuggested );
            menu.setItemChecked( FAVES,   m_showFaves );
            menu.setItemChecked( LABELS, m_showLabels );
        } else {
            // the home info page
            menu.setCheckable( true );
            menu.insertItem( i18n("Show Fresh Podcasts"), FRESHPODCASTS );
            menu.insertItem( i18n("Show Newest Albums"), NEWALBUMS );
            menu.insertItem( i18n("Show Favorite Albums"), FAVALBUMS );

            menu.setItemChecked( FRESHPODCASTS, m_showFreshPodcasts );
            menu.setItemChecked( NEWALBUMS, m_showNewestAlbums );
            menu.setItemChecked( FAVALBUMS, m_showFavoriteAlbums );
        }
    }
    else if( url.protocol() == "fetchcover" )
    {
        Amarok::coverContextMenu( this, point, artist, album );
        return;
    }
    else if( url.protocol() == "stream" )
    {
        url = KURL::fromPathOrURL( url.url().replace( QRegExp( "^stream:" ), "http:" ) );
        urls = KURL::List( url );
        menu.insertTitle( i18n("Podcast"), TITLE );
        menu.insertItem( SmallIconSet( Amarok::icon( "files" ) ), i18n( "&Load" ), MAKE );
        menu.insertItem( SmallIconSet( Amarok::icon( "add_playlist" ) ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( SmallIconSet( Amarok::icon( "queue_track" ) ), i18n( "&Queue Podcast" ), ASNEXT );
        //menu.insertSeparator();
        //menu.insertItem( SmallIconSet( "down" ), i18n( "&Download" ), DOWNLOAD );
    }
    else if( url.protocol() == "file" || url.protocol() == "artist" || url.protocol() == "album" || url.protocol() == "compilation" || url.protocol() == "albumdisc" || url.protocol() == "compilationdisc")
    {
        //TODO it would be handy and more usable to have this menu under the cover one too

        menu.insertTitle( i18n("Track"), TITLE );
        menu.insertItem( SmallIconSet( Amarok::icon( "files" ) ), i18n( "&Load" ), MAKE );
        menu.insertItem( SmallIconSet( Amarok::icon( "add_playlist" ) ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( SmallIconSet( Amarok::icon( "queue_track" ) ), i18n( "&Queue Track" ), ASNEXT );
        if( MediaBrowser::isAvailable() )
            menu.insertItem( SmallIconSet( Amarok::icon( "device" ) ), i18n( "&Transfer to Media Device" ), MEDIA_DEVICE );

        menu.insertSeparator();
        menu.insertItem( SmallIconSet( Amarok::icon( "info" ) ), i18n( "Edit Track &Information..." ), INFO );

        if ( url.protocol() == "artist" )
        {
            urls = expandURL( url );

            menu.changeTitle( TITLE, i18n("Artist") );
            menu.changeItem( INFO,   i18n("Edit Artist &Information..." ) );
            menu.changeItem( ASNEXT, i18n("&Queue Artist's Songs") );
        }
        if ( url.protocol() == "album" )
        {
            urls = expandURL( url );

            menu.changeTitle( TITLE, i18n("Album") );
            menu.changeItem( INFO,   i18n("Edit Album &Information..." ) );
            menu.changeItem( ASNEXT, i18n("&Queue Album") );
        }
        if ( url.protocol() == "albumdisc" )
        {
            urls = expandURL( url );

            menu.changeTitle( TITLE, i18n("Album Disc") );
            menu.changeItem( INFO,   i18n("Edit Album Disc &Information..." ) );
            menu.changeItem( ASNEXT, i18n("&Queue Album Disc") );
        }
        if ( url.protocol() == "compilation" )
        {
            urls = expandURL( url );

            menu.changeTitle( TITLE, i18n("Compilation") );
            menu.changeItem( INFO,   i18n("Edit Album &Information..." ) );
            menu.changeItem( ASNEXT, i18n("&Queue Album") );
        }
        if ( url.protocol() == "compilationdisc" )
        {
            urls = expandURL( url );

            menu.changeTitle( TITLE, i18n("Compilation Disc") );
            menu.changeItem( INFO,   i18n("Edit Compilation Disc &Information..." ) );
            menu.changeItem( ASNEXT, i18n("&Queue Compilation Disc") );
        }

        if( urls.count() == 0 )
        {
            menu.setItemEnabled( MAKE, false );
            menu.setItemEnabled( APPEND, false );
            menu.setItemEnabled( ASNEXT, false );
            menu.setItemEnabled( MEDIA_DEVICE, false );
            menu.setItemEnabled( INFO, false );
        }
    }

    //Not all these are used in the menu, it depends on the context
    switch( menu.exec( point ) )
    {

    case RELATED:
        m_showRelated = !menu.isItemChecked( RELATED );
        Amarok::config( "ContextBrowser" )->writeEntry( "ShowRelated", m_showRelated );
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
        break;

    case SUGGEST:
        m_showSuggested = !menu.isItemChecked( SUGGEST );
        Amarok::config( "ContextBrowser" )->writeEntry( "ShowSuggested", m_showSuggested );
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
        break;

    case FAVES:
        m_showFaves = !menu.isItemChecked( FAVES );
        Amarok::config( "ContextBrowser" )->writeEntry( "ShowFaves", m_showFaves );
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
        break;

   case LABELS:
        m_showLabels = !menu.isItemChecked( LABELS );
        Amarok::config( "ContextBrowser" )->writeEntry( "ShowLabels", m_showLabels );
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
        break;

   case FRESHPODCASTS:
        m_showFreshPodcasts = !menu.isItemChecked( FRESHPODCASTS );
        Amarok::config( "ContextBrowser" )->writeEntry( "ShowFreshPodcasts", m_showFreshPodcasts );
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
        break;

   case NEWALBUMS:
        m_showNewestAlbums = !menu.isItemChecked( NEWALBUMS );
        Amarok::config( "ContextBrowser" )->writeEntry( "ShowNewestAlbums", m_showNewestAlbums );
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
        break;

   case FAVALBUMS:
        m_showFavoriteAlbums = !menu.isItemChecked( FAVALBUMS );
        Amarok::config( "ContextBrowser" )->writeEntry( "ShowFavoriteAlbums", m_showFavoriteAlbums );
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
            TagDialog* dialog = new TagDialog( urls.first(), instance() );
            dialog->show();
        }
        break;
    }

    case MAKE:
        Playlist::instance()->clear();
        //FALL_THROUGH

    case APPEND:
        Playlist::instance()->insertMedia( urls, Playlist::Append );
        break;

    case MEDIA_DEVICE:
        MediaBrowser::queue()->addURLs( urls );
        break;

    }
}



//////////////////////////////////////////////////////////////////////////////////////////
// Current-Tab
//////////////////////////////////////////////////////////////////////////////////////////

/** This is the slowest part of track change, so we thread it */
class CurrentTrackJob : public ThreadManager::DependentJob
{
public:
    CurrentTrackJob( ContextBrowser *parent )
        : ThreadManager::DependentJob( parent, "CurrentTrackJob" )
        , b( parent )
        , m_currentTrack( QDeepCopy<MetaBundle>( EngineController::instance()->bundle() ) )
        , m_isStream( EngineController::engine()->isStream() )
    {
        for( QStringList::iterator it = b->m_metadataHistory.begin();
                it != b->m_metadataHistory.end();
                ++it )
        {
            m_metadataHistory += QDeepCopy<QString>( *it );
        }


        m_amarokIconPath = QDeepCopy<QString>(KGlobal::iconLoader()->iconPath( "amarok",
                    -KIcon::SizeEnormous ) );
        m_musicBrainIconPath = QDeepCopy<QString>(locate( "data", "amarok/images/musicbrainz.png" )
                );
        m_lastfmIcon = "file://" + locate( "data","amarok/images/lastfm.png" );
    }

private:
    virtual bool doJob();
    void addMetaHistory();
    void showLastFm( const MetaBundle &currentTrack );
    void showStream( const MetaBundle &currentTrack );
    void showPodcast( const MetaBundle &currentTrack );
    void showBrowseArtistHeader( const QString &artist );
    void showBrowseLabelHeader( const QString &label );
    void showCurrentArtistHeader( const MetaBundle &currentTrack );
    void showRelatedArtists( const QString &artist, const QStringList &relArtists );
    void showSuggestedSongs( const QStringList &relArtists );
    void showSongsWithLabel( const QString &label );
    void showArtistsFaves( const QString &artistName, uint artist_id );
    void showArtistsAlbums( const QString &artist, uint artist_id, uint album_id );
    void showArtistsCompilations( const QString &artist, uint artist_id, uint album_id );
    void showHome();
    void showUserLabels( const MetaBundle &currentTrack );
    QString fetchLastfmImage( const QString& url );
    QStringList showHomeByAlbums();
    void constructHTMLAlbums( const QStringList &albums, QString &htmlCode, const QString &idPrefix );
    static QString statsHTML( int score, int rating, bool statsbox = true ); // meh.

    virtual void completeJob()
    {
        // are we still showing the currentTrack page?
//         if( b->currentPage() != b->m_contextTab )
//             return;

        b->m_shownAlbums.clear();
        for( QStringList::iterator it = m_shownAlbums.begin();
                it != m_shownAlbums.end();
                ++it )
            b->m_shownAlbums.append( QDeepCopy<QString>( *it ) );
        b->m_HTMLSource = QDeepCopy<QString>( m_HTMLSource );
        b->m_currentTrackPage->set( m_HTMLSource );
        b->m_dirtyCurrentTrackPage = false;
        b->saveHtmlData(); // Send html code to file
    }
    QString m_HTMLSource;
    QString m_amarokIconPath;
    QString m_musicBrainIconPath;
    QString m_lastfmIcon;

    ContextBrowser *b;
    MetaBundle m_currentTrack;
    bool m_isStream;
    QStringList m_shownAlbums;
    QStringList m_metadataHistory;
};

void
ContextBrowser::showContext( const KURL &url, bool fromHistory )
{
    if ( currentPage() != m_contextTab )
    {
        blockSignals( true );
        showPage( m_contextTab );
        blockSignals( false );
    }

    m_dirtyCurrentTrackPage = true;
    m_contextURL = url.url();

    if( url.protocol() == "current" )
    {
        m_browseArtists = false;
        m_browseLabels = false;
        m_label = QString::null;
        m_artist = QString::null;
        m_contextBackHistory.clear();
        m_contextBackHistory.push_back( "current://track" );
    }
    else if( url.protocol() == "artist" )
    {
        m_browseArtists = true;
        m_browseLabels = false;
        m_label = QString::null;
        m_artist = unescapeHTMLAttr( url.path() );
    }
    else if( url.protocol() == "showlabel" )
    {
        m_browseLabels = true;
        m_browseArtists = false;
        m_artist = QString::null;
        m_label = unescapeHTMLAttr( url.path() );
    }

    // Append new URL to history
    if ( !fromHistory ) {
        m_contextBackHistory += m_contextURL.url();
    }
    // Limit number of items in history
    if ( m_contextBackHistory.count() > CONTEXT_MAX_HISTORY )
        m_contextBackHistory.pop_front();

    showCurrentTrack();
}

void
ContextBrowser::contextHistoryBack() //SLOT
{
    if( m_contextBackHistory.size() > 0 )
    {
        m_contextBackHistory.pop_back();

        m_dirtyCurrentTrackPage = true;

        showContext( KURL( m_contextBackHistory.last() ), true );
    }
}


void ContextBrowser::showCurrentTrack() //SLOT
{
#if 0
    if( BrowserBar::instance()->currentBrowser() != this )
    {
        debug() << "current browser is not context, aborting showCurrentTrack()" << endl;
        m_dirtyCurrentTrackPage = true;
        m_currentTrackPage->set( QString( "<html><body><div class='box-body'>%1</div></body></html>" )
                                 .arg( i18n( "Updating..." ) ) );
        return;
    }
#endif
    if ( currentPage() != m_contextTab ) {
        blockSignals( true );
        showPage( m_contextTab );
        blockSignals( false );
    }

    // TODO: Show CurrentTrack or Lyric tab if they were selected
    // If it's not a streaming, check for a collection
    if ( !EngineController::engine()->isStream() )
    {
        if ( m_emptyDB && CollectionDB::instance()->isValid() && !MountPointManager::instance()->collectionFolders().isEmpty() )
        {
            showScanning();
            return;
        }
        else if ( CollectionDB::instance()->isEmpty() || !CollectionDB::instance()->isValid() )
        {
            showIntroduction();
            return;
        }
    }
    if( !m_dirtyCurrentTrackPage )
        return;
    m_currentURL = EngineController::instance()->bundle().url();
    m_currentTrackPage->write( QString::null );
    ThreadManager::instance()->onlyOneJob( new CurrentTrackJob( this ) );
}



//////////////////////////////////////////////////////////////////////////////////////////
// Shows the statistics summary when no track is playing
//////////////////////////////////////////////////////////////////////////////////////////

void CurrentTrackJob::showHome()
{
    QueryBuilder qb;

    qb.clear(); //Song count
    qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    QStringList a = qb.run();
    QString songCount = a[0];

    qb.clear(); //Artist count
    //qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabArtist, QueryBuilder::valID );
    //qb.setOptions( QueryBuilder::optRemoveDuplicates );
    //a = qb.run();
    //QString artistCount = a[0];
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valArtistID );
    //I can't get the correct value w/o suing a subquery, and querybuilder doesn't support those
    QString artistCount = QString::number( qb.run().count() );

    qb.clear(); //Album count
    //qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabAlbum, QueryBuilder::valID );
    //qb.setOptions( QueryBuilder::optRemoveDuplicates );
    //a = qb.run();
    //QString albumCount = a[0];
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valAlbumID );
    QString albumCount = QString::number( qb.run().count() );

    qb.clear(); //Genre count
    //qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabGenre, QueryBuilder::valID );
    //qb.setOptions( QueryBuilder::optRemoveDuplicates );
    //a = qb.run();
    //QString genreCount = a[0];
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valGenreID );
    QString genreCount = QString::number( qb.run().count() );

    qb.clear(); //Total Playtime
    qb.addReturnFunctionValue( QueryBuilder::funcSum, QueryBuilder::tabSong, QueryBuilder::valLength );
    a = qb.run();
    QString playTime = MetaBundle::fuzzyTime( a[0].toInt() );

    m_HTMLSource.append(
            QStringx(
                "<div id='introduction_box' class='box'>\n"
                "<div id='introduction_box-header-title' class='box-header'>\n"
                "<span id='introduction_box-header-title' class='box-header-title'>\n"
                + i18n( "No Track Playing" ) +
                "</span>\n"
                "</div>\n"
                "<table id='current_box-table' class='box-body' width='100%' cellpadding='0' cellspacing='0'>\n"
                "<tr>\n"
                "<td id='current_box-largecover-td'>\n"
                "<a href='%1'><img id='current_box-largecover-image' src='%2' title='Amarok'></a>\n"
                "</td>\n"
                "<td id='current_box-information-td' align='right'>\n"
                "<span>%3</span><br />\n"
                "<span>%4</span><br />\n"
                "<span>%5</span><br />\n"
                "<span>%6</span><br />\n"
                "<span>%7</span><br />\n"
                "</td>\n"
                "</tr>\n"
                "</table>\n"
                "</div>\n" )
            .args( QStringList()
                    << escapeHTMLAttr( "externalurl://amarok.kde.org" )
                    << escapeHTMLAttr( m_amarokIconPath )
                    << i18n( "1 Track",  "%n Tracks",  songCount.toInt() )
                    << i18n( "1 Artist", "%n Artists", artistCount.toInt() )
                    << i18n( "1 Album",  "%n Albums",  albumCount.toInt() )
                    << i18n( "1 Genre",  "%n Genres",  genreCount.toInt() )
                    << i18n( "%1 Play-time" ).arg ( playTime ) ) );

    m_shownAlbums = showHomeByAlbums();

    m_HTMLSource.append(
            "</div></body></html>\n");
}


void
CurrentTrackJob::constructHTMLAlbums( const QStringList &reqResult, QString &htmlCode, const QString &stID )
{
    // This function create the html code used to display a list of albums. Each album
    // is a 'toggleable' block.
    // Parameter stID is used to differentiate same albums in different album list. So if this function
    // is called multiple time in the same HTML code, stID must be different.
    for ( uint i = 0; i < reqResult.count(); i += 4 )
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
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, reqResult[i+1] );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, reqResult[i+3] );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTitle );
        qb.setOptions( QueryBuilder::optNoCompilations ); // samplers __need__ to be handled differently
        QStringList albumValues = qb.run();

        QString albumYear;
        if ( !albumValues.isEmpty() )
        {
            albumYear = albumValues[ 3 ];
            for ( uint j = 0; j < albumValues.count(); j += qb.countReturnValues())
                if ( albumValues[j + 3] != albumYear || albumYear == "0" )
                {
                    albumYear = QString::null;
                    break;
                }
        }

        uint i_albumLength = 0;
        for ( uint j = 0; j < albumValues.count(); j += qb.countReturnValues() )
            i_albumLength += QString(albumValues[j + 4]).toInt();

        QString albumLength = ( i_albumLength==0 ? i18n( "Unknown" ) : MetaBundle::prettyTime( i_albumLength, true ) );

        htmlCode.append( QStringx (
                    "<tr class='" + QString( (i % 4) ? "box-row-alt" : "box-row" ) + "'>\n"
                    "<td>\n"
                    "<div class='album-header' onClick=\"toggleBlock('IDA%1')\">\n"
                    "<table width='100%' border='0' cellspacing='0' cellpadding='0'>\n"
                    "<tr>\n")
                .args( QStringList()
                    << stID + reqResult[i+1] ));

        QString albumName = escapeHTML( reqResult[ i ].isEmpty() ? i18n( "Unknown album" ) : reqResult[ i ] );

        QString artistName = albumValues[5].isEmpty() ? i18n( "Unknown artist" ) : albumValues[5];

        QString albumImage = ContextBrowser::getEncodedImage( CollectionDB::instance()->albumImage( albumValues[5], reqResult[ i ], true, 50 ) );
        QString albumImageTitleAttr = albumImageTooltip( albumImage, 50 );

        // Album image
        htmlCode.append( QStringx (
                    "<td width='1'>\n"
                    "<a href='fetchcover:%1 @@@ %2'>\n"
                    "<img class='album-image' align='left' vspace='2' hspace='2' title='%3' src='%4'/>\n"
                    "</a>\n"
                    "</td>\n"
                    "<td valign='middle' align='left'>\n"
                    "<a href='artist:%5'>\n"
                    "<span class='album-title'>%6</span>\n"
                    "</a>\n"
                    "<span class='song-separator'> - </span>\n"
                    "<a href='album:%7 @@@ %8'>\n"
                    "<span class='album-title'>%9</span>\n"
                    "</a>\n" )
                .args( QStringList()
                    << escapeHTMLAttr( albumValues[5] ) // artist name
                    << escapeHTMLAttr( reqResult[ i ].isEmpty() ? i18n( "Unknown" ) : reqResult[ i ] ) // album.name
                    << albumImageTitleAttr
                    << escapeHTMLAttr( albumImage )
                    << escapeHTMLAttr( artistName )
                    << escapeHTML( artistName )
                    << albumValues[6]
                    << reqResult[ i + 1 ] //album.id
                    << albumName ) );

        // Tracks number, year and length
        htmlCode.append( QStringx (
                    "<span class='album-info'>%1</span> "
                    "<br />\n"
                    "<span class='album-year'>%2</span>\n"
                    "<span class='album-length'>%3</span>\n"
                    "</td>\n")
                .args( QStringList()
                    << i18n( "Single", "%n Tracks",  albumValues.count() / qb.countReturnValues() )
                    << albumYear
                    << albumLength) );

        // Begining of the 'toggleable div' that contains the songs
        htmlCode.append( QStringx (
                    "</tr>\n"
                    "</table>\n"
                    "</div>\n"
                    "<div class='album-body' style='display:%1;' id='IDA%2'>\n" )
                .args( QStringList()
                    << "none" /* shows it if it's the current track album */
                    << stID + reqResult[ i + 1 ] ) );

        QString discNumber;

        if ( !albumValues.isEmpty() )
        {
            for ( uint j = 0; j < albumValues.count(); j += qb.countReturnValues() )
            {
                QString newDiscNumber = albumValues[ j + 7 ].stripWhiteSpace();
                if( discNumber != newDiscNumber && newDiscNumber.toInt() > 0)
                {
                    discNumber = newDiscNumber;
                    htmlCode.append( QStringx (
                                "<div class='disc-separator'>\n"
                                "<a href=\"albumdisc: %1 @@@ %2 @@@ %3\">\n"
                                "%4"
                                "</a>\n"
                                "</div>\n" )
                            .args( QStringList()
                                << albumValues[6]
                                << reqResult[ i + 1 ] //album.id
                                << escapeHTMLAttr( discNumber )
                                << i18n( "Disc %1" ).arg( discNumber ) ) );
                }
                QString track = albumValues[j + 2].stripWhiteSpace();
                if( track.length() > 0 )
                {
                    if( track.length() == 1 )
                        track.prepend( "0" );

                    track = "<span class='album-song-trackno'>\n" + track + "&nbsp;</span>\n";
                }

                QString length;
                if( albumValues[j + 4] != "0" )
                    length = "<span class='album-song-time'>(" + MetaBundle::prettyTime( QString(albumValues[j + 4]).toInt(), true ) + ")</span>\n";

                htmlCode.append(
                        "<div class='album-song'>\n"
                        "<a href=\"file:" + escapeHTMLAttr( albumValues[j + 1] ) + "\">\n"
                        + track +
                        "<span class='album-song-title'>\n" + escapeHTML( albumValues[j] ) + "</span>&nbsp;"
                        + length +
                        "</a>\n"
                        "</div>\n" );
            }
        }

        htmlCode.append(
                "</div>\n"
                "</td>\n"
                "</tr>\n" );
    }
}


// return list of albums shown
QStringList
CurrentTrackJob::showHomeByAlbums()
{
    QueryBuilder qb;

    m_HTMLSource.append( "<table width='100%' cellpadding='0' cellspacing='0' border='0'><tr>\n" );

    // <Fresh Podcasts Information>
    if( ContextBrowser::instance()->m_showFreshPodcasts )
    {
        qb.clear();
        qb.addReturnValue( QueryBuilder::tabPodcastEpisodes, QueryBuilder::valParent );
        qb.addFilter( QueryBuilder::tabPodcastEpisodes, QueryBuilder::valIsNew, CollectionDB::instance()->boolT(), QueryBuilder::modeNormal, true );
        qb.sortBy( QueryBuilder::tabPodcastEpisodes, QueryBuilder::valID, true );
        qb.setOptions( QueryBuilder::optRemoveDuplicates );
        qb.setLimit( 0, 5 );
        QStringList channels = qb.run();

        if( channels.count() > 0 )
        {
            m_HTMLSource.append(
                    "<td valign='top'><div id='least_box' class='box'>\n"
                    "<div id='least_box-header' class='box-header'>\n"
                    "<span id='least_box-header-title' class='box-header-title'>\n"
                    + i18n( "Fresh Podcast Episodes" ) +
                    "</span>\n"
                    "</div>\n"
                    "<table id='least_box-body' class='box-body' width='100%' cellpadding='0' cellspacing='0'>\n" );

            uint i = 0;
            for( QStringList::iterator it = channels.begin();
                    it != channels.end();
                    it++ )
            {
                PodcastChannelBundle pcb;
                if( !CollectionDB::instance()->getPodcastChannelBundle( *it, &pcb ) )
                    continue;

                QValueList<PodcastEpisodeBundle> episodes = CollectionDB::instance()->getPodcastEpisodes( *it, true /* only new */, 1 );
                if( !episodes.isEmpty() )
                {
                    PodcastEpisodeBundle &ep = *episodes.begin();

                    QString date;
                    ep.dateTime().isNull() ?
                            date = ep.date() :
                            date = ep.dateTime().toString();

                    QString image = CollectionDB::instance()->podcastImage( pcb.imageURL().url(), true, 50 );
                    QString imageAttr = escapeHTMLAttr( i18n( "Click to go to podcast website: %1." ).arg( pcb.link().prettyURL() ) );

                    m_HTMLSource.append( QStringx (
                                "<tr class='" + QString( (i % 2) ? "box-row-alt" : "box-row" ) + "'>\n"
                                "<td>\n"
                                "<div class='album-header' onClick=\"toggleBlock('IDP%1')\">\n"
                                "<table width='100%' border='0' cellspacing='0' cellpadding='0'>\n"
                                "<tr>\n"
                                "<td width='1'>\n"
                                "<a href='%2'>\n"
                                "<img class='album-image' align='left' vspace='2' hspace='2' title='%3' src='%4' />\n"
                                "</a>\n"
                                "</td>\n"
                                "<td valign='middle' align='left'>\n"
                                "<span class='album-info'>%5</span> \n"
                                "<a href='%6'><span class='album-title'>%7</span></a>\n"
                                "<br />\n"
                                "<span class='album-year'>%8</span>\n"
                                "</td>\n"
                                "</tr>\n"
                                "</table>\n"
                                "</div>\n"
                                "<div class='album-body' style='display:%9;' id='IDP%10'>\n" )
                                .args( QStringList()
                                        << QString::number( i )
                                        << pcb.link().url().replace( QRegExp( "^http:" ), "externalurl:" )
                                        << escapeHTMLAttr( imageAttr )
                                        << escapeHTMLAttr( image )
                                        << escapeHTML( ep.duration() ? MetaBundle::prettyTime( ep.duration() ) : QString( "" ) )
                                        << ( ep.localUrl().isValid()
                                            ? ep.localUrl().url()
                                            : ep.url().url().replace( QRegExp( "^http:" ), "stream:" ) )
                                        << escapeHTML( pcb.title() + ": " + ep.title() )
                                        << escapeHTML( date )
                                        << "none"
                                        << QString::number( i )
                                     )
                                );

                    m_HTMLSource.append( QStringx ( "<p>%1</p>\n" ).arg( ep.description() ) );

                    m_HTMLSource.append(
                            "</div>\n"
                            "</td>\n"
                            "</tr>\n" );
                    i++;
                }
            }
            m_HTMLSource.append(
                    "</table>\n"
                    "</div>\n"
                    "</td>\n"
                    "</tr>\n"
                    "<tr>\n" );
        }
    }
    // </Fresh Podcasts Information>

    QStringList albums;
    // <Newest Albums Information>
    if( ContextBrowser::instance()->m_showNewestAlbums )
    {
        qb.clear();
        qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valID );
        qb.addReturnFunctionValue( QueryBuilder::funcMax, QueryBuilder::tabSong, QueryBuilder::valCreateDate );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valID );
        qb.sortByFunction( QueryBuilder::funcMax, QueryBuilder::tabSong, QueryBuilder::valCreateDate, true );
        qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
        qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valID );
        qb.groupBy( QueryBuilder::tabArtist, QueryBuilder::valID );
        qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.setOptions( QueryBuilder::optNoCompilations ); // samplers __need__ to be handled differently
        qb.setLimit( 0, 5 );
        QStringList recentAlbums = qb.run();

        foreach( recentAlbums )
        {
            albums += *it;
            it++;
            it++;
            it++;
        }
        // toggle html here so we get correct albums
        m_HTMLSource.append(
                "<td valign='top'>\n"
                "<div id='least_box' class='box'>\n"
                "<div id='least_box-header' class='box-header'>\n"
                "<span id='least_box-header-title' class='box-header-title'>\n"
                + i18n( "Your Newest Albums" ) +
                "</span>\n"
                "</div>\n"
                "<table id='least_box-body' class='box-body' width='100%' cellpadding='0' cellspacing='0'>\n" );
        constructHTMLAlbums( recentAlbums, m_HTMLSource, "1" );

        m_HTMLSource.append(
                "</table>\n"
                "</div>\n"
                "</td>\n"
                "</tr>\n"
                "<tr>\n" );
    }
    // </Recent Tracks Information>

    // <Favorite Albums Information>
    if( ContextBrowser::instance()->m_showFavoriteAlbums )
    {
        qb.clear();
        qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valID );
        qb.sortByFavoriteAvg(); // this function adds return values!
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valID );
        // only albums with more than 3 tracks
        qb.having( QueryBuilder::tabAlbum, QueryBuilder::valID, QueryBuilder::funcCount, QueryBuilder::modeGreater, "3" );
        qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
        qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valID );
        qb.groupBy( QueryBuilder::tabArtist, QueryBuilder::valID );
        qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.setOptions( QueryBuilder::optNoCompilations ); // samplers __need__ to be handled differently
        qb.setLimit( 0, 5 );
        QStringList faveAlbums = qb.run();
        QStringList faveResults;

        bool ratings = AmarokConfig::useRatings();
        bool scores = AmarokConfig::useScores();

        foreach( faveAlbums ) {
            albums += *it;
            faveResults += *(it++);
            faveResults += *(it++);
            faveResults += *(it++);
            // sortByFavoriteAvg add some return values, and constructHTMLAlbums expects
            // a specific set of return values, so we might need to skip some values
            if ( ratings )
                it++;
            if ( scores )
                it++;
            faveResults += *(it);
        }

        m_HTMLSource.append(
                "<td valign='top'>\n"
                "<div id='albums_box' class='box'>\n"
                "<div id='albums_box-header' class='box-header'>\n"
                "<span id='albums_box-header-title' class='box-header-title'>\n"
                + i18n( "Favorite Albums" ) +
                "</span>\n"
                "</div>\n"
                "<table id='albums_box-body' class='box-body' width='100%' border='0' cellspacing='0' cellpadding='0'>\n" );

        if ( faveAlbums.count() == 0 )
        {
            m_HTMLSource.append(
                    "<div id='favorites_box-body' class='box-body'><p>\n" +
                    (QueryBuilder::valForFavoriteSorting() == QueryBuilder::valRating
                        ? i18n( "A list of your favorite albums will appear here, once you have rated a few of your songs." )
                        : i18n( "A list of your favorite albums will appear here, once you have played a few of your songs." ) ) +
                    "</p></div>\n" );
        }
        else
        {
            constructHTMLAlbums( faveResults, m_HTMLSource, "2" );
        }

        m_HTMLSource.append(
                "</table></div></td>\n" );
    }
    // </Favorite Tracks Information>

    m_HTMLSource.append( "</tr></table>\n" );

    return albums;
}


void CurrentTrackJob::showLastFm( const MetaBundle &currentTrack )
{
    if( !LastFm::Controller::instance()->isPlaying() ) return;

    const LastFm::Bundle *lastFmInfo = currentTrack.lastFmBundle();
    if ( !lastFmInfo ) return;

    const QString username   = AmarokConfig::scrobblerUsername();
    const QString userpage   = "www.last.fm/user/" + username; //no http
    const QString albumUrl   = lastFmInfo->albumUrl();
    const QString artistUrl  = lastFmInfo->artistUrl();
    const QString titleUrl   = lastFmInfo->titleUrl();

    const QString coverImage = ContextBrowser::getEncodedImage( lastFmInfo->imageUrl() );

    QPtrList<QString> newUrls;
    newUrls.append( &albumUrl  );
    newUrls.append( &artistUrl );
    newUrls.append( &titleUrl  );

    for ( QString* url = newUrls.first(); url; url = newUrls.next() )
        url->replace( QRegExp( "^http:" ), "externalurl:" );

    const QString skipIcon = KGlobal::iconLoader()->iconPath( Amarok::icon("next"),   -KIcon::SizeSmallMedium );
    const QString loveIcon = KGlobal::iconLoader()->iconPath( Amarok::icon("love"),   -KIcon::SizeSmallMedium );
    const QString banIcon  = KGlobal::iconLoader()->iconPath( Amarok::icon("remove"), -KIcon::SizeSmallMedium );


    m_HTMLSource.append( QStringx(
            "<div id='current_box' class='box'>\n"
            "<div id='current_box-header' class='box-header'>\n"
            "<span id='current_box-header-stream' class='box-header-title'>%1</span> "
            "</div>\n"
            "<table id='current_box-body' class='box-body' width='100%' border='0' cellspacing='0' cellpadding='1'>\n"
            "<tr class='box-row'>\n"
            "<td id='current_box-information-td' colspan=2>\n"
            "<a href='%2'><b>%3</b></a> - <a href='%4'><b>%5</b></a>"
            "<br />\n"
            "<a href='%6'><b>%7</b></a>"
            "</td>\n"
            "</tr>\n"
            "<tr class='box-row'>\n"
            "<td id='current_box-largecover-td'>\n"
            "<a href='%8'>"
            "<img id='current_box-largecover-image' src='%9' title='%10'>\n"
            "</a></td>\n"
            "<td id='current_box-information-td' align='right'>\n"
            "<div id='musicbrainz-div'>\n"
            "<a id='lastfm-a' href='externalurl://%11'>\n"
            "<img id='lastfm-image' title='%12' src='%13' />\n"
            "</a>\n"
            "</div>\n"
            "<table cellpadding='1'>\n"
            "<tr><td>\n"
            "<a href='lastfm:skip'>%14</a>\n"
            "</td><td>\n"
            "<a href='lastfm:skip'><img id='lastfm-skip-image' src='%15'></a>\n"
            "</td></tr>\n"
            "<tr><td>\n"
            "<a href='lastfm:love'>%16</a>\n"
            "</td><td>\n"
            "<a href='lastfm:love'><img id='lastfm-love-image' src='%17'></a>\n"
            "</td></tr>\n"
            "<tr><td>\n"
            "<a href='lastfm:ban'>%18</a>\n"
            "</td><td>\n"
            "<a href='lastfm:ban'><img id='lastfm-ban-image' src='%19'></a>\n"
            "</td></tr>\n"
            "</table>\n"
            "</td>\n"
            "</tr>\n"
            "</table>\n"
            "</div>\n" )
            .args( QStringList()
            << escapeHTML( LastFm::Controller::stationDescription() )  //1
            << artistUrl  //2
            << escapeHTML( currentTrack.artist() ) //3
            << titleUrl //4
            << escapeHTML( currentTrack.title() ) //5
            << albumUrl //6
            << escapeHTML( currentTrack.album() ) //7
            << albumUrl //8
            << coverImage //9
            << escapeHTMLAttr( currentTrack.album() )//10
            << escapeHTMLAttr( userpage ) //11
            << escapeHTMLAttr( userpage ) //12
            << escapeHTMLAttr( m_lastfmIcon ) //13
            << escapeHTML( i18n( "Skip" ) ) //14
            << escapeHTMLAttr( skipIcon ) //15
            << escapeHTML( i18n( "Love" ) ) //16
            << escapeHTMLAttr( loveIcon ) //17
            << escapeHTML( i18n( "Ban" ) ) //18
            << escapeHTMLAttr( banIcon ) //19
                ) );

    addMetaHistory();

    if( ContextBrowser::instance()->m_showRelated || ContextBrowser::instance()->m_showSuggested )
    {
        QStringList relArtists = CollectionDB::instance()->similarArtists( currentTrack.artist(), 10 );
        if ( !relArtists.isEmpty() )
        {
            if( ContextBrowser::instance()->m_showRelated )
                showRelatedArtists( currentTrack.artist(), relArtists );

            if( ContextBrowser::instance()->m_showSuggested )
                showSuggestedSongs( relArtists );
        }
    }

    const uint artist_id = CollectionDB::instance()->artistID( currentTrack.artist(), false /* don't autocreate */ );
    if( artist_id )
    {
        if( ContextBrowser::instance()->m_showFaves )
            showArtistsFaves( currentTrack.artist(), artist_id );

        const uint album_id  = CollectionDB::instance()->albumID ( currentTrack.album(), false /* don't autocreate */ );
        showArtistsAlbums( currentTrack.artist(), artist_id, album_id );
        showArtistsCompilations( currentTrack.artist(), artist_id, album_id );
    }

    m_HTMLSource.append( "</body></html>\n" );
}


void CurrentTrackJob::showStream( const MetaBundle &currentTrack )
{
    m_HTMLSource.append( QStringx(
                "<div id='current_box' class='box'>\n"
                "<div id='current_box-header' class='box-header'>\n"
                "<span id='current_box-header-stream' class='box-header-title'>%1</span> "
                "</div>\n"
                "<table id='current_box-body' class='box-body' width='100%' border='0' cellspacing='0' cellpadding='1'>\n"
                "<tr class='box-row'>\n"
                "<td height='42' valign='top' width='90%'>\n"
                "<b>%2</b>\n"
                "<br />\n"
                "<br />\n"
                "%3"
                "<br />\n"
                "<br />\n"
                "%4"
                "<br />\n"
                "%5 kbps"
                "<br />\n"
                "%6"
                "<br />\n"
                "%7"
                "</td>\n"
                "</tr>\n"
                "</table>\n"
                "</div>\n" )
                .args( QStringList()
                        << i18n( "Stream Details" )
                        << escapeHTML( currentTrack.prettyTitle() )
                        << escapeHTML( currentTrack.streamName() )
                        << escapeHTML( currentTrack.genre() )
                        << escapeHTML( currentTrack.prettyBitrate() )
                        << escapeHTML( currentTrack.streamUrl() )
                        << escapeHTML( currentTrack.prettyURL() ) ) );

    addMetaHistory();

    m_HTMLSource.append( "</body></html>\n" );
}

void CurrentTrackJob::addMetaHistory()
{
    if ( m_metadataHistory.count() > 0 )
    {
        m_HTMLSource.append(
                "<div id='stream-history_box' class='box'>\n"
                "<div id='stream-history_box-header' class='box-header'>\n" + i18n( "Metadata History" ) + "</div>\n"
                "<table id='stream-history_box-body' class='box-body' width='100%' border='0' cellspacing='0' cellpadding='1'>\n" );

        for ( uint i = 0; i < m_metadataHistory.count(); ++i )
        {
            const QString &str = m_metadataHistory[i];
            m_HTMLSource.append( QStringx( "<tr class='box-row'><td>%1</td></tr>\n" ).arg( str ) );
        }

        m_HTMLSource.append(
                "</table>\n"
                "</div>\n" );
    }
}

void CurrentTrackJob::showPodcast( const MetaBundle &currentTrack )
{
    if( !currentTrack.podcastBundle() )
        return;

    PodcastEpisodeBundle peb = *currentTrack.podcastBundle();
    PodcastChannelBundle pcb;
    bool channelInDB = true;
    if( !CollectionDB::instance()->getPodcastChannelBundle( peb.parent(), &pcb ) )
    {
        pcb.setTitle( i18n( "Unknown Channel (not in Database)" ) );
        channelInDB = false;
    }

    QString image;
    if( pcb.imageURL().isValid() )
       image = CollectionDB::instance()->podcastImage( pcb.imageURL().url(), true );
    else
       image = CollectionDB::instance()->notAvailCover( true );

    QString imageAttr = escapeHTMLAttr( pcb.link().isValid()
            ? i18n( "Click to go to podcast website: %1." ).arg( pcb.link().prettyURL() )
            : i18n( "No podcast website." )
            );

    m_HTMLSource.append( QStringx(
                "<div id='current_box' class='box'>\n"
                "<div id='current_box-header' class='box-header'>\n"
                "<span id='current_box-header-artist' class='box-header-title'>%1</span> "
                "<br />\n"
                "<span id='current_box-header-album' class='box-header-title'>%2</span>\n"
                "</div>\n"

                "<table id='current_box-table' class='box-body' width='100%' cellpadding='0' cellspacing='0'>\n"
                "<tr>\n"
                "<td id='current_box-largecover-td'>\n"
                "<a id='current_box-largecover-a' href='%3'>\n"
                "<img id='current_box-largecover-image' src='%4' title='%5'>\n"
                "</a>\n"
                "</td>\n"
                "<td id='current_box-information-td' align='right'>\n"
                "%6"
                "%7"
                "</td>\n"
                "</table>\n"
                "</div>\n" )
            .args( QStringList()
                << escapeHTML( pcb.title() )
                << escapeHTML( peb.title() )
                << ( pcb.link().isValid()
                    ? pcb.link().url().replace( QRegExp( "^http:" ), "externalurl:" )
                    : "current://track" )
                << image
                << imageAttr
                << escapeHTML( peb.author().isEmpty()
                    ? i18n( "Podcast" )
                    : i18n( "Podcast by %1" ).arg( peb.author() ) )
                << ( peb.localUrl().isValid()
                    ? "<br />\n" + escapeHTML( i18n( "(Cached)" ) )
                    : "" )
                )
            );

    if ( m_isStream && m_metadataHistory.count() > 1 )
    {
        m_HTMLSource.append(
                "<div id='stream-history_box' class='box'>\n"
                "<div id='stream-history_box-header' class='box-header'>\n" + i18n( "Metadata History" ) + "</div>\n"
                "<table id='stream-history_box-body' class='box-body' width='100%' border='0' cellspacing='0' cellpadding='1'>\n" );

        for ( uint i = 0; i < m_metadataHistory.count(); ++i )
        {
            const QString &str = m_metadataHistory[i];
            m_HTMLSource.append( QStringx( "<tr class='box-row'><td>%1</td></tr>\n" ).arg( str ) );
        }

        m_HTMLSource.append(
                "</table>\n"
                "</div>\n" );
    }

    m_HTMLSource.append(
            "<div id='albums_box' class='box'>\n"
            "<div id='albums_box-header' class='box-header'>\n"
            "<span id='albums_box-header-title' class='box-header-title'>\n"
            + ( channelInDB
                ? i18n( "Episodes from %1" ).arg( escapeHTML( pcb.title() ) )
                : i18n( "Episodes from this Channel" )
              )
            + "</span>\n"
            "</div>\n"
            "<table id='albums_box-body' class='box-body' width='100%' border='0' cellspacing='0' cellpadding='0'>\n" );

    uint i = 0;
    QValueList<PodcastEpisodeBundle> episodes = CollectionDB::instance()->getPodcastEpisodes( peb.parent() );
    while( !episodes.isEmpty() )
    {
        PodcastEpisodeBundle &ep = episodes.back();
        QString date;

        ep.dateTime().isNull() ?
                date = ep.date() :
                date = ep.dateTime().toString();

        m_HTMLSource.append( QStringx (
                    "<tr class='" + QString( (i % 2) ? "box-row-alt" : "box-row" ) + "'>\n"
                    "<td>\n"
                    "<div class='album-header' onClick=\"toggleBlock('IDE%1')\">\n"
                    "<table width='100%' border='0' cellspacing='0' cellpadding='0'>\n"
                    "<tr>\n"
                    "<td width='1'></td>\n"
                    "<td valign='middle' align='left'>\n"
                    "<span class='album-info'>%2</span> "
                    "<a href='%3'><span class='album-title'>%4</span></a>\n"
                    "<br />\n"
                    "<span class='album-year'>%5</span>\n"
                    "</td>\n"
                    "</tr>\n"
                    "</table>\n"
                    "</div>\n"
                    "<div class='album-body' style='display:%6;' id='IDE%7'>\n" )
                .args( QStringList()
                    << QString::number( i )
                    << escapeHTML( ep.duration() ? MetaBundle::prettyTime( ep.duration() ) : QString( "" ) )
                    << ( ep.localUrl().isValid()
                        ? ep.localUrl().url()
                        : ep.url().url().replace( QRegExp( "^http:" ), "stream:" ) )
                    << escapeHTML( ep.title() )
                    << escapeHTML( date )
                    << (peb.url() == ep.url() ? "block" : "none" )
                    << QString::number( i )
                    )
                );

        m_HTMLSource.append( QStringx ( "<p>%1</p>\n" ).arg( ep.description() ) );

        m_HTMLSource.append(
                "</div>\n"
                "</td>\n"
                "</tr>\n" );
        i++;
        episodes.pop_back();
    }

    m_HTMLSource.append("</body></html>\n" );
}

void CurrentTrackJob::showBrowseArtistHeader( const QString &artist )
{
    // <Artist>
    bool linkback = ( b->m_contextBackHistory.size() > 0 );
    QString back = ( linkback
            ? "<a id='artist-back-a' href='artistback://back'>\n"
            + escapeHTML( i18n( "<- Back" ) )
            + "</a>\n"
            : QString( "" )
            );
    m_HTMLSource.append(
            QString(
                "<div id='current_box' class='box'>\n"
                "<div id='current_box-header' class='box-header'>\n"
                "<span id='current_box-header-artist' class='box-header-title'>%1</span>\n"
                "<br />\n"
                "<table width='100%' cellpadding='0' cellspacing='0'><tr>\n"
                "<td><span id='current_box-header-album' class='box-header-title'>%2</span></td>\n"
                "<td><div id='current_box-header-nav' class='box-header-nav'>%3</div></td>\n"
                "</tr></table>\n"
                "</div>\n" )
            .arg( escapeHTML( artist ) )
            .arg( escapeHTML( i18n( "Browse Artist" ) ) )
            .arg( back ) );
    m_HTMLSource.append(
            "<table id='current_box-table' class='box-body' width='100%' cellpadding='0' cellspacing='0'>\n"
            );

    m_HTMLSource.append(
            "<tr>\n"
            "<td id='context'>\n"
            + QString( "<a id='context-a=' href='current://track'>\n" )
            + i18n( "Information for Current Track" )
            + "</a>\n"
            "</td>\n"
            "</tr>\n"
            );

    m_HTMLSource.append(
            "<tr>\n"
            "<td id='artist-wikipedia'>\n"
            + QString( "<a id='artist-wikipedia-a' href='wikipedia:%1'>\n" ).arg( escapeHTMLAttr( artist + b->wikiArtistPostfix() ) )
            + i18n( "Wikipedia Information for %1" ).arg( escapeHTML( artist ) ) +
            "</a>\n"
            "</td>\n"
            "</tr>\n");
    m_HTMLSource.append(
            "<tr>\n"
            "<td id='artist-google'>\n"
            + QString( "<a id='artist-google-a' href='ggartist:%1'>\n" ).arg( escapeHTMLAttr( artist ) )
            + i18n( "Google Musicsearch for %1" ).arg( escapeHTML( artist ) ) +
            "</a>\n"
            "</td>\n"
            "</tr>\n"
            );

    m_HTMLSource.append(
            "</td>\n"
            "</tr>\n"
            "</table>\n"
            "</div>\n" );
    // </Artist>
}

void
CurrentTrackJob::showBrowseLabelHeader( const QString &label )
{
    bool linkback = ( b->m_contextBackHistory.size() > 0 );
    QString back = ( linkback
            ? "<a id='artist-back-a' href='artistback://back'>\n"
            + escapeHTML( i18n( "<- Back" ) )
            + "</a>\n"
            : QString( "" )
            );
    m_HTMLSource.append(
            QString(
                "<div id='current_box' class='box'>\n"
                "<div id='current_box-header' class='box-header'>\n"
                "<span id='current_box-header-artist' class='box-header-title'>%1</span>\n"
                "<br />\n"
                "<table width='100%' cellpadding='0' cellspacing='0'><tr>\n"
                "<td><span id='current_box-header-album' class='box-header-title'>%2</span></td>\n"
                "<td><div id='current_box-header-nav' class='box-header-nav'>%3</div></td>\n"
                "</tr></table>\n"
                "</div>\n" )
            .arg( escapeHTML( label ) )
            .arg( escapeHTML( i18n( "Browse Label" ) ) )
            .arg( back ) );
    m_HTMLSource.append(
            "<table id='current_box-table' class='box-body' width='100%' cellpadding='0' cellspacing='0'>\n"
            );

    m_HTMLSource.append(
            "<tr>\n"
            "<td id='context'>\n"
            + QString( "<a id='context-a=' href='current://track'>\n" )
            + i18n( "Information for Current Track" )
            + "</a>\n"
            "</td>\n"
            "</tr>\n"
            );

    m_HTMLSource.append(
            "<tr>\n"
            "<td id='label-lastfm'>\n"
            + QString( "<a id='label-lastfm-a' href='externalurl://www.last.fm/tag/%1'>\n" ).arg( escapeHTMLAttr( label ) )
            + i18n( "Last.fm Information for %1" ).arg( escapeHTML( label ) ) +
            "</a>\n"
            "</td>\n"
            "</tr>\n");

    m_HTMLSource.append(
            "</td>\n"
            "</tr>\n"
            "</table>\n"
            "</div>\n" );
}

void CurrentTrackJob::showCurrentArtistHeader( const MetaBundle &currentTrack )
{
    QueryBuilder qb;
    QStringList values;
    // <Current Track Information>
    qb.clear();
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valCreateDate );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valAccessDate );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valPlayCounter );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valRating );
    qb.addMatch( QueryBuilder::tabStats, QueryBuilder::valURL, currentTrack.url().path() );
    values = qb.run();
    usleep( 10000 );

    //making 2 tables is most probably not the cleanest way to do it, but it works.
    QString albumImage = ContextBrowser::getEncodedImage( CollectionDB::instance()->albumImage( currentTrack, true, 1 ) );
    QString albumImageTitleAttr = albumImageTooltip( albumImage, 0 );

    bool isCompilation = false;
    if( !currentTrack.album().isEmpty() )
    {
        isCompilation = CollectionDB::instance()->albumIsCompilation(
                QString::number( CollectionDB::instance()->albumID( currentTrack.album() ) )
                );
    }

    m_HTMLSource.append(
            "<div id='current_box' class='box'>\n"
            "<div id='current_box-header' class='box-header'>\n"
            // Show "Title - Artist \n Album", or only "PrettyTitle" if there's no title tag
            + ( !currentTrack.title().isEmpty()
                ? QStringx(
                    "<span id='current_box-header-songname' class='box-header-title'>%1</span> "
                    "<span id='current_box-header-separator' class='box-header-title'>-</span> "
                    "<span id='current_box-header-artist' class='box-header-title'>%2</span>\n"
                    "<br />\n"
                    "<span id='current_box-header-album' class='box-header-title'>%3</span>\n"
                    "</div>\n"
                    "<table id='current_box-table' class='box-body' width='100%' cellpadding='0' cellspacing='0'>\n"
                    "<tr>\n"
                    "<td id='current_box-largecover-td'>\n"
                    "<a id='current_box-largecover-a' href='fetchcover:%4 @@@ %5'>\n"
                    "<img id='current_box-largecover-image' src='%6' title='%7'>\n"
                    "</a>\n"
                    "</td>\n"
                    "<td id='current_box-information-td' align='right'>\n"
                    "<div id='musicbrainz-div'>\n"
                    "<a id='musicbrainz-a' title='%8' href='musicbrainz:%9 @@@ %10 @@@ %11'>\n"
                    "<img id='musicbrainz-image' src='%12' />\n"
                    "</a>\n"
                    "</div>\n"
                    )
                .args( QStringList()
                        << escapeHTML( currentTrack.title() )
                        << escapeHTML( currentTrack.artist() )
                        << escapeHTML( currentTrack.album() )
                        << ( isCompilation ? "" : escapeHTMLAttr( currentTrack.artist() ) )
                        << escapeHTMLAttr( currentTrack.album() )
                        << escapeHTMLAttr( albumImage )
                        << albumImageTitleAttr
                        << i18n( "Look up this track at musicbrainz.org" )
                        << escapeHTMLAttr( currentTrack.artist() )
                        << escapeHTMLAttr( currentTrack.album() )
                        << escapeHTMLAttr( currentTrack.title() )
                        << escapeHTML( m_musicBrainIconPath ) )
                : QString ( //no title
                        "<span id='current_box-header-prettytitle' class='box-header-prettytitle'>%1</span> "
                        "</div>\n"
                        "<table id='current_box-table' class='box-body' width='100%' cellpadding='0' cellspacing='0'>\n"
                        "<tr>\n"
                        "<td id='current_box-largecover-td'>\n"
                        "<a id='current_box-largecover-a' href='fetchcover:%2 @@@ %3'>\n"
                        "<img id='current_box-largecover-image' src='%4' title='%5'>\n"
                        "</a>\n"
                        "</td>\n"
                        "<td id='current_box-information-td' align='right'>\n"
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
        const uint score = static_cast<uint>( values[3].toFloat() );
        const uint rating = values[4].toInt();

        //SAFE   = .arg( x, y )
        //UNSAFE = .arg( x ).arg( y )
        m_HTMLSource.append( QString(
                    "<span>%1</span><br />\n"
                    "<div>%2</div>\n"
                    "<span>%3</span><br />\n"
                    "<span>%4</span>\n"
                    )
                .arg( i18n( "Track played once", "Track played %n times", playtimes ),
                      statsHTML( score, rating, false ),
                      i18n( "Last played: %1" ).arg( Amarok::verboseTimeSince( lastPlay ) ),
                      i18n( "First played: %1" ).arg( Amarok::verboseTimeSince( firstPlay ) ) ) );
    }
    else
        m_HTMLSource.append( i18n( "Never played before" ) );

    m_HTMLSource.append(
            "</td>\n"
            "</tr>\n"
            "</table>\n"
            "</div>\n" );
    // </Current Track Information>

    if ( currentTrack.url().isLocalFile() && !CollectionDB::instance()->isFileInCollection( currentTrack.url().path() ) )
    {
        m_HTMLSource.append(
                "<div id='notindb_box' class='box'>\n"
                "<div id='notindb_box-header' class='box-header'>\n"
                "<span id='notindb_box-header-title' class='box-header-title'>\n"
                + i18n( "This file is not in your Collection!" ) +
                "</span>\n"
                "</div>\n"
                "<div id='notindb_box-body' class='box-body'>\n"
                "<div class='info'><p>\n"
                + i18n( "If you would like to see contextual information about this track,"
                    " you should add it to your Collection." ) +
                "</p></div>\n"
                "<div align='center'>\n"
                "<input type='button' onClick='window.location.href=\"show:collectionSetup\";' value='"
                + i18n( "Change Collection Setup..." ) +
                "' class='button' /></div><br />\n"
                "</div>\n"
                "</div>\n"
                );
    }

    /* cue file code */
    if ( b->m_cuefile && (b->m_cuefile->count() > 0) ) {
        m_HTMLSource.append(
                "<div id='cue_box' class='box'>\n"
                "<div id='cue_box-header' class='box-header'>\n"
                "<span id='cue_box-header-title' class='box-header-title' onClick=\"toggleBlock('T_CC'); window.location.href='togglebox:cc';\" style='cursor: pointer;'>\n"
                + i18n( "Cue File" ) +
                "</span>\n"
                "</div>\n"
                "<table id='cue_box-body' class='box-body' id='T_CC' width='100%' border='0' cellspacing='0' cellpadding='1'>\n" );
        CueFile::Iterator it;
        uint i = 0;
        for ( it = b->m_cuefile->begin(); it != b->m_cuefile->end(); ++it ) {
            m_HTMLSource.append(
                    "<tr class='" + QString( (i++ % 2) ? "box-row-alt" : "box-row" ) + "'>\n"
                    "<td class='song'>\n"
                    "<a href=\"seek:" + QString::number(it.key()) + "\">\n"
                    "<span class='album-song-trackno'>\n" + QString::number(it.data().getTrackNumber()) + "&nbsp;</span>\n"
                    "<span class='album-song-title'>\n" + escapeHTML( it.data().getTitle() ) + "</span>\n"
                    "<span class='song-separator'>\n"
                    + i18n("&#xa0;&#8211; ") +
                    "</span>\n"
                    "<span class='album-song-title'>\n" + escapeHTML( it.data().getArtist() ) + "</span>\n"
                    "<span class='album-song-time'>&nbsp;(" + MetaBundle::prettyTime( it.data().getLength()/1000, false ) + ")</span>\n"
                    "</a>\n"
                    "</td>\n"
                    ""
                    );
        }
        m_HTMLSource.append(
                "</table>\n"
                "</div>\n"
                );
    }
}


void CurrentTrackJob::showRelatedArtists( const QString &artist, const QStringList &relArtists )
{
    // <Related Artists>
    m_HTMLSource.append( QString(
                "<div id='related_box' class='box'>\n"
                "<div id='related_box-header' class='box-header' onClick=\"toggleBlock('T_RA'); window.location.href='togglebox:ra';\" style='cursor: pointer;'>\n"
                "<span id='related_box-header-title' class='box-header-title'>%1</span>\n"
                "</div>\n"
                "<table class='box-body' id='T_RA' width='100%' border='0' cellspacing='0' cellpadding='1'>\n" )
            .arg( i18n( "Artists Related to %1" ).arg( escapeHTML( artist ) ) ) );
    m_HTMLSource.append( "<tr><td>\n" );
    for ( uint i = 0; i < relArtists.count(); i += 1 )
    {
        bool isInCollection = !CollectionDB::instance()->albumListOfArtist( relArtists[i] ).isEmpty();
        m_HTMLSource.append(
                ( isInCollection ? "" : "<i>" )
                + QString( "<a href='artist:" ) + escapeHTMLAttr( relArtists[i] ) + "'>" + escapeHTML( relArtists[i] ) + "</a>"
                + ( isInCollection ? "" : "</i>" )
                );
        if( i != relArtists.count()-1 )
            m_HTMLSource.append( ", \n" );
    }

    m_HTMLSource.append( "</td></tr>\n" );
    m_HTMLSource.append(
            "</table>\n"
            "</div>\n" );

    if ( !b->m_relatedOpen )
        m_HTMLSource.append( "<script language='JavaScript'>toggleBlock('T_RA');</script>\n" );
    // </Related Artists>
}

void CurrentTrackJob::showSuggestedSongs( const QStringList &relArtists )
{
    QString token;

    QueryBuilder qb;
    QStringList values;
    qb.clear();
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valRating );
    qb.addMatches( QueryBuilder::tabArtist, relArtists );
    qb.sortByFavorite();
    qb.setLimit( 0, 10 );
    values = qb.run();

    // <Suggested Songs>
    if ( !values.isEmpty() )
    {
        m_HTMLSource.append(
                "<div id='suggested_box' class='box'>\n"
                "<div id='suggested_box-header' class='box-header' onClick=\"toggleBlock('T_SS'); window.location.href='togglebox:ss';\" style='cursor: pointer;'>\n"
                "<span id='suggested_box-header-title' class='box-header-title'>\n"
                + i18n( "Suggested Songs" ) +
                "</span>\n"
                "</div>\n"
                "<table class='box-body' id='T_SS' width='100%' border='0' cellspacing='0' cellpadding='0'>\n" );

        for ( uint i = 0; i < values.count(); i += 5 )
            m_HTMLSource.append(
                    "<tr class='" + QString( (i % 8) ? "box-row-alt" : "box-row" ) + "'>\n"
                    "<td class='song'>\n"
                    "<a href=\"file:" + escapeHTMLAttr ( values[i] ) + "\">\n"
                    "<span class='album-song-title'>\n"+ escapeHTML( values[i + 2] ) + "</span>\n"
                    "<span class='song-separator'>\n"
                    + i18n("&#xa0;&#8211; ") +
                    "</span><span class='album-song-title'>\n" + escapeHTML( values[i + 1] ) + "</span>\n"
                    "</a>\n"
                    "</td>\n"
                    "<td>\n" + statsHTML( static_cast<int>( values[i + 3].toFloat() ), values[i + 4].toInt() ) + "</td>\n"
                    "<td width='1'></td>\n"
                    "</tr>\n" );

        m_HTMLSource.append(
                "</table>\n"
                "</div>\n" );

        if ( !b->m_suggestionsOpen )
            m_HTMLSource.append( "<script language='JavaScript'>toggleBlock('T_SS');</script>\n" );
    }
    // </Suggested Songs>
}

void
CurrentTrackJob::showSongsWithLabel( const QString &label )
{
    QueryBuilder qb;
    QStringList values;
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valRating );
    qb.addMatch( QueryBuilder::tabLabels, QueryBuilder::valType, QString::number( CollectionDB::typeUser ) );
    qb.addMatch( QueryBuilder::tabLabels, QueryBuilder::valName, label );
    qb.sortByFavorite();
    qb.setOptions( QueryBuilder::optRandomize );
    qb.setLimit( 0, 30 );
    values = qb.run();

    if ( !values.isEmpty() )
    {
        m_HTMLSource.append(
                "<div id='suggested_box' class='box'>\n"
                "<div id='suggested_box-header' class='box-header' onClick=\"toggleBlock('T_SS'); window.location.href='togglebox:ss';\" style='cursor: pointer;'>\n"
                "<span id='suggested_box-header-title' class='box-header-title'>\n"
                + i18n( "Songs with label %1" ).arg( label ) +
                "</span>\n"
                "</div>\n"
                "<table class='box-body' id='T_' width='100%' border='0' cellspacing='0' cellpadding='0'>\n" );

        for ( uint i = 0; i < values.count(); i += 5 )
            m_HTMLSource.append(
                    "<tr class='" + QString( (i % 8) ? "box-row-alt" : "box-row" ) + "'>\n"
                    "<td class='song'>\n"
                    "<a href=\"file:" + escapeHTMLAttr ( values[i] ) + "\">\n"
                    "<span class='album-song-title'>\n"+ escapeHTML( values[i + 2] ) + "</span>\n"
                    "<span class='song-separator'>\n"
                    + i18n("&#xa0;&#8211; ") +
                    "</span><span class='album-song-title'>\n" + escapeHTML( values[i + 1] ) + "</span>\n"
                    "</a>\n"
                    "</td>\n"
                    "<td>\n" + statsHTML( static_cast<int>( values[i + 3].toFloat() ), values[i + 4].toInt() ) + "</td>\n"
                    "<td width='1'></td>\n"
                    "</tr>\n" );

        m_HTMLSource.append(
                "</table>\n"
                "</div>\n" );
    }
}

void
CurrentTrackJob::showUserLabels( const MetaBundle &currentTrack )
{
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabLabels, QueryBuilder::valName, true );
    qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valURL, currentTrack.url().path() );
    qb.addMatch( QueryBuilder::tabLabels, QueryBuilder::valType, QString::number( CollectionDB::typeUser ) );
    qb.setLimit( 0, 10 );
    qb.sortBy( QueryBuilder::tabLabels, QueryBuilder::valName, false );
    qb.buildQuery();
    QStringList values = qb.run();

    QString title;
    if ( currentTrack.title().isEmpty() )
        title = currentTrack.veryNiceTitle();
    else
        title = currentTrack.title();

     m_HTMLSource.append(
                "<div id='songlabels_box' class='box'>\n"
                "<div id='songlabels-header' class='box-header' onCLick=\"toggleBlock('T_SL');window.location.href='togglebox:sl';\" style='cursor: pointer;'>\n"
                "<span id='songlabels_box-header-title' class='box-header-title'>\n"
                + i18n( " Labels for %1 " ).arg( escapeHTML( title ) ) +
                "</span>\n"
                "</div>\n"
                "<table class='box-body' id='T_SL' width='100%' border='0' cellspacing='0' cellpadding='1'>\n" );
    m_HTMLSource.append( "<tr><td>\n" );
    if ( !values.isEmpty() )
    {
        foreach( values )
        {
            if( it != values.begin() )
                m_HTMLSource.append( ", \n" );
            m_HTMLSource.append( "<a href='showlabel:" + escapeHTMLAttr( *it ) + "'>" + escapeHTML( *it ) + "</a>" );
        }
    }
    m_HTMLSource.append( "</td></tr>\n" );
    m_HTMLSource.append( "<tr><td><a id='songlabels_box_addlabel' href='show:editLabels'>" + i18n( "Add labels to %1" ).arg( escapeHTML( title ) ) + "</a></td></tr>\n" );
    m_HTMLSource.append(
            "</table>\n"
            "</div>\n" );

    if ( !b->m_labelsOpen )
        m_HTMLSource.append( "<script language='JavaScript'>toggleBlock('T_SL');</script>\n" );
}

void CurrentTrackJob::showArtistsFaves( const QString &artist, uint artist_id )
{
    QString artistName = artist.isEmpty() ? escapeHTML( i18n( "This Artist" ) ) : escapeHTML( artist );
    QueryBuilder qb;
    QStringList values;

    qb.clear();
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valRating );
    qb.addNumericFilter( QueryBuilder::tabStats, QueryBuilder::valPlayCounter, "0", QueryBuilder::modeGreater );
    qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, QString::number( artist_id ) );
    qb.sortByFavorite();
    qb.setLimit( 0, 10 );
    values = qb.run();
    usleep( 10000 );

    if ( !values.isEmpty() )
    {
        m_HTMLSource.append(
                "<div id='favoritesby_box' class='box'>\n"
                "<div id='favoritesby-header' class='box-header' onClick=\"toggleBlock('T_FT'); window.location.href='togglebox:ft';\" style='cursor: pointer;'>\n"
                "<span id='favoritesby_box-header-title' class='box-header-title'>\n"
                + i18n( "Favorite Tracks by %1" ).arg( artistName ) +
                "</span>\n"
                "</div>\n"
                "<table class='box-body' id='T_FT' width='100%' border='0' cellspacing='0' cellpadding='0'>\n" );

        for ( uint i = 0; i < values.count(); i += 4 )
            m_HTMLSource.append(
                    "<tr class='" + QString( (i % 6) ? "box-row-alt" : "box-row" ) + "'>\n"
                    "<td class='song'>\n"
                    "<a href=\"file:" + escapeHTMLAttr ( values[i + 1] ) + "\">\n"
                    "<span class='album-song-title'>\n" + escapeHTML( values[i] ) + "</span>\n"
                    "</a>\n"
                    "</td>\n"
                    "<td>\n" + statsHTML( static_cast<int>( values[i + 2].toFloat() ), values[i + 3].toInt() ) + "</td>\n"
                    "<td width='1'></td>\n"
                    "</tr>\n"
                    );

        m_HTMLSource.append(
                "</table>\n"
                "</div>\n" );

        if ( !b->m_favoritesOpen )
            m_HTMLSource.append( "<script language='JavaScript'>toggleBlock('T_FT');</script>\n" );

    }
}

void CurrentTrackJob::showArtistsAlbums( const QString &artist, uint artist_id, uint album_id )
{
    DEBUG_BLOCK
    QString artistName = artist.isEmpty() ? escapeHTML( i18n( "This Artist" ) ) : escapeHTML( artist );
    QueryBuilder qb;
    QStringList values;
    // <Albums by this artist>
    qb.clear();
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valID );
    qb.addReturnFunctionValue( QueryBuilder::funcMax, QueryBuilder::tabYear, QueryBuilder::valName );
    qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, QString::number( artist_id ) );
    qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valID );
    qb.sortByFunction( QueryBuilder::funcMax, QueryBuilder::tabYear, QueryBuilder::valName, true );
    qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName, true );
    qb.setOptions( QueryBuilder::optNoCompilations );
    values = qb.run();

    if ( !values.isEmpty() )
    {
        // write the script to toggle blocks visibility
        m_HTMLSource.append(
                "<div id='albums_box' class='box'>\n"
                "<div id='albums_box-header' class='box-header'>\n"
                "<span id='albums_box-header-title' class='box-header-title'>\n"
                + i18n( "Albums by %1" ).arg( artistName ) +
                "</span>\n"
                "</div>\n"
                "<table id='albums_box-body' class='box-body' width='100%' border='0' cellspacing='0' cellpadding='0'>\n" );

        uint vectorPlace = 0;
        // find album of the current track (if it exists)
        while ( vectorPlace < values.count() && values[ vectorPlace+1 ] != QString::number( album_id ) )
            vectorPlace += 3;
        for ( uint i = 0; i < values.count(); i += 3 )
        {
            qb.clear();
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTrack );
            qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valLength );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
            qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, values[ i + 1 ] );
            qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, QString::number( artist_id ) );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTitle );
            qb.setOptions( QueryBuilder::optNoCompilations );
            QStringList albumValues = qb.run();
            usleep( 10000 );

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

            uint i_albumLength = 0;
            for ( uint j = 0; j < albumValues.count(); j += qb.countReturnValues() )
                i_albumLength += QString(albumValues[j + 4]).toInt();

            QString albumLength = ( i_albumLength==0 ? i18n( "Unknown" ) : MetaBundle::prettyTime( i_albumLength, true ) );
            QString albumImage = ContextBrowser::getEncodedImage( CollectionDB::instance()->albumImage( artist, values[ i ], true, 50 ) );
            QString albumImageTitleAttr = albumImageTooltip( albumImage, 50 );

            m_HTMLSource.append( QStringx (
                        "<tr class='" + QString( (i % 4) ? "box-row-alt" : "box-row" ) + "'>\n"
                        "<td>\n"
                        "<div class='album-header' onClick=\"toggleBlock('IDA%1')\">\n"
                        "<table width='100%' border='0' cellspacing='0' cellpadding='0'>\n"
                        "<tr>\n"
                        "<td width='1'>\n"
                        "<a href='fetchcover:%2 @@@ %3'>\n"
                        "<img class='album-image' align='left' vspace='2' hspace='2' title='%4' src='%5'/>\n"
                        "</a>\n"
                        "</td>\n"
                        "<td valign='middle' align='left'>\n"
                        "<span class='album-info'>%6</span> "
                        "<a href='album:%7 @@@ %8'><span class='album-title'>%9</span></a>\n"
                        "<br />\n"
                        "<span class='album-year'>%10</span>\n"
                        "<span class='album-length'>%11</span>\n"
                        "</td>\n"
                        "</tr>\n"
                        "</table>\n"
                        "</div>\n"
                        "<div class='album-body' style='display:%12;' id='IDA%13'>\n" )
                    .args( QStringList()
                        << values[ i + 1 ]
                        << escapeHTMLAttr( artist ) // artist name
                        << escapeHTMLAttr( values[ i ].isEmpty() ? i18n( "Unknown" ) : values[ i ] ) // album.name
                        << albumImageTitleAttr
                        << escapeHTMLAttr( albumImage )
                        << i18n( "Single", "%n Tracks",  albumValues.count() / qb.countReturnValues() )
                        << QString::number( artist_id )
                        << values[ i + 1 ] //album.id
                        << escapeHTML( values[ i ].isEmpty() ? i18n( "Unknown" ) : values[ i ] )
                        << albumYear
                        << albumLength
                        << ( i!=vectorPlace ? "none" : "block" ) /* shows it if it's the current track album */
                        << values[ i + 1 ] ) );

            QString discNumber;
            if ( !albumValues.isEmpty() )
                for ( uint j = 0; j < albumValues.count(); j += qb.countReturnValues() )
                {
                    QString newDiscNumber = albumValues[ j + 5 ].stripWhiteSpace();
                    if( discNumber != newDiscNumber && newDiscNumber.toInt() > 0)
                    {
                        discNumber = newDiscNumber;
                        m_HTMLSource.append( QStringx (
                                                 "<div class='disc-separator'>\n"
                                                 "<a href=\"albumdisc: %1 @@@ %2 @@@ %3\">\n"
                                                 "%4"
                                                 "</a>\n"
                                                 "</div>\n" )
                                             .args( QStringList()
                                                    << QString::number( artist_id )
                                                    << values[ i + 1 ] //album.id
                                                    << escapeHTMLAttr( discNumber )
                                                    << i18n( "Disc %1" ).arg( discNumber ) ) );
                    }
                    QString track = albumValues[j + 2].stripWhiteSpace();
                    if( track.length() > 0 ) {
                        if( track.length() == 1 )
                            track.prepend( "0" );

                        track = "<span class='album-song-trackno'>\n" + track + "&nbsp;</span>\n";
                    }

                    QString length;
                    if( albumValues[j + 4] != "0" )
                        length = "<span class='album-song-time'>(" + MetaBundle::prettyTime( QString(albumValues[j + 4]).toInt(), true ) + ")</span>\n";

                    bool current = false;
                    if( i==vectorPlace && albumValues[j + 2].toInt() == m_currentTrack.track() && discNumber.toInt() == m_currentTrack.discNumber() )
                        current = true;
                    m_HTMLSource.append(
                            "<div class='album-song'>\n"
                            "<a href=\"file:" + escapeHTMLAttr ( albumValues[j + 1] ) + "\">\n"
                            + track +
                            "<span class='album-song-title'>\n" + (current?"<i>":"") + escapeHTML( albumValues[j] ) + (current?"</i>":"") + "</span>&nbsp;"
                            + length +
                            "</a>\n"
                            "</div>\n" );
                }

            m_HTMLSource.append(
                    "</div>\n"
                    "</td>\n"
                    "</tr>\n" );
        }
        m_HTMLSource.append(
                "</table>\n"
                "</div>\n" );
    }
    // </Albums by this artist>
}

void CurrentTrackJob::showArtistsCompilations( const QString &artist, uint artist_id, uint album_id )
{
    QString artistName = artist.isEmpty() ? escapeHTML( i18n( "This Artist" ) ) : escapeHTML( artist );
    QueryBuilder qb;
    QStringList values;
    // <Compilations with this artist>
    qb.clear();
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valID );
    qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, QString::number( artist_id ) );
    qb.sortBy( QueryBuilder::tabYear, QueryBuilder::valName, true );
    qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName, true );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.setOptions( QueryBuilder::optOnlyCompilations );
    values = qb.run();

    if ( !values.isEmpty() )
    {
        // write the script to toggle blocks visibility
        m_HTMLSource.append(
                "<div id='albums_box' class='box'>\n"
                "<div id='albums_box-header' class='box-header'>\n"
                "<span id='albums_box-header-title' class='box-header-title'>\n"
                + i18n( "Compilations with %1" ).arg( artistName ) +
                "</span>\n"
                "</div>\n"
                "<table id='albums_box-body' class='box-body' width='100%' border='0' cellspacing='0' cellpadding='0'>\n" );

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
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
            qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, values[ i + 1 ] );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
            qb.setOptions( QueryBuilder::optOnlyCompilations );
            QStringList albumValues = qb.run();
            usleep( 10000 );

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

            uint i_albumLength = 0;
            for ( uint j = 0; j < albumValues.count(); j += qb.countReturnValues() )
                i_albumLength += QString(albumValues[j + 4]).toInt();

            QString albumLength = ( i_albumLength==0 ? i18n( "Unknown" ) : MetaBundle::prettyTime( i_albumLength, true ) );
            QString albumImage = ContextBrowser::getEncodedImage( CollectionDB::instance()->albumImage( artist, values[ i ], true, 50 ) );
            QString albumImageTitleAttr = albumImageTooltip( albumImage, 50 );

            m_HTMLSource.append( QStringx (
                        "<tr class='" + QString( (i % 4) ? "box-row-alt" : "box-row" ) + "'>\n"
                        "<td>\n"
                        "<div class='album-header' onClick=\"toggleBlock('IDA%1')\">\n"
                        "<table width='100%' border='0' cellspacing='0' cellpadding='0'>\n"
                        "<tr>\n"
                        "<td width='1'>\n"
                        "<a href='fetchcover: @@@ %2'>\n"
                        "<img class='album-image' align='left' vspace='2' hspace='2' title='%3' src='%4'/>\n"
                        "</a>\n"
                        "</td>\n"
                        "<td valign='middle' align='left'>\n"
                        "<span class='album-info'>%5</span> "
                        "<a href='compilation:%6'><span class='album-title'>%7</span></a>\n"
                        "<br />\n"
                        "<span class='album-year'>%8</span>\n"
                        "<span class='album-length'>%9</span>\n"
                        "</td>\n"
                        "</tr>\n"
                        "</table>\n"
                        "</div>\n"
                        "<div class='album-body' style='display:%10;' id='IDA%11'>\n" )
                    .args( QStringList()
                        << values[ i + 1 ]
                        << escapeHTMLAttr( values[ i ].isEmpty() ? i18n( "Unknown" ) : values[ i ] ) // album.name
                        << albumImageTitleAttr
                        << escapeHTMLAttr( albumImage )
                        << i18n( "Single", "%n Tracks",  albumValues.count() / qb.countReturnValues() )
                        << values[ i + 1 ] //album.id
                        << escapeHTML( values[ i ].isEmpty() ? i18n( "Unknown" ) : values[ i ] )
                        << albumYear
                        << albumLength
                        << ( i!=vectorPlace ? "none" : "block" ) /* shows it if it's the current track album */
                        << values[ i + 1 ] ) );

            QString discNumber;
            if ( !albumValues.isEmpty() )
                for ( uint j = 0; j < albumValues.count(); j += qb.countReturnValues() )
                {
                    QString newDiscNumber = albumValues[ j + 6 ].stripWhiteSpace();
                    if( discNumber != newDiscNumber && newDiscNumber.toInt() > 0)
                    {
                        discNumber = newDiscNumber;
                        m_HTMLSource.append( QStringx (
                                                 "<div class='disc-separator'>\n"
                                                 "<a href=\"compilationdisc: __discard__ @@@ %1 @@@ %2\">\n"
                                                 "%3"
                                                 "</a>\n"
                                                 "</div>\n" )
                                             .args( QStringList()
                                                    << values[ i + 1 ] //album.id
                                                    << escapeHTMLAttr( discNumber )
                                                    << i18n( "Disc %1" ).arg( discNumber ) ) );
                    }

                    QString track = albumValues[j + 2].stripWhiteSpace();
                    if( track.length() > 0 ) {
                        if( track.length() == 1 )
                            track.prepend( "0" );

                        track = "<span class='album-song-trackno'>\n" + track + "&nbsp;</span>\n";
                    }

                    QString length;
                    if( albumValues[j + 4] != "0" )
                        length = "<span class='album-song-time'>(" + MetaBundle::prettyTime( QString(albumValues[j + 4]).toInt(), true ) + ")</span>\n";

                    QString tracktitle_formated;
                    QString tracktitle;
                    tracktitle = escapeHTML( i18n("%1 - %2").arg( albumValues[j + 5], albumValues[j] ) );
                    tracktitle_formated = "<span class='album-song-title'>\n";
                    if( i==vectorPlace && albumValues[j + 2].toInt() == m_currentTrack.track() && discNumber.toInt() == m_currentTrack.discNumber() )
                        tracktitle_formated += "<i>\n";
                    if ( artist == albumValues[j + 5] )
                        tracktitle_formated += "<b>\n";
                    tracktitle_formated += tracktitle;
                    if ( artist == albumValues[j + 5] )
                        tracktitle_formated += "</b>\n";
                    if( i==vectorPlace && track.toInt() == m_currentTrack.track() && discNumber.toInt() == m_currentTrack.discNumber() )
                        tracktitle_formated += "</i>\n";
                    tracktitle_formated += "</span>&nbsp;";
                    m_HTMLSource.append(
                            "<div class='album-song'>\n"
                            "<a href=\"file:" + escapeHTMLAttr ( albumValues[j + 1] ) + "\">\n"
                            + track
                            + tracktitle_formated
                            + length +
                            "</a>\n"
                            "</div>\n" );
                }

            m_HTMLSource.append(
                    "</div>\n"
                    "</td>\n"
                    "</tr>\n" );
        }
        m_HTMLSource.append(
                "</table>\n"
                "</div>\n" );
    }
    // </Compilations with this artist>
}

QString CurrentTrackJob::statsHTML( int score, int rating, bool statsbox ) //static
{
    if( !AmarokConfig::useScores() && !AmarokConfig::useRatings() )
        return "";

    if ( rating < 0 )
        rating = 0;
    if ( rating > 10 )
        rating = 10;

    QString table = QString( "<table %1 align='right' border='0' cellspacing='0' cellpadding='0' width='100%'>%2</table>\n" )
                          .arg( statsbox ? "class='statsBox'" : "" );
    QString contents;

    if( AmarokConfig::useScores() )
        contents += QString( "<tr title='%1'>\n" ).arg( i18n( "Score: %1" ).arg( score ) ) +
                    "<td class='sbtext' width='100%' align='right'>\n" + QString::number( score ) + "</td>\n"
                    "<td align='left' width='1'>\n"
                    "<div class='sbouter'>\n"
                    "<div class='sbinner' style='width: "
                    + QString::number( score / 2 ) + "px;'></div>\n"
                    "</div>\n"
                    "</td>\n"
                    "</tr>\n";

    if( AmarokConfig::useRatings() )
    {
        contents += QString( "<tr title='%1'>\n" ).arg( i18n( "Rating: %1" )
                                                      .arg( MetaBundle::ratingDescription( rating ) ) ) +
                    "<td class='ratingBox' align='right' colspan='2'>\n";
        if( rating )
        {
            bool half = rating%2;
            contents += "<nobr>\n";

            QImageIO fullStarIO;
            fullStarIO.setImage( StarManager::instance()->getStarImage( half ? rating/2 + 1 : rating/2 ) );
            fullStarIO.setFormat( "PNG" );
            QBuffer fullStarBuf;
            fullStarBuf.open( IO_WriteOnly );
            fullStarIO.setIODevice( &fullStarBuf );
            fullStarIO.write();
            fullStarBuf.close();
            QCString fullStar = KCodecs::base64Encode( fullStarBuf.buffer(), true );

            const QString img = "<img src='%1' height='13px' class='ratingStar'></img>\n";
            for( int i = 0, n = rating / 2; i < n; ++i )
                contents += img.arg( "data:image/png;base64," + fullStar );
            if( rating % 2 )
            {
                QImageIO halfStarIO;
                halfStarIO.setImage( StarManager::instance()->getHalfStarImage( half ? rating/2 + 1 : rating/2 ) );
                halfStarIO.setFormat( "PNG" );
                QBuffer halfStarBuf;
                halfStarBuf.open( IO_WriteOnly );
                halfStarIO.setIODevice( &halfStarBuf );
                halfStarIO.write();
                halfStarBuf.close();
                QCString halfStar = KCodecs::base64Encode( halfStarBuf.buffer(), true );
                contents += img.arg( "data:image/png;base64," + halfStar );
            }
            contents += "</nobr>\n";
        }
        else
            contents += i18n( "Not rated" );
        contents += "</td>\n"
                    "</tr>\n";
    }

    return table.arg( contents );
}

bool CurrentTrackJob::doJob()
{
    m_HTMLSource.append( "<html><body>\n"
                    "<script type='text/javascript'>\n"
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
                    "</script>\n" );

    if( !b->m_browseArtists )
    {
        if( !EngineController::engine()->loaded() )
        {
            showHome();
            return true;
        }
        MetaBundle mb( m_currentTrack.url() );
        if( mb.podcastBundle() )
        {
            showPodcast( mb );
            return true;
        }

        if( m_currentTrack.url().protocol() == "lastfm" )
        {
            showLastFm( m_currentTrack );
            return true;
        }

        if( m_isStream && m_currentTrack.url().protocol() != "daap" )
        {
            showStream( m_currentTrack );
            return true;
        }
    }

    QString artist;
    if( b->m_browseArtists )
    {
        artist = b->m_artist;
        if( artist == m_currentTrack.artist() )
        {
            b->m_browseArtists = false;
            b->m_artist = QString::null;
            b->m_contextBackHistory.clear();
            b->m_contextBackHistory.push_back( "current://track" );
        }
    }
    else
        artist = m_currentTrack.artist();

    const uint artist_id = CollectionDB::instance()->artistID( artist );
    const uint album_id  = CollectionDB::instance()->albumID ( m_currentTrack.album() );
    QueryBuilder qb;
    QStringList values;
    if( b->m_browseArtists )
        showBrowseArtistHeader( artist );
    else if( b->m_browseLabels )
    {
        showBrowseLabelHeader( b->m_label );
        showSongsWithLabel( b->m_label );
        m_HTMLSource.append( "</body></html>\n" );

        return true;
    }
    else
        showCurrentArtistHeader( m_currentTrack );

    if ( ContextBrowser::instance()->m_showLabels && !b->m_browseArtists )
        showUserLabels( m_currentTrack );

    if( ContextBrowser::instance()->m_showRelated || ContextBrowser::instance()->m_showSuggested )
    {
        QStringList relArtists = CollectionDB::instance()->similarArtists( artist, 10 );
        if ( !relArtists.isEmpty() )
        {
            if( ContextBrowser::instance()->m_showRelated )
                showRelatedArtists( artist, relArtists );

            if( ContextBrowser::instance()->m_showSuggested )
                showSuggestedSongs( relArtists );
        }
    }

    QString artistName = artist.isEmpty() ? i18n( "This Artist" ) : artist ;
    if ( !artist.isEmpty() )
    {
        if( ContextBrowser::instance()->m_showFaves )
            showArtistsFaves( artistName, artist_id );

        showArtistsAlbums( artist, artist_id, album_id );
        showArtistsCompilations( artist, artist_id, album_id );
    }
    m_HTMLSource.append( "</body></html>\n" );

    return true;
}


void ContextBrowser::showIntroduction()
{
    if ( currentPage() != m_contextTab )
    {
        blockSignals( true );
        showPage( m_contextTab );
        blockSignals( false );
    }

    // Do we have to rebuild the page? I don't care
    m_HTMLSource = QString::null;
    m_HTMLSource.append(
            "<html><body>\n"
            "<div id='introduction_box' class='box'>\n"
                "<div id='introduction_box-header' class='box-header'>\n"
                    "<span id='introduction_box-header-title' class='box-header-title'>\n"
                    + i18n( "Hello Amarok user!" ) +
                    "</span>\n"
                "</div>\n"
                "<div id='introduction_box-body' class='box-body'>\n"
                    "<div class='info'><p>\n" +
                    i18n( "This is the Context Browser: "
                          "it shows you contextual information about the currently playing track. "
                          "In order to use this feature of Amarok, you need to build a Collection."
                        ) +
                    "</p></div>\n"
                    "<div align='center'>\n"
                    "<input type='button' onClick='window.location.href=\"show:collectionSetup\";' value='" +
                    i18n( "Build Collection..." ) +
                    "'></div><br />\n"
                "</div>\n"
            "</div>\n"
            "</body></html>\n"
                       );

    m_currentTrackPage->set( m_HTMLSource );
    saveHtmlData(); // Send html code to file
}


void ContextBrowser::showScanning()
{
    if ( currentPage() != m_contextTab )
    {
        blockSignals( true );
        showPage( m_contextTab );
        blockSignals( false );
    }

    // Do we have to rebuild the page? I don't care
    m_HTMLSource="";
    m_HTMLSource.append(
            "<html><body>\n"
            "<div id='building_box' class='box'>\n"
                "<div id='building_box-header' class='box-header'>\n"
                    "<span id='building_box-header-title' class='box-header-title'>\n"
                    + i18n( "Building Collection Database..." ) +
                    "</span>\n"
                "</div>\n"
                "<div id='building_box-body' class='box-body'>\n"
                    "<div class='info'><p>\n" + i18n( "Please be patient while Amarok scans your music collection. You can watch the progress of this activity in the statusbar." ) + "</p></div>\n"
                "</div>\n"
            "</div>\n"
            "</body></html>\n"
                       );

    m_currentTrackPage->set( m_HTMLSource );
    saveHtmlData(); // Send html code to file
}

QString
ContextBrowser::getEncodedImage( const QString &imageUrl )
{
    // Embed cover image in html (encoded string), to get around khtml's caching
    //debug() << "Encoding imageUrl: " << imageUrl << endl;
    qApp->lock();
    const QImage img( imageUrl, "PNG" );
    qApp->unlock();
    QByteArray ba;
    QBuffer buffer( ba );
    buffer.open( IO_WriteOnly );
    qApp->lock();
    img.save( &buffer, "PNG" ); // writes image into ba in PNG format
    qApp->unlock();
    const QString coverImage = QString( "data:image/png;base64,%1" ).arg( KCodecs::base64Encode( ba ) );
    //debug() << "Encoded imageUrl: " << coverImage << endl;
    return coverImage;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Lyrics-Tab
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::showLyrics( const QString &url )
{
    #if 0
    if( BrowserBar::instance()->currentBrowser() != this )
    {
        debug() << "current browser is not context, aborting showLyrics()" << endl;
        m_dirtyLyricsPage = true;
        return;
    }
    #endif

    DEBUG_BLOCK

    if ( currentPage() != m_lyricsTab )
    {
        blockSignals( true );
        showPage( m_lyricsTab );
        blockSignals( false );
    }
    if ( !m_dirtyLyricsPage ) return;

    QString lyrics = CollectionDB::instance()->getLyrics( EngineController::instance()->bundle().url().path() );
    // don't rely on caching for streams
    const bool cached = !lyrics.isEmpty() && !EngineController::engine()->isStream();
    QString title  = EngineController::instance()->bundle().title();
    QString artist = EngineController::instance()->bundle().artist();

    if( title.contains("PREVIEW: buy it at www.magnatune.com", true) >= 1 )
        title = title.remove(" (PREVIEW: buy it at www.magnatune.com)");
    if( artist.contains("PREVIEW: buy it at www.magnatune.com", true) >= 1 )
        artist = artist.remove(" (PREVIEW: buy it at www.magnatune.com)");

    if ( title.isEmpty() ) {
        /* If title is empty, try to use pretty title.
           The fact that it often (but not always) has artist name together, can be bad,
           but at least the user will hopefully get nice suggestions. */
        QString prettyTitle = EngineController::instance()->bundle().prettyTitle();
        int h = prettyTitle.find( '-' );
        if ( h != -1 )
        {
            title = prettyTitle.mid( h+1 ).stripWhiteSpace();
            if( title.contains("PREVIEW: buy it at www.magnatune.com", true) >= 1 )
                title = title.remove(" (PREVIEW: buy it at www.magnatune.com)");
            if ( artist.isEmpty() ) {
                artist = prettyTitle.mid( 0, h ).stripWhiteSpace();
                if( artist.contains("PREVIEW: buy it at www.magnatune.com", true) >= 1 )
                    artist = artist.remove(" (PREVIEW: buy it at www.magnatune.com)");
            }

        }
    }

    m_lyricSearchUrl = QString( "http://www.google.com/search?ie=UTF-8&q=lyrics+%1+%2" )
        .arg( KURL::encode_string_no_slash( '"' + artist + '"', 106 /*utf-8*/ ),
              KURL::encode_string_no_slash( '"' + title  + '"', 106 /*utf-8*/ ) );

    m_lyricsToolBar->getButton( LYRICS_BROWSER )->setEnabled(false);

    if( ( !cached || url == "reload" ) && !ScriptManager::instance()->lyricsScriptRunning() ) {
        const QStringList scripts = ScriptManager::instance()->lyricsScripts();
        lyrics =
              i18n( "Sorry, no lyrics script running.") + "<br />\n" +
              "<br /><div class='info'>\n"+
              i18n( "Available Lyrics Scripts:" ) + "<br />\n";
        foreach ( scripts ) {
            lyrics += QString( "<a href=\"runscript:%1\">%2</a><br />\n" ).arg( *it, *it );
        }
        lyrics += "<br />\n" + i18n( "Click on one of the scripts to run it, or use the Script Manager, to be able"
                        " to see all the scripts, and download new ones from the Web." );
        lyrics += "<br /><div align='center'>\n"
                  "<form><input type='button' onClick=\"window.location='show:scriptmanager'\" value='" +
                    i18n( "Run Script Manager..." ) +
                  "'></form></div><br /></div>\n";

        m_HTMLSource = QString (
            "<html><body>\n"
            "<div id='lyrics_box' class='box'>\n"
                "<div id='lyrics_box-header' class='box-header'>\n"
                    "<span id='lyrics_box-header-title' class='box-header-title'>\n"
                    + ( cached ? i18n( "Cached Lyrics" ) : i18n( "Lyrics" ) ) +
                    "</span>\n"
                "</div>\n"
                "<div id='lyrics_box-body' class='box-body'>\n"
                    + lyrics +
                "</div>\n"
            "</div>\n"
            "</body></html>\n"
            );
        m_lyricsPage->set( m_HTMLSource );

        m_dirtyLyricsPage = false;
        saveHtmlData(); // Send html code to file

        return;
    }

    if( cached && url.isEmpty() )
    {
        lyricsResult( lyrics.utf8(), true );
    }
    else
    {
        m_HTMLSource = QString (
            "<html><body>\n"
            "<div id='lyrics_box' class='box'>\n"
                "<div id='lyrics_box-header' class='box-header'>\n"
                    "<span id='lyrics_box-header-title' class='box-header-title'>\n"
                    + i18n( "Fetching Lyrics" ) +
                    "</span>\n"
                "</div>\n"
                "<div id='lyrics_box-body' class='box-body'>\n"
                    "<div class='info'><p>\n" + i18n( "Fetching Lyrics..." ) + "</p></div>\n"
                "</div>\n"
            "</div>\n"
            "</body></html>\n"
            );
        m_lyricsPage->set( m_HTMLSource );
        saveHtmlData(); // Send html code to file


        if( url.isNull() || url == "reload" )
            ScriptManager::instance()->notifyFetchLyrics( artist, title );
        else
            ScriptManager::instance()->notifyFetchLyricsByUrl( url );
    }
}


void
ContextBrowser::lyricsResult( QCString cXmlDoc, bool cached ) //SLOT
{
    QDomDocument doc;
    QString xmldoc = QString::fromUtf8( cXmlDoc );
    if( !doc.setContent( xmldoc ) )
    {
        m_HTMLSource="";
        m_HTMLSource.append(
                "<html><body>\n"
                "<div id='lyrics_box' class='box'>\n"
                    "<div id='lyrics_box-header' class='box-header'>\n"
                        "<span id='lyrics_box-header-title' class='box-header-title'>\n"
                        + i18n( "Error" ) +
                        "</span>\n"
                    "</div>\n"
                    "<div id='lyrics_box-body' class='box-body'><p>\n"
                        + i18n( "Lyrics could not be retrieved because the server was not reachable." ) +
                    "</p></div>\n"
                "</div>\n"
                "</body></html>\n"
                        );
        m_lyricsPage->set( m_HTMLSource );
        saveHtmlData(); // Send html code to file

        m_dirtyLyricsPage = false;

        return;
    }

    QString lyrics;

    QDomElement el = doc.documentElement();
    m_lyricCurrentUrl = el.attribute( "page_url" );

    ScriptManager* const sm = ScriptManager::instance();
    KConfig spec( sm->specForScript( sm->lyricsScriptRunning() ), true, false );
    spec.setGroup( "Lyrics" );

    if ( el.attribute( "add_url" ).isEmpty() )
    {
        m_lyricAddUrl = spec.readPathEntry( "add_url" );
        m_lyricAddUrl.replace( "MAGIC_ARTIST", KURL::encode_string_no_slash( EngineController::instance()->bundle().artist() ) );
        m_lyricAddUrl.replace( "MAGIC_TITLE", KURL::encode_string_no_slash( EngineController::instance()->bundle().title() ) );
        m_lyricAddUrl.replace( "MAGIC_ALBUM", KURL::encode_string_no_slash( EngineController::instance()->bundle().album() ) );
        m_lyricAddUrl.replace( "MAGIC_YEAR", KURL::encode_string_no_slash( QString::number( EngineController::instance()->bundle().year() ) ) );
    }
    else
        m_lyricAddUrl = el.attribute( "add_url" );

    if ( el.tagName() == "suggestions" )
    {


        const QDomNodeList l = doc.elementsByTagName( "suggestion" );

        if( l.length() ==0 )
        {
            lyrics = i18n( "Lyrics for track not found" );
        }
        else
        {
            lyrics = i18n( "Lyrics for track not found, here are some suggestions:" ) + "<br/><br/>\n";
            for( uint i = 0; i < l.length(); ++i ) {
                const QString url    = l.item( i ).toElement().attribute( "url" );
                const QString artist = l.item( i ).toElement().attribute( "artist" );
                const QString title  = l.item( i ).toElement().attribute( "title" );

                lyrics += "<a href='show:suggestLyric-" + url + "'>\n" + i18n("%1 - %2").arg( artist, title );
                lyrics += "</a><br/>\n";
            }
        }
        lyrics += i18n( "<p>You can <a href=\"%1\">search for the lyrics</a> on the Web.</p>" )
                  .arg( QString( m_lyricSearchUrl ).replace( QRegExp( "^http:" ), "externalurl:" ) );
    }
    else {
        lyrics = el.text();
        lyrics.replace( "\n", "<br/>\n" ); // Plaintext -> HTML

        const QString title    = el.attribute( "title" );
        const QString artist   = el.attribute( "artist" );
        const QString site     = el.attribute( "site" ).isEmpty() ? spec.readEntry( "site" ) : el.attribute( "site" );
        const QString site_url = el.attribute( "site_url" ).isEmpty() ? spec.readEntry( "site_url" ) : el.attribute( "site_url" );

        lyrics.prepend( "<font size='2'><b>\n" + title + "</b><br/><u>\n" + artist+ "</font></u></font><br/>\n" );

        if( !cached ) {
            lyrics.append( "<br/><br/><i>\n" + i18n( "Powered by %1 (%2)" ).arg( site, site_url ) + "</i>\n" );
            CollectionDB::instance()->setLyrics( EngineController::instance()->bundle().url().path(), xmldoc, EngineController::instance()->bundle().uniqueId() );
        }
    }

    m_HTMLSource="";
    m_HTMLSource.append(
            "<html><body>\n"
            "<div id='lyrics_box' class='box'>\n"
                "<div id='lyrics_box-header' class='box-header'>\n"
                    "<span id='lyrics_box-header-title' class='box-header-title'>\n"
                    + ( cached ? i18n( "Cached Lyrics" ) : i18n( "Lyrics" ) ) +
                    "</span>\n"
                "</div>\n"
                "<div id='lyrics_box-body' class='box-body'>\n"
                    + lyrics +
                "</div>\n"
            "</div>\n"
            "</body></html>\n"
    );


    m_lyricsPage->set( m_HTMLSource );
    //Reset scroll
    m_lyricsPage->view()->setContentsPos(0, 0);
    saveHtmlData(); // Send html code to file

    m_lyricsToolBar->getButton( LYRICS_BROWSER )->setEnabled( !m_lyricCurrentUrl.isEmpty() );
    m_dirtyLyricsPage = false;
}


void
ContextBrowser::lyricsExternalPage() //SLOT
{
    Amarok::invokeBrowser( m_lyricCurrentUrl );
}


void
ContextBrowser::lyricsAdd() //SLOT
{
    Amarok::invokeBrowser( m_lyricAddUrl );
}

void
ContextBrowser::lyricsEditToggle() //SLOT
{
    if ( m_lyricsToolBar->getButton( LYRICS_EDIT )->isOn() )
    {
        m_lyricsBeingEditedUrl    = EngineController::instance()->bundle().url().path();
        m_lyricsBeingEditedArtist = EngineController::instance()->bundle().artist();
        m_lyricsBeingEditedTitle  = EngineController::instance()->bundle().title();
        QString xml = CollectionDB::instance()->getLyrics( m_lyricsBeingEditedUrl ), lyrics;
        QDomDocument doc;
        if( doc.setContent( xml ) )
            lyrics = doc.documentElement().text();
        else
            lyrics = QString::null;
        m_lyricsTextEdit->setText( lyrics );
        m_lyricsPage->hide();
        m_lyricsTextEdit->show();
    }
    else
    {
        m_lyricsTextEdit->hide();

        QDomDocument doc;
        QDomElement e = doc.createElement( "lyrics" );
        e.setAttribute( "artist", m_lyricsBeingEditedArtist );
        e.setAttribute( "title", m_lyricsBeingEditedTitle );
        QDomText t = doc.createTextNode( m_lyricsTextEdit->text() );
        e.appendChild( t );
        doc.appendChild( e );
        CollectionDB::instance()->setLyrics( m_lyricsBeingEditedUrl, doc.toString(), CollectionDB::instance()->uniqueIdFromUrl( KURL( m_lyricsBeingEditedUrl) ) );
        m_lyricsPage->show();
        lyricsChanged( m_lyricsBeingEditedUrl );
    }
}

void
ContextBrowser::lyricsSearch() //SLOT
{
    Amarok::invokeBrowser( m_lyricSearchUrl );
}


void
ContextBrowser::lyricsRefresh() //SLOT
{
    m_dirtyLyricsPage = true;
    showLyrics( "reload" );
}

void
ContextBrowser::lyricsSearchText(QString const &text) //SLOT
{
    m_lyricsPage->findText( text, 0 );
    lyricsSearchTextNext();
}

void
ContextBrowser::lyricsSearchTextNext() //SLOT
{
    m_lyricsPage->findTextNext();
}

void
ContextBrowser::lyricsSearchTextShow() //SLOT
{
    m_lyricsSearchText->setEnabled( true );
    m_lyricsTextBar->show();
    m_lyricsTextBarShowed = true;
    m_lyricsSearchText->setFocus();
}


void
ContextBrowser::lyricsSearchTextHide() //SLOT
{
    m_lyricsSearchText->setText("");
    m_lyricsSearchText->setEnabled( false );
    m_lyricsTextBar->hide();
    m_lyricsTextBarShowed=false;
}


void
ContextBrowser::lyricsSearchTextToggle() //SLOT
{
    if ( m_lyricsTextBarShowed )
    {
        lyricsSearchTextHide();
    }
    else
    {
        lyricsSearchTextShow();
    }
}

// Wikipedia-Tab
//////////////////////////////////////////////////////////////////////////////////////////

QString
ContextBrowser::wikiArtistPostfix() const
{
    if( wikiLocale() == "en" )
        return " (band)";
    else if( wikiLocale() == "de" )
        return " (Band)";
    else
        return "";
}

QString
ContextBrowser::wikiAlbumPostfix() const
{
    if( wikiLocale() == "en" )
        return " (album)";
    else
        return "";
}

QString
ContextBrowser::wikiTrackPostfix() const
{
    if( wikiLocale() == "en" )
        return " (song)";
    else
        return "";
}

void
ContextBrowser::wikiConfigChanged( int /*activeItem*/ ) // SLOT
{
    // keep in sync with localeList in wikiConfig
    QString text = m_wikiLocaleCombo->currentText();

    m_wikiLocaleEdit->setEnabled( text == i18n("Other...") );

    if( text == i18n("English") )
        m_wikiLocaleEdit->setText( "en" );

    else if( text == i18n("German") )
        m_wikiLocaleEdit->setText( "de" );

    else if( text == i18n("French") )
        m_wikiLocaleEdit->setText( "fr" );

    else if( text == i18n("Polish") )
        m_wikiLocaleEdit->setText( "pl" );

    else if( text == i18n("Japanese") )
        m_wikiLocaleEdit->setText( "ja" );

    else if( text == i18n("Spanish") )
        m_wikiLocaleEdit->setText( "es" );
}

void
ContextBrowser::wikiConfigApply() // SLOT
{
    const bool changed = m_wikiLocaleEdit->text() != wikiLocale();
    setWikiLocale( m_wikiLocaleEdit->text() );

    if ( changed && currentPage() == m_wikiTab && !m_wikiCurrentEntry.isNull() )
    {
        m_dirtyWikiPage = true;
        showWikipediaEntry( m_wikiCurrentEntry );
    }

    showWikipedia();
}

void
ContextBrowser::wikiConfig() // SLOT
{
    QStringList localeList;
    localeList
        << i18n( "English" )
        << i18n( "German" )
        << i18n( "French" )
        << i18n( "Polish" )
        << i18n( "Japanese" )
        << i18n( "Spanish" )
        << i18n( "Other..." );

    int index;

    if( wikiLocale() == "en" )
        index = 0;
    else if( wikiLocale() == "de" )
        index = 1;
    else if( wikiLocale() == "fr" )
        index = 2;
    else if( wikiLocale() == "pl" )
        index = 3;
    else if( wikiLocale() == "ja" )
        index = 4;
    else if( wikiLocale() == "es" )
        index = 5;
    else // other
        index = 6;

    m_wikiConfigDialog = new KDialogBase( this, 0, true, 0, KDialogBase::Ok|KDialogBase::Apply|KDialogBase::Cancel );
    kapp->setTopWidget( m_wikiConfigDialog );
    m_wikiConfigDialog->setCaption( kapp->makeStdCaption( i18n( "Wikipedia Locale" ) ) );
    QVBox *box = m_wikiConfigDialog->makeVBoxMainWidget();

    m_wikiLocaleCombo = new QComboBox( box );
    m_wikiLocaleCombo->insertStringList( localeList );

    QHBox  *hbox       = new QHBox( box );
    QLabel *otherLabel = new QLabel( i18n( "Locale: " ), hbox );
    m_wikiLocaleEdit   = new QLineEdit( "en", hbox );

    otherLabel->setBuddy( m_wikiLocaleEdit );
    QToolTip::add( m_wikiLocaleEdit, i18n( "2-letter language code for your Wikipedia locale" ) );

    connect( m_wikiLocaleCombo,  SIGNAL( activated(int) ), SLOT( wikiConfigChanged(int) ) );
    connect( m_wikiConfigDialog, SIGNAL( applyClicked() ), SLOT( wikiConfigApply() ) );

    m_wikiLocaleEdit->setText( wikiLocale() );
    m_wikiLocaleCombo->setCurrentItem( index );
    wikiConfigChanged( index ); // a little redundant, but saves ugly code, and ensures the lineedit enabled status is correct

    m_wikiConfigDialog->setInitialSize( QSize( 240, 100 ) );
    const int result = m_wikiConfigDialog->exec();


    if( result == QDialog::Accepted )
        wikiConfigApply();

    delete m_wikiConfigDialog;
}

QString
ContextBrowser::wikiLocale()
{
    if( s_wikiLocale.isEmpty() )
        return QString( "en" );

    return s_wikiLocale;
}

void
ContextBrowser::setWikiLocale( const QString &locale )
{
    AmarokConfig::setWikipediaLocale( locale );
    s_wikiLocale = locale;
}

QString
ContextBrowser::wikiURL( const QString &item )
{
    // add any special characters to be replaced here
    QString wStr = QString(item).replace( "/", " " );

    return QString( "http://%1.wikipedia.org/wiki/" ).arg( wikiLocale() )
        + KURL::encode_string_no_slash( wStr, 106 /*utf-8*/ );
}

void
ContextBrowser::reloadWikipedia()
{
    m_wikiJob = NULL;
    showWikipediaEntry( m_wikiCurrentEntry, true );
}

void
ContextBrowser::showWikipediaEntry( const QString &entry, bool replaceHistory )
{
    m_wikiCurrentEntry = entry;
    showWikipedia( wikiURL( entry ), false, replaceHistory );
}

void
ContextBrowser::showLabelsDialog()
{
    DEBUG_BLOCK
    KURL currentUrl = EngineController::instance()->bundle().url();
    QStringList allLabels = CollectionDB::instance()->labelList();
    QStringList trackLabels = CollectionDB::instance()->getLabels( currentUrl.path(), CollectionDB::typeUser );
    debug() << "Showing add label dialog" << endl;
    KDialogBase *dialog = new KDialogBase( this, 0, false, QString::null, KDialogBase::Ok|KDialogBase::Cancel );
    dialog->makeVBoxMainWidget();

    QLabel *labelText = new QLabel( i18n( 
		"<p>Add a new label in the field below and press Enter, or choose labels from the list</p>"),
	    			dialog->mainWidget() );
    m_addLabelEdit = new ClickLineEdit( i18n( "Add new label" ), dialog->mainWidget() );
    m_addLabelEdit->installEventFilter( this );
    m_addLabelEdit->setFrame( QFrame::Sunken );
    QToolTip::add( m_addLabelEdit, i18n( "Enter a new label and press Return to add it" ) );
    dialog->setFocusProxy( m_addLabelEdit );
    labelText->setBuddy( m_addLabelEdit );

    m_labelListView = new QListView( dialog->mainWidget() );
    m_labelListView->addColumn( i18n( "Label" ) );
    m_labelListView->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_labelListView->setColumnWidthMode( 0, QListView::Maximum );

    foreach( allLabels )
    {
        QCheckListItem *item = new QCheckListItem( m_labelListView, *it, QCheckListItem::CheckBox );
        item->setOn( trackLabels.contains( *it ) );
    }
    if( dialog->exec() == QDialog::Accepted )
    {
        debug() << "Dialog closed, updating labels" << endl;
        QStringList newTrackLabels;
        QListViewItemIterator iter( m_labelListView );
        while( iter.current() )
        {
            QCheckListItem *item = static_cast<QCheckListItem*>( iter.current() );
            if( item->isOn() )
                newTrackLabels.append( item->text() );
            iter++;
        }
        CollectionDB::instance()->setLabels( currentUrl.path(),
                                             newTrackLabels,
                                             CollectionDB::instance()->uniqueIdFromUrl( currentUrl ),
                                             CollectionDB::typeUser );
        CollectionDB::instance()->cleanLabels();
        if( newTrackLabels != trackLabels
                && currentUrl == EngineController::instance()->bundle().url() )
        {
            m_dirtyCurrentTrackPage = true;
            showCurrentTrack();
        }
    }
    delete dialog; //deletes children
    m_addLabelEdit = 0;
    m_labelListView = 0;
}

bool
ContextBrowser::eventFilter( QObject *o, QEvent *e )
{
    switch( e->type() )
    {
    case 6/*QEvent::KeyPress*/:
        #define e static_cast<QKeyEvent*>(e)

        if( o == m_addLabelEdit ) //the add label lineedit
        {
            switch( e->key() )
            {
            case Key_Return:
            case Key_Enter:
            {
                QCheckListItem *item = new QCheckListItem( m_labelListView, m_addLabelEdit->text(), QCheckListItem::CheckBox );
                item->setOn( true );
                m_addLabelEdit->setText( QString() );
                return true;
            }

            default:
                return false;
            }
        }
	if (o == m_lyricsSearchText)
	{
	   switch ( e->key() )
	   {
	   case Key_Escape:
	   {
	   	lyricsSearchTextHide();
		return true;
	   }
	   default:
	       return false;
	   }
       }

    default:
        break;
    }

    return KTabWidget::eventFilter( o, e );
}

void ContextBrowser::showWikipedia( const QString &url, bool fromHistory, bool replaceHistory )
{
    #if 0
    if( BrowserBar::instance()->currentBrowser() != this )
    {
        debug() << "current browser is not context, aborting showWikipedia()" << endl;
        m_dirtyWikiPage = true;
        return;
    }
    #endif

    if ( currentPage() != m_wikiTab )
    {
        blockSignals( true );
        showPage( m_wikiTab );
        blockSignals( false );
    }
    if ( !m_dirtyWikiPage || m_wikiJob ) return;

    // Disable the Open in a Browser button, because while loading it would open wikipedia main page.
    m_wikiToolBar->setItemEnabled( WIKI_BROWSER, false );

    m_HTMLSource="";
    m_HTMLSource.append(
            "<html><body>\n"
            "<div id='wiki_box' class='box'>\n"
                "<div id='wiki_box-header' class='box-header'>\n"
                    "<span id='wiki_box-header-title' class='box-header-title'>\n"
                    + i18n( "Wikipedia" ) +
                    "</span>\n"
                "</div>\n"
                "<div id='wiki_box-body' class='box-body'>\n"
                    "<div class='info'><p>\n" + i18n( "Fetching Wikipedia Information" ) + " ...</p></div>\n"
                "</div>\n"
            "</div>\n"
            "</body></html>\n"
                    );

    m_wikiPage->set( m_HTMLSource );
    saveHtmlData(); // Send html code to file

    if ( url.isEmpty() )
    {
        QString tmpWikiStr;

        if ( (EngineController::instance()->bundle().url().protocol() == "lastfm") ||
             (EngineController::instance()->bundle().url().protocol() == "daap") ||
             !EngineController::engine()->isStream() )
        {
            if ( !EngineController::instance()->bundle().artist().isEmpty() )
            {
                tmpWikiStr = EngineController::instance()->bundle().artist();
                tmpWikiStr += wikiArtistPostfix();
            }
            else if ( !EngineController::instance()->bundle().title().isEmpty() )
            {
                tmpWikiStr = EngineController::instance()->bundle().title();
            }
            else
            {
                tmpWikiStr = EngineController::instance()->bundle().prettyTitle();
            }
        }
        else
        {
            tmpWikiStr = EngineController::instance()->bundle().prettyTitle();
        }

        //Hack to make wiki searches work with magnatune preview tracks
        if (tmpWikiStr.contains( "PREVIEW: buy it at www.magnatune.com" ) >= 1 ) {
            tmpWikiStr = tmpWikiStr.remove(" (PREVIEW: buy it at www.magnatune.com)" );
            int index = tmpWikiStr.find( '-' );
            if ( index != -1 ) {
                tmpWikiStr = tmpWikiStr.left (index - 1);
            }

        }
        m_wikiCurrentEntry = tmpWikiStr;

        m_wikiCurrentUrl = wikiURL( tmpWikiStr );
    }
    else
    {
        m_wikiCurrentUrl = url;
    }

    // Append new URL to history
    if ( replaceHistory )
    {
        m_wikiBackHistory.back() = m_wikiCurrentUrl;
    }
    else if ( !fromHistory ) {
        m_wikiBackHistory += m_wikiCurrentUrl;
        m_wikiForwardHistory.clear();
    }
    // Limit number of items in history
    if ( m_wikiBackHistory.count() > WIKI_MAX_HISTORY )
        m_wikiBackHistory.pop_front();

    // Remove all items from the button-menus
    m_wikiBackPopup->clear();
    m_wikiForwardPopup->clear();

    // Populate button menus with URLs from the history
    QStringList::ConstIterator it;
    uint count;
    // Reverse iterate over both lists
    count = m_wikiBackHistory.count()-1;
    it = m_wikiBackHistory.fromLast();
    if( count > 0 )
        it--;
    for ( uint i=0; i<count; i++, --it )
        m_wikiBackPopup->insertItem( SmallIconSet( "wiki" ), *it, i );
    count = m_wikiForwardHistory.count();
    it = m_wikiForwardHistory.fromLast();
    for ( uint i=0; i<count; i++, --it )
        m_wikiForwardPopup->insertItem( SmallIconSet( "wiki" ), *it, i );

    m_wikiToolBar->setItemEnabled( WIKI_BACK, m_wikiBackHistory.size() > 1 );
    m_wikiToolBar->setItemEnabled( WIKI_FORWARD, m_wikiForwardHistory.size() > 0 );

    m_wikiBaseUrl = m_wikiCurrentUrl.mid(0 , m_wikiCurrentUrl.find("wiki/"));
    m_wikiJob = KIO::storedGet( m_wikiCurrentUrl, false, false );

    Amarok::StatusBar::instance()->newProgressOperation( m_wikiJob )
            .setDescription( i18n( "Fetching Wikipedia Information" ) );

    connect( m_wikiJob, SIGNAL( result( KIO::Job* ) ), SLOT( wikiResult( KIO::Job* ) ) );
}


void
ContextBrowser::wikiHistoryBack() //SLOT
{
    //Disable the button as history may be empty. Reenabled later by showWikipedia.
    m_wikiToolBar->setItemEnabled( WIKI_BACK, false );
    m_wikiToolBar->setItemEnabled( WIKI_FORWARD, false );

    m_wikiForwardHistory += m_wikiBackHistory.last();
    m_wikiBackHistory.pop_back();

    m_dirtyWikiPage = true;
    m_wikiCurrentEntry = QString::null;
    showWikipedia( m_wikiBackHistory.last(), true );
}


void
ContextBrowser::wikiHistoryForward() //SLOT
{
    //Disable the button as history may be empty. Reenabled later by showWikipedia.
    m_wikiToolBar->setItemEnabled( WIKI_FORWARD, false );
    m_wikiToolBar->setItemEnabled( WIKI_BACK, false );

    m_wikiBackHistory += m_wikiForwardHistory.last();
    m_wikiForwardHistory.pop_back();

    m_dirtyWikiPage = true;
    m_wikiCurrentEntry = QString::null;
    showWikipedia( m_wikiBackHistory.last(), true );
}


void
ContextBrowser::wikiBackPopupActivated( int id ) //SLOT
{
    do
    {
        m_wikiForwardHistory += m_wikiBackHistory.last();
        m_wikiBackHistory.pop_back();
        if ( m_wikiForwardHistory.count() > WIKI_MAX_HISTORY )
            m_wikiForwardHistory.pop_front();
        id--;
    } while( id >= 0 );

    m_dirtyWikiPage = true;
    m_wikiCurrentEntry = QString::null;
    showWikipedia( m_wikiBackHistory.last(), true );
}

void
ContextBrowser::wikiForwardPopupActivated( int id ) //SLOT
{
    do
    {
        m_wikiBackHistory += m_wikiForwardHistory.last();
        m_wikiForwardHistory.pop_back();
        if ( m_wikiBackHistory.count() > WIKI_MAX_HISTORY )
            m_wikiBackHistory.pop_front();

        id--;
    } while( id >= 0 );

    m_dirtyWikiPage = true;
    m_wikiCurrentEntry = QString::null;
    showWikipedia( m_wikiBackHistory.last(), true );
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
    showWikipediaEntry( EngineController::instance()->bundle().album() + wikiAlbumPostfix() );
}


void
ContextBrowser::wikiTitlePage() //SLOT
{
    m_dirtyWikiPage = true;
    showWikipediaEntry( EngineController::instance()->bundle().title() + wikiTrackPostfix() );
}


void
ContextBrowser::wikiExternalPage() //SLOT
{
    Amarok::invokeBrowser( m_wikiCurrentUrl );
}


void
ContextBrowser::wikiResult( KIO::Job* job ) //SLOT
{
    DEBUG_BLOCK

    if ( !job->error() == 0 )
    {
        m_HTMLSource="";
        m_HTMLSource.append(
            "<div id='wiki_box' class='box'>\n"
                "<div id='wiki_box-header' class='box-header'>\n"
                    "<span id='wiki_box-header-title' class='box-header-title'>\n"
                    + i18n( "Error" ) +
                    "</span>\n"
                "</div>\n"
                "<div id='wiki_box-body' class='box-body'><p>\n"
                    + i18n( "Artist information could not be retrieved because the server was not reachable." ) +
                "</p></div>\n"
            "</div>\n"
                        );
        m_wikiPage->set( m_HTMLSource );

        m_dirtyWikiPage = false;
        //m_wikiPage = NULL; // FIXME: what for? leads to crashes
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

    if( m_wiki.find( "var wgArticleId = 0" ) != -1 )
    {
        debug() << "Article not found." << endl;
        
        // article was not found
        if( !wikiArtistPostfix().isEmpty() && m_wikiCurrentEntry.endsWith( wikiArtistPostfix() ) )
        {
            m_wikiCurrentEntry = m_wikiCurrentEntry.left( m_wikiCurrentEntry.length() - wikiArtistPostfix().length() );
            reloadWikipedia();
            return;
        }
        else if( !wikiAlbumPostfix().isEmpty() && m_wikiCurrentEntry.endsWith( wikiAlbumPostfix() ) )
        {
            m_wikiCurrentEntry = m_wikiCurrentEntry.left( m_wikiCurrentEntry.length() - wikiAlbumPostfix().length() );
            reloadWikipedia();
            return;
        }
        else if( !wikiTrackPostfix().isEmpty() && m_wikiCurrentEntry.endsWith( wikiTrackPostfix() ) )
        {
            m_wikiCurrentEntry = m_wikiCurrentEntry.left( m_wikiCurrentEntry.length() - wikiTrackPostfix().length() );
            reloadWikipedia();
            return;
        }
    }

    //remove the new-lines and tabs(replace with spaces IS needed).
    m_wiki.replace( "\n", " " );
    m_wiki.replace( "\t", " " );

    m_wikiLanguages = QString::null;
    // Get the available language list
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

    m_wiki.replace( QRegExp( "<span class=\"editsection\"[^>]*>[^<]*<[^>]*>[^<]*<[^>]*>[^<]*</span>" ), QString::null );

    m_wiki.replace( QRegExp( "<a href=\"[^\"]*\" class=\"new\"[^>]*>([^<]*)</a>" ), "\\1" );

    // Remove anything inside of a class called urlexpansion, as it's pointless for us
    m_wiki.replace( QRegExp( "<span class= *'urlexpansion'>[^(]*[(][^)]*[)]</span>" ), QString::null );

    // Remove hidden table rows as well
    QRegExp hidden( "<tr *class= *[\"\']hiddenStructure[\"\']>.*</tr>", false );
    hidden.setMinimal( true ); //greedy behaviour wouldn't be any good!
    m_wiki.replace( hidden, QString::null );

    // we want to keep our own style (we need to modify the stylesheet a bit to handle things nicely)
    m_wiki.replace( QRegExp( "style= *\"[^\"]*\"" ), QString::null );
    m_wiki.replace( QRegExp( "class= *\"[^\"]*\"" ), QString::null );
    // let's remove the form elements, we don't want them.
    m_wiki.replace( QRegExp( "<input[^>]*>" ), QString::null );
    m_wiki.replace( QRegExp( "<select[^>]*>" ), QString::null );
    m_wiki.replace( "</select>\n" , QString::null );
    m_wiki.replace( QRegExp( "<option[^>]*>" ), QString::null );
    m_wiki.replace( "</option>\n" , QString::null );
    m_wiki.replace( QRegExp( "<textarea[^>]*>" ), QString::null );
    m_wiki.replace( "</textarea>" , QString::null );

    //first we convert all the links with protocol to external, as they should all be External Links.
    m_wiki.replace( QRegExp( "href= *\"http:" ), "href=\"externalurl:" );
    m_wiki.replace( QRegExp( "href= *\"/" ), "href=\"" +m_wikiBaseUrl );
    m_wiki.replace( QRegExp( "href= *\"#" ), "href=\"" +m_wikiCurrentUrl + '#' );

    m_HTMLSource = "<html><body>\n";
    m_HTMLSource.append(
            "<div id='wiki_box' class='box'>\n"
                "<div id='wiki_box-header' class='box-header'>\n"
                    "<span id='wiki_box-header-title' class='box-header-title'>\n"
                    + i18n( "Wikipedia Information" ) +
                    "</span>\n"
                "</div>\n"
                "<div id='wiki_box-body' class='box-body'>\n"
                    + m_wiki +
                "</div>\n"
            "</div>\n"
                       );
    if ( !m_wikiLanguages.isEmpty() )
    {
        m_HTMLSource.append(
                "<div id='wiki_box' class='box'>\n"
                    "<div id='wiki_box-header' class='box-header'>\n"
                        "<span id='wiki_box-header-title' class='box-header-title'>\n"
                        + i18n( "Wikipedia Other Languages" ) +
                        "</span>\n"
                    "</div>\n"
                    "<div id='wiki_box-body' class='box-body'>\n"
                        + m_wikiLanguages +
                    "</div>\n"
                "</div>\n"
        );
    }
    m_HTMLSource.append( "</body></html>\n" );
    m_wikiPage->set( m_HTMLSource );

    m_dirtyWikiPage = false;
    saveHtmlData(); // Send html code to file
    m_wikiJob = NULL;
}


void
ContextBrowser::coverFetched( const QString &artist, const QString &album ) //SLOT
{
    if ( currentPage() == m_contextTab &&
            !EngineController::engine()->loaded() &&
            !m_browseArtists )
    {
        m_dirtyCurrentTrackPage = true;
        if( m_shownAlbums.contains( album ) )
            showCurrentTrack();
        return;
    }

    const MetaBundle &currentTrack = EngineController::instance()->bundle();
    if ( currentTrack.artist().isEmpty() && currentTrack.album().isEmpty() )
        return;

    if ( currentPage() == m_contextTab &&
       ( currentTrack.artist().string() == artist || m_artist == artist || currentTrack.album().string() == album ) ) // this is for compilations or when artist is empty
    {
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
    }
}


void
ContextBrowser::coverRemoved( const QString &artist, const QString &album ) //SLOT
{
    if ( currentPage() == m_contextTab &&
            !EngineController::engine()->loaded() &&
            !m_browseArtists )
    {
        m_dirtyCurrentTrackPage = true;
        if( m_shownAlbums.contains( album ) )
            showCurrentTrack();
        return;
    }

    const MetaBundle &currentTrack = EngineController::instance()->bundle();
    if ( currentTrack.artist().isEmpty() && currentTrack.album().isEmpty() && m_artist.isNull() )
        return;

    if ( currentPage() == m_contextTab &&
       ( currentTrack.artist().string() == artist || m_artist == artist || currentTrack.album().string() == album ) ) // this is for compilations or when artist is empty
    {
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
    }
}


void
ContextBrowser::similarArtistsFetched( const QString &artist ) //SLOT
{
    if( artist == m_artist || EngineController::instance()->bundle().artist().string() == artist ) {
        m_dirtyCurrentTrackPage = true;
        if ( currentPage() == m_contextTab )
            showCurrentTrack();
    }
}

void
ContextBrowser::imageFetched( const QString &url ) //SLOT
{
    const MetaBundle &currentTrack = EngineController::instance()->bundle();
    PodcastEpisodeBundle peb;
    if( CollectionDB::instance()->getPodcastEpisodeBundle( currentTrack.url(), &peb ) )
    {
        PodcastChannelBundle pcb;
        if( CollectionDB::instance()->getPodcastChannelBundle( peb.parent(), &pcb ) )
        {
            if( pcb.imageURL().url() == url )
            {
                m_dirtyCurrentTrackPage = true;
                showCurrentTrack();
            }
        }
    }
}

void ContextBrowser::ratingOrScoreOrLabelsChanged( const QString &path ) //SLOT
{
    const MetaBundle &currentTrack = EngineController::instance()->bundle();

    //Always refresh if using ratings, otherwise suggested songs and other songs by artist that
    //have their ratings changed in the playlist won't be reflected until the context browser refreshes
    //which can be confusing, and looks less polished/professional
    //This can be changed if it slows things down too much...
    if( m_browseLabels || ( currentTrack.isFile() && ( currentTrack.url().path() == path || AmarokConfig::useRatings() ) ) )
        m_dirtyCurrentTrackPage = true; // will be reloaded when viewed (much faster)
     
    if( currentPage() == m_contextTab )
        refreshCurrentTrackPage();
}

void ContextBrowser::tagsChanged( const MetaBundle &bundle ) //SLOT
{
    const MetaBundle &currentTrack = EngineController::instance()->bundle();

    if( !m_shownAlbums.contains( bundle.album() ) && m_artist != bundle.artist() )
    {
        if( currentTrack.artist().isEmpty() && currentTrack.album().isEmpty() )
            return;

        if( bundle.artist() != currentTrack.artist() && bundle.album() != currentTrack.album() )
            return;
    }

    refreshCurrentTrackPage();
}

void ContextBrowser::tagsChanged( const QString &oldArtist, const QString &oldAlbum ) //SLOT
{
    const MetaBundle &currentTrack = EngineController::instance()->bundle();

    if( !m_shownAlbums.contains( oldAlbum ) && m_artist != oldArtist )
    {
        if( currentTrack.artist().isEmpty() && currentTrack.album().isEmpty() )
            return;

        if( oldArtist != currentTrack.artist() && oldAlbum != currentTrack.album() )
            return;
    }

    refreshCurrentTrackPage();
}


void ContextBrowser::refreshCurrentTrackPage() //SLOT
{
    if ( currentPage() == m_contextTab ) // this is for compilations or when artist is empty
    {
        m_dirtyCurrentTrackPage = true;
        showCurrentTrack();
    }
}


bool
ContextBrowser::hasContextProtocol( const KURL &url )
{
    QString protocol = url.protocol();
    return protocol == "album"
        || protocol == "artist"
        || protocol == "stream"
        || protocol == "compilation"
        || protocol == "albumdisc"
        || protocol == "compilationdisc"
        || protocol == "fetchcover";
}

KURL::List
ContextBrowser::expandURL( const KURL &url )
{
    KURL::List urls;
    QString protocol = url.protocol();

    if( protocol == "artist" ) {
        uint artist_id = CollectionDB::instance()->artistID( url.path(), false );
        if( artist_id )
        {
            QStringList trackUrls = CollectionDB::instance()->artistTracks( QString::number( artist_id ) );
            foreach( trackUrls )
                urls += KURL::fromPathOrURL( *it );
        }
    }
    else if( protocol == "album" ) {
        QString artist, album, track; // track unused here
        Amarok::albumArtistTrackFromUrl( url.path(), artist, album, track );

        QStringList trackUrls = CollectionDB::instance()->albumTracks( artist, album );
        foreach( trackUrls ) {
            urls += KURL::fromPathOrURL( *it );
        }
    }
    else if( protocol == "albumdisc" ) {
        QString artist, album, discnumber; // discnumber is returned in track number field
        Amarok::albumArtistTrackFromUrl( url.path(), artist, album, discnumber );

        QStringList trackUrls = CollectionDB::instance()->albumDiscTracks( artist, album, discnumber );
        foreach( trackUrls ) {
            urls += KURL::fromPathOrURL( *it );
        }
    }
    else if( protocol == "compilation" ) {
        QueryBuilder qb;
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, url.path() );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
        qb.setOptions( QueryBuilder::optOnlyCompilations );
        QStringList values = qb.run();

        for( QStringList::ConstIterator it = values.begin(), end = values.end(); it != end; ++it ) {
            urls += KURL::fromPathOrURL( *it );
        }
    }
    else if( protocol == "compilationdisc") {
        QString artist, album, discnumber; // artist is unused
        Amarok::albumArtistTrackFromUrl( url.path(), artist, album, discnumber );

        QueryBuilder qb;
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, album );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valDiscNumber, discnumber );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
        qb.setOptions( QueryBuilder::optOnlyCompilations );
        QStringList values = qb.run();

        for( QStringList::ConstIterator it = values.begin(), end = values.end(); it != end; ++it ) {
            urls += KURL::fromPathOrURL( *it );
        }
    }

    else if( protocol == "fetchcover" ) {
        QString artist, album, track; // track unused here
        Amarok::albumArtistTrackFromUrl( url.path(), artist, album, track );

        QString artistID = QString::number( CollectionDB::instance()->artistID( artist ) );
        QString albumID = QString::number( CollectionDB::instance()->albumID( album ) );

        QStringList trackUrls = CollectionDB::instance()->albumTracks( artistID, albumID );
        foreach( trackUrls ) {
            urls += KURL::fromPathOrURL( *it );
        }
    }

    else if( protocol == "stream" ) {
        urls += KURL::fromPathOrURL( url.url().replace( QRegExp( "^stream:" ), "http:" ) );
    }

    return urls;
}


#include "contextbrowser.moc"
