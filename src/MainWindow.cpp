/***************************************************************************
  begin                : Fre Nov 15 2002
  copyright            : (C) Mark Kretschmann <markey@web.de>
                       : (C) Max Howell <max.howell@methylblue.com>
                       : (C) G??bor Lehel <illissius@gmail.com>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "config-amarok.h"           //HAVE_LIBVISUAL definition

#include "actionclasses.h"    //see toolbar construction
#include "amarokconfig.h"
#include "amarok.h"
//#include "AmarokStatusBar.h"
#include "collection/CollectionManager.h"
#include "collectionbrowser/CollectionWidget.h"
#include "context/CoverBling.h"
#include "context/ContextView.h"
#include "context/DataEngineManager.h"
#include "CoverManager.h" // for actions
#include "debug.h"
#include "editfilterdialog.h"
#include "enginecontroller.h" //for actions in ctor
#include "filebrowser/FileBrowser.h"
#include "k3bexporter.h"
#include "MainWindow.h"
#include "mediabrowser.h"
#include "playlist/PlaylistModel.h"
#include "playlist/PlaylistWidget.h"
#include "scriptmanager.h"
#include "searchwidget.h"
#include "servicebrowser/ServicePluginManager.h"
#include "servicebrowser/scriptableservice/scriptableservice.h"
#include "servicebrowser/servicebrowser.h"
#include "servicebrowser/shoutcast/ShoutcastService.h"
#include "servicebrowser/mp3tunes/mp3tunesservice.h"
#include "Sidebar.h"
#include "Sidebar.moc"
#include "socketserver.h"
#include "Statistics.h"
#include "ContextStatusBar.h"
#include "TheInstances.h"
#include "PodcastCollection.h"
#include "playlistmanager/PlaylistManager.h"
#include "playlistmanager/PlaylistFileProvider.h"
#include "playlistbrowser/PlaylistBrowser.h"

#include "queuemanager/QueueManager.h"

#include <QFont>
#include <QHeaderView>
#include <QLabel>           //search filter label
#include <QList>
#include <QPaintEngine>
#include <QPainter>         //dynamic title
#include <QPen>
#include <QTimer>           //search filter timer
#include <QToolTip>         //QToolTip::add()
#include <QVBoxLayout>
#include <QPixmapCache>

#include <KAction>          //m_actionCollection
#include <KActionCollection>
#include <KActionMenu>
#include <KApplication>     //kapp
#include <KFileDialog>      //savePlaylist(), openPlaylist()
#include <KGlobal>
#include <KInputDialog>     //slotAddStream()
#include <KLocale>
#include <KMenuBar>
#include <KMessageBox>      //savePlaylist()
#include <KMenu>
#include <KPushButton>
#include <kdeversion.h>

#ifdef Q_WS_X11
#include <fixx11h.h> 
#endif

class ContextWidget : public KVBox
{
    // Set a useful size default of the center tab.
    public:
        ContextWidget( QWidget *parent ) : KVBox( parent ) {}
        QSize sizeHint() const { return QSize( static_cast<QWidget*>(parent())->size().width() / 3, 300 ); }
};

MainWindow *MainWindow::s_instance = 0;

MainWindow::MainWindow()
        :KXmlGuiWindow( 0, Qt::WGroupLeader )
        , m_lastBrowser( 0 )
{
    setObjectName("MainWindow");
    s_instance = this;

    PERF_LOG( "Creating PlaylistModel" )
    new Playlist::Model( this );
    PERF_LOG( "Created PlaylistModel" )

    // Sets caption and icon correctly (needed e.g. for GNOME)
    kapp->setTopWidget( this );
    PERF_LOG( "Set Top Widget" )
    createActions();
    PERF_LOG( "Created actions" )

    //new K3bExporter();

    if( AmarokConfig::mainWindowSize().isValid() )
    {
        // if first ever run, use sizeHint(), and let
        // KWindowSystem place us otherwise use the stored values
        resize( AmarokConfig::mainWindowSize() );
        move( AmarokConfig::mainWindowPos() );
    }
    PERF_LOG( "Create sidebar" )
    m_browsers = new SideBar( this, new KVBox );
    m_browsers->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );
}

MainWindow::~MainWindow()
{
    DEBUG_BLOCK
    AmarokConfig::setMainWindowPos( pos() );  //TODO de XT?
    AmarokConfig::setMainWindowSize( size() ); //TODO de XT?
    delete m_playlistFiles;
}


///////// public interface

/**
 * This function will initialize the playlist window.
 */
void MainWindow::init()
{
    layout()->setContentsMargins( 0, 0, 0, 0 );
    DEBUG_BLOCK

    //this function is necessary because Amarok::actionCollection() returns our actionCollection
    //via the App::m_pMainWindow pointer since App::m_pMainWindow is not defined until
    //the above ctor returns it causes a crash unless we do the initialisation in 2 stages.
    PERF_LOG( "Create Playlist" )
    Playlist::Widget *playlistWidget = new Playlist::Widget( this );
    playlistWidget->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );
    PERF_LOG( "Playlist created" )

    {
        m_controlBar = new MainToolbar( this );

    }

    QPalette p;
    QColor bottomColor;
    QColor topColor = bottomColor = palette().highlight().color().dark( 150 );
    bottomColor = bottomColor.dark( 100 );
    topColor.setAlpha( 75 );
    bottomColor.setAlpha( 130 );

    createMenus();

    PERF_LOG( "Creating ContextWidget" )
    ContextWidget *contextWidget = new ContextWidget( this );
    PERF_LOG( "ContextWidget created" )
    contextWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Maximum );
    PERF_LOG( "Creating ContextView" )
    new Context::ContextView( contextWidget );
    PERF_LOG( "ContextView created" )
    {

        if( AmarokConfig::useCoverBling() && QGLFormat::hasOpenGL() )
            new CoverBling( contextWidget );
    }

    connect( m_browsers, SIGNAL( widgetActivated( int ) ), SLOT( slotShrinkBrowsers( int ) ) );

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins( 1, 1, 1, 1 );
    mainLayout->setSpacing( 1 );

    QWidget *centralWidget = new QWidget( this );
    centralWidget->setLayout( mainLayout );

    m_splitter = new QSplitter( Qt::Horizontal, centralWidget );
    m_splitter->setHandleWidth( 0 );
    m_splitter->addWidget( m_browsers );
    m_splitter->addWidget( contextWidget );
    m_splitter->addWidget( playlistWidget );


    //make room for a full width statusbar at the bottom of everything

    KHBox * m_statusbarArea = new KHBox( this );

    //figure out the needed heigh tbased on system font settings
    // do make sure that it is at least 26 pixels tall though
    //or progress bars will not fit...

    QFont currentFont = font();
    currentFont.setBold( true );
    QFontMetrics fm( currentFont );
    int fontHeight = qMax( 26, fm.height() );
    
    m_statusbarArea->setMinimumHeight( fontHeight );
    m_statusbarArea->setMaximumHeight( fontHeight );
    new Amarok::ContextStatusBar( m_statusbarArea );

    mainLayout->addWidget( m_controlBar );
    mainLayout->addWidget( m_splitter );
    mainLayout->addWidget( m_statusbarArea);



    
    setCentralWidget( centralWidget );

    //<Browsers>
    {
        Debug::Block block( "Creating browsers. Please report long start times!" );

        #define addBrowserMacro( Type, name, text, icon ) { \
            Debug::Block block( name ); \
             m_browsers->addWidget( KIcon( icon ), text, new Type( name ) ); \
             m_browserNames.append( name ); }

        #define addInstBrowserMacro( Type, name, text, icon ) { \
             m_browsers->addWidget( KIcon( icon ), text, Type::instance() ); \
             m_browserNames.append( name ); }

        PERF_LOG( "Creating CollectionWidget" )
        addBrowserMacro( CollectionWidget, "CollectionBrowser", i18n("Collection"), "collection-amarok" )
        PERF_LOG( "Created CollectionWidget" )

        //cant use macros here since we need access to the browsers directly
        PERF_LOG( "Creating ServiceBrowser" )
        ServiceBrowser * internetContentServiceBrowser = ServiceBrowser::instance();
        internetContentServiceBrowser->setParent( this );
        m_browsers->addWidget( KIcon( "services-amarok" ), i18n("Internet"), internetContentServiceBrowser );
        m_browserNames.append( "Internet" );
        PERF_LOG( "Created ServiceBrowser" )

        PERF_LOG( "Do podcast stuff" )
        //TODO: find a better place to load the default collections and providers
        PodcastCollection *localPodcasts = new PodcastCollection();
        The::playlistManager()->addProvider( localPodcasts->channelProvider(), PlaylistManager::PodcastChannel );
        CollectionManager::instance()->addTrackProvider( localPodcasts );
        PERF_LOG( "Podcast stuff done" )

        m_playlistFiles = new PlaylistFileProvider();
        The::playlistManager()->addProvider( m_playlistFiles, PlaylistManager::UserPlaylist );

        PERF_LOG( "Creating PlaylistBrowser" )
        addBrowserMacro( PlaylistBrowserNS::PlaylistBrowser, "PlaylistBrowser", i18n("Playlists"), "view-media-playlist-amarok" )
        PERF_LOG( "CreatedPlaylsitBrowser" )

        PERF_LOG( "Creating FileBrowser" )
        addBrowserMacro( FileBrowser::Widget, "FileBrowser::Widget",  i18n("Files"), "folder-amarok" )
        PERF_LOG( "Created FileBrowser" )


        //get the plugin manager
        ServicePluginManager::instance()->setBrowser( internetContentServiceBrowser );
        PERF_LOG( "Initialising ServicePluginManager" )
        ServicePluginManager::instance()->init();
        PERF_LOG( "Initialised ServicePluginManager" )

        internetContentServiceBrowser->setScriptableServiceManager( new ScriptableServiceManager( 0 ) );
        PERF_LOG( "ScriptableServiceManager done" )
        new MediaBrowser( "MediaBrowser" );
        PERF_LOG( "created mediabrowser" )
        if( MediaBrowser::isAvailable() )
        {
            addInstBrowserMacro( MediaBrowser, "MediaBrowser", i18n("Devices"), "multimedia-player-amarok" )
        }
        #undef addBrowserMacro
        #undef addInstBrowserMacro
        PERF_LOG( "finished MainWindow::init" )
    }
    //</Browsers>


    kapp->installEventFilter( this ); // keyboards shortcuts for the browsers

    Amarok::MessageQueue::instance()->sendMessages();
}

void MainWindow::deleteBrowsers()
{
    m_browsers->deleteBrowsers();
}

void MainWindow::slotSetFilter( const QString &filter ) //SLOT
{
    Q_UNUSED( filter );
//    m_lineEdit->setText( filter );
}

void MainWindow::slotShrinkBrowsers( int index ) const
{
    // Because QSplitter sucks and will not recompute sizes if a pane is shrunk and not hidden.
    if( index == -1 )
    {
        QList<int> sizes;
        sizes << m_browsers->sideBarWidget()->width()
            << m_splitter->sizes()[1] + m_splitter->sizes()[0] - m_browsers->sideBarWidget()->width()
            << m_splitter->sizes()[2];
        m_splitter->setSizes( sizes );
    }
}

// void MainWindow::slotEditFilter() //SLOT
// {
//     EditFilterDialog *fd = new EditFilterDialog( this, true, "" );
//     connect( fd, SIGNAL(filterChanged(const QString &)), SLOT(slotSetFilter(const QString &)) );
//     if( fd->exec() )
//         m_searchWidget->lineEdit()->setText( fd->filter() );
//     delete fd;
// }

void MainWindow::addBrowser( const QString &name, QWidget *browser, const QString &text, const QString &icon )
{
    if( !m_browserNames.contains( name ) )
    {
        m_browsers->addWidget( KIcon( icon ), text, browser );
        m_browserNames.append( name );
    }
}

void MainWindow::showBrowser( const QString &name )
{
    const int index = m_browserNames.indexOf( name );
    if( index >= 0 && index != m_browsers->currentIndex() )
        m_browsers->showWidget( index );
}


/**
 * @param o The object
 * @param e The event
 *
 * Here we filter some events for the Playlist Search LineEdit and the Playlist. @n
 * this makes life easier since we have more useful functions available from this class
 */
bool MainWindow::eventFilter( QObject *o, QEvent *e )
{
    typedef Q3ListViewItemIterator It;

    switch( e->type() )
    {
    case 6/*QEvent::KeyPress*/:

        //there are a few keypresses that we intercept

        #define e static_cast<QKeyEvent*>(e)

        //TODO:PORT ME
//         if( e->key() == Qt::Key_F2 )
//         {
//             // currentItem is ALWAYS visible.
//             Q3ListViewItem *item = pl->currentItem();
//
//             // intercept F2 for inline tag renaming
//             // NOTE: tab will move to the next tag
//             // NOTE: if item is still null don't select first item in playlist, user wouldn't want that. It's silly.
//             // TODO: berkus has solved the "inability to cancel" issue with K3ListView, but it's not in kdelibs yet..
//
//             // item may still be null, but this is safe
//             // NOTE: column 0 cannot be edited currently, hence we pick column 1
//             pl->rename( item, 1 ); //TODO what if this column is hidden?
//
//             return true;
//         }


//         if( o == m_searchWidget->lineEdit() ) //the search lineedit
//         {
//             Q3ListViewItem *item;
//             switch( e->key() )
//             {
//             case Qt::Key_Up:
//             case Qt::Key_Down:
//             case Qt::Key_PageDown:
//             case Qt::Key_PageUp:
//                 pl->setFocus();
//                 QApplication::sendEvent( pl, e );
//                 return true;
//
//             case Qt::Key_Return:
//             case Qt::Key_Enter:
//                 item = *It( pl, It::Visible );
//                 //m_lineEdit->clear();
//                 pl->m_filtertimer->stop(); //HACK HACK HACK
//
//                 if( e->modifiers() & Qt::ControlModifier )
//                 {
//                     QList<PlaylistItem*> in, out;
//                     if( e->modifiers() & Qt::ShiftModifier )
//                         for( It it( pl, It::Visible ); PlaylistItem *x = static_cast<PlaylistItem*>( *it ); ++it )
//                         {
//                             pl->queue( x, true );
//                             ( pl->m_nextTracks.contains( x ) ? in : out ).append( x );
//                         }
//                     else
//                     {
//                         It it( pl, It::Visible );
//                         pl->activate( *it );
//                         ++it;
//                         for( int i = 0; PlaylistItem *x = static_cast<PlaylistItem*>( *it ); ++i, ++it )
//                         {
//                             in.append( x );
//                             pl->m_nextTracks.insert( i, x );
//                         }
//                     }
//                     if( !in.isEmpty() || !out.isEmpty() )
//                         emit pl->queueChanged( in, out );
//                     pl->setFilter( "" );
//                     pl->ensureItemCentered( ( e->modifiers() & Qt::ShiftModifier ) ? item : pl->currentTrack() );
//                 }
//                 else
//                 {
//                     pl->setFilter( "" );
//                     if( ( e->modifiers() & Qt::ShiftModifier ) && item )
//                     {
//                         pl->queue( item );
//                         pl->ensureItemCentered( item );
//                     }
//                     else
//                     {
//                         pl->activate( item );
//                         pl->showCurrentTrack();
//                     }
//                 }
//                 return true;
//
//             case Qt::Key_Escape:
//                 m_searchWidget->lineEdit()->clear();
//                 return true;
//
//             default:
//                 return false;
//             }
//         }

        //following are for Playlist::instance() only
        //we don't handle these in the playlist because often we manipulate the lineEdit too

        #undef e
        break;

    default:
        break;
    }

    return QWidget::eventFilter( o, e );
}


void MainWindow::closeEvent( QCloseEvent *e )
{
#ifdef Q_WS_MAC
    Q_UNUSED( e );
    hide();
#else

    //KDE policy states we should hide to tray and not quit() when the
    //close window button is pushed for the main widget

    //e->accept(); //if we don't do this the info box appears on quit()!

    if( AmarokConfig::showTrayIcon() && e->spontaneous() && !kapp->sessionSaving() )
    {
        KMessageBox::information( this,
                i18n( "<qt>Closing the main-window will keep Amarok running in the System Tray. "
                      "Use <B>Quit</B> from the menu, or the Amarok tray-icon to exit the application.</qt>" ),
                i18n( "Docking in System Tray" ), "hideOnCloseInfo" );
        hide();
        e->ignore();
    }
    else
    {
        e->accept();
        kapp->quit();
    }
#endif
}


void MainWindow::showEvent( QShowEvent* )
{
    //PORT 2.0
//     static bool firstTime = true;
//     if( firstTime )
//         Playlist::instance()->setFocus();
//     firstTime = false;
}

#include <qdesktopwidget.h>
QSize MainWindow::sizeHint() const
{
    return QApplication::desktop()->screenGeometry( (QWidget*)this ).size() / 1.5;
}


void MainWindow::savePlaylist() const //SLOT
{
    QString playlistName = KFileDialog::getSaveFileName();
    if( !playlistName.isEmpty() )
        The::playlistModel()->saveM3U( playlistName );
}


void MainWindow::slotBurnPlaylist() const //SLOT
{
    K3bExporter::instance()->exportCurrentPlaylist();
}

void MainWindow::slotShowCoverManager() const //SLOT
{
    CoverManager::showOnce();
}

void MainWindow::slotPlayMedia() //SLOT
{
    // Request location and immediately start playback
    slotAddLocation( true );
}


void MainWindow::slotAddLocation( bool directPlay ) //SLOT
{
    // open a file selector to add media to the playlist
    KUrl::List files;
    KFileDialog dlg( KUrl(QString()), QString("*.*|"), this );
    dlg.setCaption( directPlay ? i18n("Play Media (Files or URLs)") : i18n("Add Media (Files or URLs)") );
    dlg.setMode( KFile::Files | KFile::Directory );
    dlg.exec();
    files = dlg.selectedUrls();
    if( files.isEmpty() ) return;
    const int options = directPlay ? Playlist::Append | Playlist::DirectPlay : Playlist::Append;

    Meta::TrackList tracks = CollectionManager::instance()->tracksForUrls( files );

    The::playlistModel()->insertOptioned( tracks.takeFirst(), options );

    foreach( Meta::TrackPtr track, tracks )
    {
        The::playlistModel()->insertOptioned( track, Playlist::Append );
    }
}

void MainWindow::slotAddStream() //SLOT
{
    bool ok;
    QString url = KInputDialog::getText( i18n("Add Stream"), i18n("Enter Stream URL:"), QString(), &ok, this );

    if ( !ok ) return;

    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl( url ) );

    The::playlistModel()->insertOptioned( track, Playlist::Append|Playlist::DirectPlay );
}

// TODO: need to add these menu items via last.fm service
#if 0
void MainWindow::playLastfmPersonal() //SLOT
{
    if( !LastFm::Controller::checkCredentials() ) return;

    const KUrl url( QString( "lastfm://user/%1/personal" )
                    .arg( AmarokConfig::scrobblerUsername() ) );

    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );

    The::playlistModel()->insertOptioned( track, Playlist::Append|Playlist::DirectPlay );
}


void MainWindow::addLastfmPersonal() //SLOT
{
    if( !LastFm::Controller::checkCredentials() ) return;

    const KUrl url( QString( "lastfm://user/%1/personal" )
                    .arg( AmarokConfig::scrobblerUsername() ) );

    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );

    The::playlistModel()->insertOptioned( track, Playlist::Append );
}


void MainWindow::playLastfmNeighbor() //SLOT
{
    if( !LastFm::Controller::checkCredentials() ) return;

    const KUrl url( QString( "lastfm://user/%1/neighbours" )
                    .arg( AmarokConfig::scrobblerUsername() ) );

    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );

    The::playlistModel()->insertOptioned( track, Playlist::Append );
}


void MainWindow::addLastfmNeighbor() //SLOT
{
    if( !LastFm::Controller::checkCredentials() ) return;

    const KUrl url( QString( "lastfm://user/%1/neighbours" )
                    .arg( AmarokConfig::scrobblerUsername() ) );

    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );

    The::playlistModel()->insertOptioned( track, Playlist::Append );
}


void MainWindow::playLastfmCustom() //SLOT
{
    const QString token = LastFm::Controller::createCustomStation();
    if( token.isEmpty() ) return;

    const KUrl url( "lastfm://artist/" + token + "/similarartists" );
    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );

    The::playlistModel()->insertOptioned( track, Playlist::Append|Playlist::DirectPlay );
}


void MainWindow::addLastfmCustom() //SLOT
{
    const QString token = LastFm::Controller::createCustomStation();
    if( token.isEmpty() ) return;

    const KUrl url( "lastfm://artist/" + token + "/similarartists" );
    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );

    The::playlistModel()->insertOptioned( track, Playlist::Append );
}


void MainWindow::playLastfmGlobaltag() //SLOT
{
    if( !LastFm::Controller::checkCredentials() ) return;

    KAction *action = dynamic_cast<KAction*>( sender() );
    if( !action ) return;

    const QString tag = action->text();
    const KUrl url( "lastfm://globaltags/" + tag );
    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );

    The::playlistModel()->insertOptioned( track, Playlist::Append|Playlist::DirectPlay );
}


void MainWindow::addLastfmGlobaltag() //SLOT
{
    if( !LastFm::Controller::checkCredentials() ) return;

    KAction *action = dynamic_cast<KAction*>( sender() );
    if( !action ) return;

    const QString tag = action->text();
    const KUrl url( "lastfm://globaltags/" + tag );
    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );

    The::playlistModel()->insertOptioned( track, Playlist::Append );
}
#endif

void MainWindow::playAudioCD() //SLOT
{
    KUrl::List urls;
    if( The::engineController()->getAudioCDContents(QString(), urls) )
    {
        Meta::TrackList tracks = CollectionManager::instance()->tracksForUrls( urls );
        if( !tracks.isEmpty() )
            The::playlistModel()->insertOptioned( tracks, Playlist::Replace );
    }
    else
    { // Default behaviour
        showBrowser( "FileBrowser" );
    }
}

void MainWindow::showScriptSelector() //SLOT
{
    ScriptManager::instance()->show();
    ScriptManager::instance()->raise();
}

void MainWindow::showQueueManager() //SLOT
{
    if( QueueManagerNS::QueueManager::instance() )
    {
        QueueManagerNS::QueueManager::instance()->raise();
        return;
    }

    QueueManagerNS::QueueManager dialog;
    if( dialog.exec() == QDialog::Accepted )
    {
        // TODO: alter queue
    }
}

void MainWindow::showStatistics() //SLOT
{
#if 0
    if( Statistics::instance() ) {
        Statistics::instance()->raise();
        return;
    }
    Statistics dialog;
    dialog.exec();
#endif
}

void MainWindow::slotToggleFocus() //SLOT
{
    //Port 2.0
//     if( m_browsers->currentWidget() && ( Playlist::instance()->hasFocus() /*|| m_searchWidget->lineEdit()->hasFocus()*/ ) )
//         m_browsers->currentWidget()->setFocus();
}

void MainWindow::slotToggleToolbar() //SLOT
{
    m_controlBar->setVisible( !m_controlBar->isHidden() );
    AmarokConfig::setShowToolbar( !AmarokConfig::showToolbar() );
    Amarok::actionCollection()->action( "toggle_toolbar" )->setText( !m_controlBar->isHidden() ? i18n("Hide Toolbar") : i18n("Show Toolbar") );
}

void MainWindow::toolsMenuAboutToShow() //SLOT
{
    Amarok::actionCollection()->action( "equalizer" )->setEnabled( false ); //TODO phonon
}


#include <kwindowsystem.h>
/**
 * Show/hide playlist global shortcut and PlayerWindow PlaylistButton connect to this slot
 * RULES:
 * 1. hidden & iconified -> deiconify & show @n
 * 2. hidden & deiconified -> show @n
 * 3. shown & iconified -> deiconify @n
 * 4. shown & deiconified -> hide @n
 * 5. don't hide if there is no tray icon or playerWindow. todo (I can't be arsed) @n
 *
 * @note isMinimized() can only be true if the window isVisible()
 * this has taken me hours to get right, change at your peril!
 * there are more contingencies than you can believe
 */
void MainWindow::showHide() //SLOT
{
#ifdef Q_WS_X11
    const KWindowInfo info = KWindowSystem::windowInfo( winId(), 0, 0 );
    const uint desktop = KWindowSystem::currentDesktop();
    const bool isOnThisDesktop = info.isOnDesktop( desktop );
    const bool isShaded = false;

    if( isShaded )
    {
        KWindowSystem::clearState( winId(), NET::Shaded );
        setVisible( true );
    }

    if( !isOnThisDesktop )
    {
        KWindowSystem::setOnDesktop( winId(), desktop );
        setVisible( true );
    }
    else if( !info.isMinimized() && !isShaded ) setVisible( !isVisible() );

    if( isVisible() ) KWindowSystem::unminimizeWindow( winId() );
#else
    setVisible( !isVisible() );
#endif
}

void MainWindow::activate()
{
#ifdef Q_WS_X11
    const KWindowInfo info = KWindowSystem::windowInfo( winId(), 0, 0 );

    if( KWindowSystem::activeWindow() != winId())
        setVisible( true );
    else if( !info.isMinimized() )
        setVisible( true );
    if( !isHidden() )
        KWindowSystem::activateWindow( winId() );
#else
    setVisible( true );
#endif
}

bool MainWindow::isReallyShown() const
{
#ifdef Q_WS_X11
    const KWindowInfo info = KWindowSystem::windowInfo( winId(), 0, 0 );
    return !isHidden() && !info.isMinimized() && info.isOnDesktop( KWindowSystem::currentDesktop() );
#else
    return isHidden();
#endif
}

void MainWindow::createActions()
{
    KActionCollection* const ac = actionCollection();
    const EngineController* const ec = EngineController::instance();

    KStandardAction::keyBindings( kapp, SLOT( slotConfigShortcuts() ), ac );
    KStandardAction::preferences( kapp, SLOT( slotConfigAmarok() ), ac );
    ac->action(KStandardAction::name(KStandardAction::KeyBindings))->setIcon( KIcon( "configure-shortcuts-amarok" ) );
    ac->action(KStandardAction::name(KStandardAction::Preferences))->setIcon( KIcon( "configure-amarok" ) );
    ac->action(KStandardAction::name(KStandardAction::Preferences))->setMenuRole(QAction::PreferencesRole); // Define OS X Prefs menu here, removes need for ifdef later

    KStandardAction::quit( kapp, SLOT( quit() ), ac );

    KAction *action = new KAction( KIcon( "folder-amarok" ), i18n("&Add Media..."), this );
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( slotAddLocation() ) );
    ac->addAction( "playlist_add", action );

    action = new KAction( KIcon( "edit-clear-list-amarok" ), i18nc( "clear playlist", "&Clear" ), this );
    connect( action, SIGNAL( triggered( bool ) ), The::playlistModel(), SLOT( clear() ) );
    ac->addAction( "playlist_clear", action );

    action = new KAction( KIcon( "folder-amarok" ), i18n("&Add Stream..."), this );
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( slotAddStream() ) );
    ac->addAction( "stream_add", action );

    action = new KAction( KIcon( "document-save-amarok" ), i18n("&Save Playlist As..."), this );
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( savePlaylist() ) );
    ac->addAction( "playlist_save", action );

    KAction *burn = new KAction( KIcon( "tools-media-optical-burn-amarok" ), i18n( "Burn Current Playlist" ), this );
    connect( burn, SIGNAL( triggered(bool) ), SLOT( slotBurnPlaylist() ) );
    burn->setEnabled( K3bExporter::isAvailable() );
    ac->addAction( "playlist_burn", burn );

    KAction *covermanager = new KAction( KIcon( "media-album-cover-manager-amarok" ), i18n( "Cover Manager" ), this );
    connect( covermanager, SIGNAL( triggered(bool) ), SLOT( slotShowCoverManager() ) );
    ac->addAction( "cover_manager", covermanager );

    KAction *visuals = new KAction( KIcon( "view-media-visualization-amarok" ), i18n("&Visualizations"), this );
    // connect( visuals, SIGNAL( triggered(bool) ), Vis::Selector::instance(), SLOT( show() ) );
    ac->addAction( "visualizations", visuals );

    KAction *equalizer = new KAction( KIcon( "view-media-equalizer-amarok" ), i18n( "E&qualizer"), this );
    connect( equalizer, SIGNAL( triggered(bool) ), kapp, SLOT( slotConfigEqualizer() ) );
    ac->addAction( "equalizer", equalizer );

    KAction *toggleToolbar = new KAction( this );
    toggleToolbar->setText( i18n("Hide Toolbar") );

    //FIXME m_controlBar is initialised after the actions are created so we need to change the text of this action
    //when the menu is shown
    //toggleToolbar->setText( !m_controlBar->isHidden() ? i18n("Hide Toolbar") : i18n("Show Toolbar") );
    connect( toggleToolbar, SIGNAL( triggered(bool) ), SLOT( slotToggleToolbar() ) );
    ac->addAction( "toggle_toolbar", toggleToolbar );

//     KAction *update_podcasts = new KAction( this );
//     update_podcasts->setText( i18n( "Update Podcasts" ) );
//     //update_podcasts->setIcon( KIcon("view-refresh-amarok") );
//     ac->addAction( "podcasts_update", update_podcasts );
//     connect(update_podcasts, SIGNAL(triggered(bool)),
//             The::podcastCollection(), SLOT(slotUpdateAll()));

    KAction *playact = new KAction( KIcon("folder-amarok"), i18n("Play Media..."), this );
    connect(playact, SIGNAL(triggered(bool)), SLOT(slotPlayMedia()));
    ac->addAction( "playlist_playmedia", playact );

    KAction *acd = new KAction( KIcon( "media-optical-audio-amarok" ), i18n("Play Audio CD"), this );
    connect(acd, SIGNAL(triggered(bool)), SLOT(playAudioCD()));
    ac->addAction( "play_audiocd", acd );

    KAction *script = new KAction( KIcon("preferences-plugin-script-amarok"), i18n("Script Manager"), this );
    connect(script, SIGNAL(triggered(bool)), SLOT(showScriptSelector()));
    ac->addAction( "script_manager", script );

    KAction *queue = new KAction( KIcon( "go-bottom-amarok"), i18n( "Queue Manager" ), this );
    connect(queue, SIGNAL(triggered(bool)), SLOT(showQueueManager()));
    ac->addAction( "queue_manager", queue );

    KAction *seekForward = new KAction( KIcon( "media-seek-forward-amarok" ), i18n("&Seek Forward"), this );
    seekForward->setShortcut( Qt::Key_Right );
    connect(seekForward, SIGNAL(triggered(bool)), ec, SLOT(seekForward()));
    ac->addAction( "seek_forward", seekForward );

    KAction *seekBackward = new KAction( KIcon( "media-seek-backward-amarok" ), i18n("&Seek Backward"), this );
    seekForward->setShortcut( Qt::Key_Left );
    connect(seekForward, SIGNAL(triggered(bool)), ec, SLOT(seekBackward()));
    ac->addAction( "seek_backward", seekBackward );

    KAction *statistics = new KAction( KIcon("view-statistics-amarok"), i18n( "Statistics" ), this );
    connect(statistics, SIGNAL(triggered(bool)), SLOT(showStatistics()));
    ac->addAction( "statistics", statistics );
    PERF_LOG( "MainWindow::createActions 6" )
    KAction *update = new KAction( KIcon("view-refresh-amarok"), i18n( "Update Collection" ), this );
    connect(update, SIGNAL(triggered(bool)), CollectionManager::instance(), SLOT(checkCollectionChanges()));
    ac->addAction( "update_collection", update );
    PERF_LOG( "MainWindow::createActions 7" )
    KAction *rescan = new KAction( KIcon("view-refresh-amarok"), i18n( "Rescan Collection" ), this );
    connect(rescan, SIGNAL(triggered(bool)), CollectionManager::instance(), SLOT(startFullScan()));
    ac->addAction( "rescan_collection", rescan );

    // TODO: Add these via last.fm service
#if 0
    m_lastfmTags << "Alternative" <<  "Ambient" << "Chill Out" << "Classical" << "Dance"
                 << "Electronica" << "Favorites" << "Heavy Metal" << "Hip Hop" << "Indie Rock"
                 << "Industrial" << "Japanese" << "Pop" << "Psytrance" << "Rap" << "Rock"
                 << "Soundtrack" << "Techno" << "Trance";

    KMenu* playTagRadioMenu = new KMenu( this );
    playTagRadioMenu->setTitle( i18n( "Global Tag Radio" ) );
    foreach( const QString &lastfmTag, m_lastfmTags )
    {
        KAction *lastfmAction = new KAction( lastfmTag, this );
        connect( lastfmAction, SIGNAL( triggered(bool) ), SLOT( playLastfmGlobaltag() ) );
        playTagRadioMenu->addAction( lastfmAction );
    }

    KMenu* addTagRadioMenu = new KMenu( this );
    addTagRadioMenu->setTitle( i18n( "Global Tag Radio" ) );
    foreach( const QString &lastfmTag, m_lastfmTags )
    {
        KAction *lastfmAction = new KAction( lastfmTag, this );
        connect( lastfmAction, SIGNAL( triggered(bool) ), SLOT( addLastfmGlobaltag() ) );
        addTagRadioMenu->addAction( lastfmAction );
    }

    KActionMenu* playLastfm = new KActionMenu( KIcon("audioscrobbler-amarok"), i18n( "Play las&t.fm Stream" ), ac);
    KMenu* playLastfmMenu = playLastfm->menu();
    playLastfmMenu->addAction( i18n( "Personal Radio" ), this, SLOT( playLastfmPersonal() ) );
    playLastfmMenu->addAction( i18n( "Neighbor Radio" ), this, SLOT( playLastfmNeighbor() ) );
    playLastfmMenu->addAction( i18n( "Custom Station" ), this, SLOT( playLastfmCustom() ) );
    playLastfmMenu->addMenu( playTagRadioMenu );
    ac->addAction( "lastfm_play", playLastfm );

    KActionMenu* addLastfm = new KActionMenu( KIcon("audioscrobbler-amarok"), i18n( "Add las&t.fm Stream" ), ac);
    KMenu* addLastfmMenu = addLastfm->menu();
    addLastfmMenu->addAction( i18n( "Personal Radio" ), this, SLOT( addLastfmPersonal() ) );
    addLastfmMenu->addAction( i18n( "Neighbor Radio" ), this, SLOT( addLastfmNeighbor() ) );
    addLastfmMenu->addAction( i18n( "Custom Station" ), this, SLOT( addLastfmCustom() ) );
    addLastfmMenu->addMenu( addTagRadioMenu );
    ac->addAction( "lastfm_add", addLastfm );
#endif

    KAction *previous = new KAction( this );
    previous->setIcon( KIcon("media-skip-backward-amarok") );
    previous->setText( i18n( "Previous Track" ) );
    ac->addAction( "prev", previous );
    connect( previous, SIGNAL(triggered(bool)), The::playlistModel(), SLOT( back() ) );

    KAction *play = new KAction( this );
    play->setIcon( KIcon("media-playback-start-amarok") );
    play->setText( i18n( "Play" ) );
    ac->addAction( "play", play );
    connect( play, SIGNAL(triggered(bool)), ec, SLOT( play() ));

    KAction *pause = new KAction( this );
    pause->setIcon( KIcon("media-playback-pause-amarok") );
    pause->setText( i18n( "Pause" ));
    ac->addAction( "pause", pause );
    connect( pause, SIGNAL(triggered(bool)), ec, SLOT( pause() ) );

    KAction *next = new KAction( this );
    next->setIcon( KIcon("media-skip-forward-amarok") );
    next->setText( i18n( "Next Track" ) );
    ac->addAction( "next", next );
    connect( next, SIGNAL(triggered(bool)), The::playlistModel(), SLOT( next() ) );

    KAction *toggleFocus = new KAction(i18n( "Toggle Focus" ), ac);
    toggleFocus->setShortcut( Qt::ControlModifier + Qt::Key_Tab );
    connect( toggleFocus, SIGNAL(triggered(bool)), SLOT( slotToggleFocus() ));

    PERF_LOG( "MainWindow::createActions 8" )
    new Amarok::MenuAction( ac );
    new Amarok::StopAction( ac );
    new Amarok::PlayPauseAction( ac );
    PERF_LOG( "Before Repeat Action" )
    new Amarok::RepeatAction( ac );
    PERF_LOG( "Before RandomAction" )
    new Amarok::RandomAction( ac );
    PERF_LOG( "BeforeFavorAction" )
    new Amarok::FavorAction( ac );

    PERF_LOG( "MainWindow::createActions 9" )
    if( K3bExporter::isAvailable() )
        new Amarok::BurnMenuAction( ac );

    ac->addAssociatedWidget( this );
    foreach (QAction* action, ac->actions())
        action->setShortcutContext(Qt::WindowShortcut);
}

void MainWindow::createMenus()
{

    

    //BEGIN Actions menu
    KMenu *actionsMenu;    
#ifdef Q_WS_MAC
    m_menubar = new QMenuBar(0);  // Fixes menubar in OS X
    actionsMenu = new KMenu( m_menubar );
    // Add these functions to the dock icon menu in OS X
    extern void qt_mac_set_dock_menu(QMenu *); 
    qt_mac_set_dock_menu(actionsMenu); 
    // Change to avoid duplicate menu titles in OS X
    actionsMenu->setTitle( i18n("&Playback") );
#else
    m_menubar = menuBar();
    actionsMenu = new KMenu( m_menubar );
    actionsMenu->setTitle( i18n("&Amarok") );
#endif    
    actionsMenu->addAction( actionCollection()->action("playlist_playmedia") );
    actionsMenu->addAction( actionCollection()->action("lastfm_play") );
    actionsMenu->addAction( actionCollection()->action("play_audiocd") );
    actionsMenu->addSeparator();
    actionsMenu->addAction( actionCollection()->action("prev") );
    actionsMenu->addAction( actionCollection()->action("play_pause") );
    actionsMenu->addAction( actionCollection()->action("stop") );
    actionsMenu->addAction( actionCollection()->action("next") );
    actionsMenu->addSeparator();
#ifdef Q_WS_MAC
    //BEGIN Mode submenu entries
    QAction *repeat = actionCollection()->action("repeat");
    connect( repeat, SIGNAL(triggered( int ) ), The::playlistModel(), SLOT(playlistRepeatMode(int) ) );
    actionsMenu->addAction( repeat );
    KSelectAction *random = static_cast<KSelectAction*>( actionCollection()->action("random_mode") );
    actionsMenu->addAction( random );
    random->menu()->addSeparator();
    random->menu()->addAction( actionCollection()->action("favor_tracks") );
    //END Mode submenu menu entries
#endif
#ifndef Q_WS_MAC    // Hide in OS X. Avoids duplicate "Quit" in dock menu
    actionsMenu->addAction( actionCollection()->action(KStandardAction::name(KStandardAction::Quit)) );
#endif
    //END Actions menu

    //BEGIN Playlist menu
    KMenu *playlistMenu = new KMenu( m_menubar );
    playlistMenu->setTitle( i18n("&Playlist") );
    playlistMenu->addAction( actionCollection()->action("playlist_add") );
    playlistMenu->addAction( actionCollection()->action("stream_add") );
    playlistMenu->addAction( actionCollection()->action("lastfm_add") );
    playlistMenu->addAction( actionCollection()->action("playlist_save") );
    playlistMenu->addAction( actionCollection()->action("playlist_burn") );
    playlistMenu->addAction( actionCollection()->action("podcasts_update") );
    playlistMenu->addSeparator();
    playlistMenu->addAction( actionCollection()->action("playlist_undo") );
    playlistMenu->addAction( actionCollection()->action("playlist_redo") );
    playlistMenu->addSeparator();
    playlistMenu->addAction( actionCollection()->action("playlist_clear") );
    playlistMenu->addAction( actionCollection()->action("playlist_shuffle") );

    playlistMenu->addSeparator();
    //FIXME: REENABLE When ported
//     playlistMenu->addAction( actionCollection()->action("queue_selected") );
    playlistMenu->addAction( actionCollection()->action("playlist_remove_duplicates") );
    playlistMenu->addAction( actionCollection()->action("playlist_select_all") );

    //END Playlist menu

    //BEGIN Mode menu
#ifndef Q_WS_MAC    // Hide here because we moved it to the Playback Menu in OS X. 
    KMenu *modeMenu = new KMenu( m_menubar );
    modeMenu->setTitle( i18n("&Mode") );
    QAction *repeat = actionCollection()->action("repeat");
    connect( repeat, SIGNAL(triggered( int ) ), The::playlistModel(), SLOT(playlistRepeatMode(int) ) );
    modeMenu->addAction( repeat );
    KSelectAction *random = static_cast<KSelectAction*>( actionCollection()->action("random_mode") );
    modeMenu->addAction( random );
    random->menu()->addSeparator();
    random->menu()->addAction( actionCollection()->action("favor_tracks") );
#endif
    //END Mode menu

    //BEGIN Tools menu
    m_toolsMenu = new KMenu( m_menubar );
    m_toolsMenu->setTitle( i18n("&Tools") );
    m_toolsMenu->addAction( actionCollection()->action("cover_manager") );
//FIXME: Reenable when ported//working
//     m_toolsMenu->addAction( actionCollection()->action("queue_manager") );
//     m_toolsMenu->addAction( actionCollection()->action("visualizations") );
//     m_toolsMenu->addAction( actionCollection()->action("equalizer") );
    m_toolsMenu->addAction( actionCollection()->action("script_manager") );
//     m_toolsMenu->addAction( actionCollection()->action("statistics") );
    m_toolsMenu->addSeparator();
    m_toolsMenu->addAction( actionCollection()->action("update_collection") );
    m_toolsMenu->addAction( actionCollection()->action("rescan_collection") );

#ifndef HAVE_LIBVISUAL
    actionCollection()->action( "visualizations" )->setEnabled( false );
#endif

    connect( m_toolsMenu, SIGNAL( aboutToShow() ), SLOT( toolsMenuAboutToShow() ) );
    connect( m_toolsMenu, SIGNAL( activated(int) ), SLOT( slotMenuActivated(int) ) );
    //END Tools menu

    //BEGIN Settings menu
    m_settingsMenu = new KMenu( m_menubar );
    m_settingsMenu->setTitle( i18n("&Settings") );
    //TODO use KStandardAction or KXmlGuiWindow
#ifndef Q_WS_MAC
    m_settingsMenu->addAction( actionCollection()->action( "toggle_toolbar" ) );
    m_settingsMenu->addSeparator();
#endif

//  m_settingsMenu->addAction( actionCollection()->action("options_configure_globals") );
    m_settingsMenu->addAction( actionCollection()->action(KStandardAction::name(KStandardAction::ConfigureToolbars)) );
    m_settingsMenu->addAction( actionCollection()->action(KStandardAction::name(KStandardAction::Preferences)) );
    m_settingsMenu->addAction( actionCollection()->action(KStandardAction::name(KStandardAction::KeyBindings)) );

    connect( m_settingsMenu, SIGNAL( activated(int) ), SLOT( slotMenuActivated(int) ) );
    //END Settings menu

    m_menubar->addMenu( actionsMenu );
    m_menubar->addMenu( playlistMenu );
#ifndef Q_WS_MAC
    m_menubar->addMenu( modeMenu );
#endif
    m_menubar->addMenu( m_toolsMenu );
    m_menubar->addMenu( m_settingsMenu );
    m_menubar->addMenu( Amarok::Menu::helpMenu() );
}

void MainWindow::paletteChange(const QPalette & oldPalette)
{
    QPixmapCache::clear();
}



#include "MainWindow.moc"
