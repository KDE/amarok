 /****************************************************************************************
 * Copyright (c) 2002-2009 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2002 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2002 Gabor Lehel <illissius@gmail.com>                                 *
 * Copyright (c) 2002 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Artur Szymiec <artur.szymiec@gmail.com>                           *
 * Copyright (c) 2010 Téo Mrnjavac <teo@kde.org>                                        *
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

#define DEBUG_PREFIX "MainWindow"

#include "MainWindow.h"

#include "aboutdialog/ExtendedAboutDialog.h"
#include "ActionClasses.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "EngineController.h" //for actions in ctor
#include "KNotificationBackend.h"
#include "Osd.h"
#include "PaletteHandler.h"
#include "ScriptManager.h"
#include "amarokconfig.h"
#include "aboutdialog/OcsData.h"
#include "amarokurls/AmarokUrlHandler.h"
#include "amarokurls/BookmarkManager.h"
#include "browsers/collectionbrowser/CollectionWidget.h"
#include "browsers/filebrowser/FileBrowser.h"
#include "browsers/playlistbrowser/PlaylistBrowser.h"
#include "browsers/servicebrowser/ServiceBrowser.h"
#include "context/ContextDock.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "covermanager/CoverManager.h" // for actions
#include "dialogs/EqualizerDialog.h"
#include "likeback/LikeBack.h"
#include "moodbar/MoodbarManager.h"
#include "network/NetworkAccessManagerProxy.h"
#ifdef DEBUG_BUILD_TYPE
#include "network/NetworkAccessViewer.h"
#endif // DEBUG_BUILD_TYPE
#include "playlist/layouts/LayoutConfigAction.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModelStack.h"
#include "playlist/PlaylistDock.h"
#include "playlist/ProgressiveSearchWidget.h"
#include "playlistmanager/file/PlaylistFileProvider.h"
#include "playlistmanager/PlaylistManager.h"
#include "PodcastCategory.h"
#include "services/ServicePluginManager.h"
#include "services/scriptable/ScriptableService.h"
#include "statusbar/StatusBar.h"
#include "toolbar/SlimToolbar.h"
#include "toolbar/MainToolbar.h"
#include "SvgHandler.h"
#include "widgets/Splitter.h"
//#include "mediabrowser.h"

#include <KAction>          //m_actionCollection
#include <KActionCollection>
#include <KApplication>     //kapp
#include <KFileDialog>      //openPlaylist()
#include <KInputDialog>     //slotAddStream()
#include <KMessageBox>
#include <KLocale>
#include <KMenu>
#include <KMenuBar>
#include <KPixmapCache>
#include <KBugReport>
#include <KStandardAction>
#include <KStandardDirs>
#include <KWindowSystem>
#include <kabstractfilewidget.h>

#include <plasma/plasma.h>

#include <QCheckBox>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QList>
#include <QSizeGrip>
#include <QStyle>
#include <QVBoxLayout>

#include <iostream>

#ifdef Q_WS_X11
#include <fixx11h.h>
#endif

#ifdef Q_WS_MAC
#include "mac/GrowlInterface.h"
#endif

#define AMAROK_CAPTION "Amarok"


extern KAboutData aboutData;
extern OcsData ocsData;

class ContextWidget : public KVBox
{
    // Set a useful size default of the center tab.
    public:
        ContextWidget( QWidget *parent ) : KVBox( parent ) {}

        QSize sizeHint() const
        {
            return QSize( static_cast<QWidget*>( parent() )->size().width() / 3, 300 );
        }
};

QWeakPointer<MainWindow> MainWindow::s_instance;

namespace The {
    MainWindow* mainWindow() { return MainWindow::s_instance.data(); }
}

MainWindow::MainWindow()
    : KMainWindow( 0 )
    , m_showMenuBar( 0 )
    , m_lastBrowser( 0 )
    , m_layoutEverRestored( false )
    , m_waitingForCd( false )
    , m_mouseDown( false )
    , m_LH_initialized( false )
{
    DEBUG_BLOCK

    m_saveLayoutChangesTimer = new QTimer( this );
    m_saveLayoutChangesTimer->setSingleShot( true );
    connect( m_saveLayoutChangesTimer, SIGNAL( timeout() ), this, SLOT( saveLayout() ) );

    setObjectName( "MainWindow" );
    s_instance = this;

#ifdef Q_WS_MAC
    QSizeGrip* grip = new QSizeGrip( this );
    GrowlInterface* growl = new GrowlInterface( qApp->applicationName() );
#endif


    /* The ServicePluginManager needs to be loaded before the playlist model
    * (which gets started by "statusBar" below up so that it can handle any
    * tracks in the saved playlist that are associated with services. Eg, if
    * the playlist has a Magnatune track in it when Amarok is closed, then the
    * Magnatune service needs to be initialized before the playlist is loaded
    * here. */

    ServicePluginManager::instance();

    StatusBar * statusBar = new StatusBar( this );

    setStatusBar( statusBar );

    // Sets caption and icon correctly (needed e.g. for GNOME)
//     kapp->setTopWidget( this );
    PERF_LOG( "Set Top Widget" )
    createActions();
    PERF_LOG( "Created actions" )

    //new K3bExporter();

    The::paletteHandler()->setPalette( palette() );
    setPlainCaption( i18n( AMAROK_CAPTION ) );

    init();  // We could as well move the code from init() here, but meh.. getting a tad long

    //restore active category ( as well as filters and levels and whatnot.. )
    const QString path = Amarok::config().readEntry( "Browser Path", QString() );
    if( !path.isEmpty() )
        m_browserDock.data()->list()->navigate( path );

    setAutoSaveSettings();

    m_showMenuBar->setChecked(!menuBar()->isHidden());  // workaround for bug #171080

    EngineController *engine = The::engineController();
    connect( engine, SIGNAL( stopped( qint64, qint64 ) ),
             this, SLOT( slotStopped() ) );
    connect( engine, SIGNAL( paused() ),
             this, SLOT( slotPaused() ) );
    connect( engine, SIGNAL( trackPlaying( Meta::TrackPtr ) ),
             this, SLOT( slotNewTrackPlaying() ) );
    connect( engine, SIGNAL( trackMetadataChanged( Meta::TrackPtr ) ),
             this, SLOT( slotMetadataChanged( Meta::TrackPtr ) ) );
}

MainWindow::~MainWindow()
{
    DEBUG_BLOCK

    //save currently active category
    Amarok::config().writeEntry( "Browser Path", m_browserDock.data()->list()->path() );

    QList<int> sPanels;

    //foreach( int a, m_splitter->saveState() )
    //    sPanels.append( a );

    //AmarokConfig::setPanelsSavedState( sPanels );

    //delete m_splitter;
#ifdef DEBUG_BUILD_TYPE
    delete m_networkViewer.data();
#endif // DEBUG_BUILD_TYPE
    delete The::statusBar();
    delete The::svgHandler();
    delete The::paletteHandler();
}


///////// public interface

/**
 * This function will initialize the main window.
 */
void
MainWindow::init()
{
    DEBUG_BLOCK

    layout()->setContentsMargins( 0, 0, 0, 0 );
    layout()->setSpacing( 0 );

    //create main toolbar
    m_mainToolbar = new MainToolbar( 0 );
    m_mainToolbar.data()->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
    m_mainToolbar.data()->setMovable ( true );
    addToolBar( Qt::TopToolBarArea, m_mainToolbar.data() );
    m_mainToolbar.data()->hide();

    //create slim toolbar
    m_slimToolbar = new SlimToolbar( 0 );
    m_slimToolbar.data()->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
    m_slimToolbar.data()->setMovable ( true );
    addToolBar( Qt::TopToolBarArea, m_slimToolbar.data() );
    m_slimToolbar.data()->hide();

    //BEGIN Creating Widgets
    PERF_LOG( "Create sidebar" )
    m_browserDock = new BrowserDock( this );
    m_browserDock.data()->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );

    m_browserDock.data()->installEventFilter( this );
    PERF_LOG( "Sidebar created" )

    PERF_LOG( "Create Playlist" )
    m_playlistDock = new Playlist::Dock( this );
    m_playlistDock.data()->installEventFilter( this );
    PERF_LOG( "Playlist created" )

    PERF_LOG( "Creating ContextWidget" )
    m_contextDock = new ContextDock( this );
    m_contextDock.data()->installEventFilter( this );

    PERF_LOG( "ContextScene created" )
    //END Creating Widgets

    createMenus();

    PERF_LOG( "Loading default contextScene" )

    PERF_LOG( "Loaded default contextScene" )

    setDockOptions ( QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::AnimatedDocks );

    addDockWidget( Qt::LeftDockWidgetArea, m_browserDock.data() );
    addDockWidget( Qt::LeftDockWidgetArea, m_contextDock.data(), Qt::Horizontal );
    addDockWidget( Qt::LeftDockWidgetArea, m_playlistDock.data(), Qt::Horizontal );

    setLayoutLocked( AmarokConfig::lockLayout() );

    //<Browsers>
    {
        Debug::Block block( "Creating browsers. Please report long start times!" );

        PERF_LOG( "Creating CollectionWidget" )
        m_collectionBrowser = new CollectionWidget( "collections", 0 );
        m_collectionBrowser->setPrettyName( i18n( "Local Music" ) );
        m_collectionBrowser->setIcon( KIcon( "collection-amarok" ) );
        m_collectionBrowser->setShortDescription( i18n( "Local sources of content" ) );
        m_browserDock.data()->list()->addCategory( m_collectionBrowser );
        PERF_LOG( "Created CollectionWidget" )


        PERF_LOG( "Creating ServiceBrowser" )
        ServiceBrowser *internetContentServiceBrowser = ServiceBrowser::instance();
        internetContentServiceBrowser->setParent( 0 );
        internetContentServiceBrowser->setPrettyName( i18n( "Internet" ) );
        internetContentServiceBrowser->setIcon( KIcon( "applications-internet" ) );
        internetContentServiceBrowser->setShortDescription( i18n( "Online sources of content" ) );
        m_browserDock.data()->list()->addCategory( internetContentServiceBrowser );
        PERF_LOG( "Created ServiceBrowser" )

        PERF_LOG( "Creating PlaylistBrowser" )
        m_playlistBrowser = new PlaylistBrowserNS::PlaylistBrowser( "playlists", 0 );
        m_playlistBrowser->setPrettyName( i18n("Playlists") );
        m_playlistBrowser->setIcon( KIcon( "view-media-playlist-amarok" ) );
        m_playlistBrowser->setShortDescription( i18n( "Various types of playlists" ) );
        m_browserDock.data()->list()->addCategory( m_playlistBrowser );
        PERF_LOG( "CreatedPlaylsitBrowser" )


        PERF_LOG( "Creating FileBrowser" )
        FileBrowser * fileBrowserMkII = new FileBrowser( "files", 0 );
        fileBrowserMkII->setPrettyName( i18n("Files") );
        fileBrowserMkII->setIcon( KIcon( "folder-amarok" ) );
        fileBrowserMkII->setShortDescription( i18n( "Browse local hard drive for content" ) );
        m_browserDock.data()->list()->addCategory( fileBrowserMkII );


        PERF_LOG( "Created FileBrowser" )

        PERF_LOG( "Initialising ServicePluginManager" )
        ServicePluginManager::instance()->init();
        PERF_LOG( "Initialised ServicePluginManager" )

        internetContentServiceBrowser->setScriptableServiceManager( The::scriptableServiceManager() );
        PERF_LOG( "ScriptableServiceManager done" )

        PERF_LOG( "Creating Podcast Category" )
        m_browserDock.data()->list()->addCategory( The::podcastCategory() );
        PERF_LOG( "Created Podcast Category" )

        PERF_LOG( "finished MainWindow::init" )
    }

    The::amarokUrlHandler(); //Instantiate

    // Runtime check for Qt 4.6 here.
    // We delete the layout file once, because of binary incompatibility with older Qt version.
    // @see: https://bugs.kde.org/show_bug.cgi?id=213990
    const QChar major = qVersion()[0];
    const QChar minor = qVersion()[2];
    if( major.digitValue() >= 4 && minor.digitValue() > 5 )
    {
        KConfigGroup config = Amarok::config();
        if( !config.readEntry( "LayoutFileDeleted", false ) )
        {
            QFile::remove( Amarok::saveLocation() + "layout" );
            config.writeEntry( "LayoutFileDeleted", true );
            config.sync();
        }
    }

    // we must filter ourself to get mouseevents on the "splitter" - what is us, but filtered by the layouter
    installEventFilter( this );

    // restore the layout on app start
    restoreLayout();

    // wait for the eventloop
    QTimer::singleShot( 0, this, SLOT( initLayoutHack() ) );
}


QMenu*
MainWindow::createPopupMenu()
{
    QMenu* menu = new QMenu( this );

    // Show/hide menu bar
    if (!menuBar()->isVisible())
        menu->addAction( m_showMenuBar );

    menu->addSeparator();

    addViewMenuItems(menu);

    return menu;
}

void
MainWindow::addViewMenuItems(QMenu* menu)
{
    DEBUG_BLOCK
    menu->setTitle( i18nc("@item:inmenu", "&View" ) );

    // Layout locking:
    QAction* lockAction = new QAction( i18n( "Lock Layout" ), this );
    lockAction->setCheckable( true );
    lockAction->setChecked( AmarokConfig::lockLayout() );
    connect( lockAction, SIGNAL( toggled( bool ) ), SLOT( setLayoutLocked( bool ) ) );
    menu->addAction( lockAction );

    menu->addSeparator();

    // Dock widgets:
    QList<QDockWidget *> dockwidgets = qFindChildren<QDockWidget *>( this );

    foreach( QDockWidget* dockWidget, dockwidgets )
    {
        if( dockWidget->parentWidget() == this )
            menu->addAction( dockWidget->toggleViewAction() );
    }

    menu->addSeparator();

    // Toolbars:
    QList<QToolBar *> toolbars = qFindChildren<QToolBar *>( this );
    QActionGroup* toolBarGroup = new QActionGroup( this );
    toolBarGroup->setExclusive( true );

    foreach( QToolBar* toolBar, toolbars )
    {
        if( toolBar->parentWidget() == this )
        {
            QAction* action = toolBar->toggleViewAction();
            connect( action, SIGNAL( toggled( bool ) ), toolBar, SLOT( setVisible( bool ) ) );
            toolBarGroup->addAction( action );
            menu->addAction( action );
        }
    }
}

void
MainWindow::showBrowser( const QString &name )
{
    const int index = m_browserNames.indexOf( name );
    showBrowser( index );
}

void
MainWindow::showBrowser( const int index )
{
    Q_UNUSED( index )
    //if( index >= 0 && index != m_browsersDock->currentIndex() )
    //    m_browsersDock->showWidget( index );
}

void
MainWindow::showDock( AmarokDockId dockId )
{
    QString name;
    switch( dockId )
    {
        case AmarokDockNavigation:
            name = m_browserDock.data()->windowTitle();
            break;
        case AmarokDockContext:
            name = m_contextDock.data()->windowTitle();
            break;
        case AmarokDockPlaylist:
            name = m_playlistDock.data()->windowTitle();
            break;
    }
  
    QList < QTabBar * > tabList = findChildren < QTabBar * > ();

    foreach( QTabBar *bar, tabList )
    {
        for( int i = 0; i < bar->count(); i++ )
        {
            if( bar->tabText( i ) == name )
            {
                bar->setCurrentIndex( i );
                break;
            }
        }
    }
}

void
MainWindow::saveLayout()  //SLOT
{
    DEBUG_BLOCK

    // Do not save the layout if the main window is hidden
    // Qt takes widgets out of the layout if they're not visible.
    // So this is not going to work. Also see bug 244583
    if (!isVisible())
        return;

    //save layout to file. Does not go into to rc as it is binary data.
    QFile file( Amarok::saveLocation() + "layout" );

    if( file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
        file.write( saveState( LAYOUT_VERSION ) );

        #ifdef Q_OS_UNIX  // fsync() only exists on Posix
        fsync( file.handle() );
        #endif

        file.close();
    }
}

void
MainWindow::keyPressEvent( QKeyEvent *e )
{
    if( !( e->modifiers() & Qt::ControlModifier ) )
        return KMainWindow::keyPressEvent( e );

    /*int n = -1;
    switch( e->key() )
    {
        case Qt::Key_0: n = 0; break;
        case Qt::Key_1: n = 1; break;
        case Qt::Key_2: n = 2; break;
        case Qt::Key_3: n = 3; break;
        case Qt::Key_4: n = 4; break;
        default:
            return KMainWindow::keyPressEvent( e );
    }
    if( n == 0 && m_browsersDock->currentIndex() >= 0 )
        m_browsersDock->showWidget( m_browsersDock->currentIndex() );
    else if( n > 0 )
        showBrowser( n - 1 ); // map from human to computer counting*/
}


void
MainWindow::closeEvent( QCloseEvent *e )
{
    DEBUG_BLOCK

    debug() << "Saving layout before minimizing the MainWindow";
    saveLayout();

#ifdef Q_WS_MAC
    Q_UNUSED( e );
    hide();
#else

    //KDE policy states we should hide to tray and not quit() when the
    //close window button is pushed for the main widget
    if( AmarokConfig::showTrayIcon() && e->spontaneous() && !kapp->sessionSaving() )
    {
        KMessageBox::information( this,
                i18n( "<qt>Closing the main window will keep Amarok running in the System Tray. "
                      "Use <B>Quit</B> from the menu, or the Amarok tray icon to exit the application.</qt>" ),
                i18n( "Docking in System Tray" ), "hideOnCloseInfo" );

        hide();
        e->ignore();
        return;
    }

    e->accept();
    kapp->quit();
#endif
}

void
MainWindow::showEvent(QShowEvent* e)
{
    DEBUG_BLOCK

    // restore layout on first maximize request after start. See bug 244583
    if (!m_layoutEverRestored)
        restoreLayout();

    static bool windowEverShown = false;
    if ( !windowEverShown )
    {
        windowEverShown = true;
        QTimer::singleShot( 250, this, SLOT( resizeWindowHack() ) );
    }

    QWidget::showEvent(e);
}


bool
MainWindow::queryExit()
{
    DEBUG_BLOCK

    // save layout on app exit
    saveLayout();
    
    return true; // KMainWindow API expects us to always return true
}


void
MainWindow::exportPlaylist() const //SLOT
{
    DEBUG_BLOCK

    KFileDialog fileDialog( KUrl("kfiledialog:///amarok-playlist-export"), QString(), 0 );
    QCheckBox *saveRelativeCheck = new QCheckBox( i18n("Use relative path for &saving") );

    QStringList supportedMimeTypes;
    supportedMimeTypes << "audio/x-mpegurl"; //M3U
    supportedMimeTypes << "audio/x-scpls"; //PLS
    supportedMimeTypes << "application/xspf+xml"; //XSPF

    fileDialog.setMimeFilter( supportedMimeTypes, supportedMimeTypes.first() );
    fileDialog.fileWidget()->setCustomWidget( saveRelativeCheck );
    fileDialog.setOperationMode( KFileDialog::Saving );
    fileDialog.setMode( KFile::File );
    fileDialog.setCaption( i18n("Save As") );
    fileDialog.setObjectName( "PlaylistExport" );

    fileDialog.exec();

    QString playlistPath = fileDialog.selectedFile();

    if( !playlistPath.isEmpty() )
    {
        AmarokConfig::setRelativePlaylist( saveRelativeCheck->isChecked() );
        The::playlist()->exportPlaylist( playlistPath );
    }
}

void
MainWindow::slotShowActiveTrack() const
{
    m_playlistDock.data()->showActiveTrack();
}

void
MainWindow::slotShowCoverManager() const //SLOT
{
    CoverManager::showOnce();
}

void MainWindow::slotShowBookmarkManager() const
{
    The::bookmarkManager()->showOnce();
}

void MainWindow::slotShowEqualizer() const
{
    The::equalizer()->showOnce();
}

void
MainWindow::slotPlayMedia() //SLOT
{
    // Request location and immediately start playback
    slotAddLocation( true );
}

void
MainWindow::slotAddLocation( bool directPlay ) //SLOT
{
    static KUrl lastDirectory;

    // open a file selector to add media to the playlist
    KUrl::List files;
    KFileDialog dlg( KUrl(QDesktopServices::storageLocation(QDesktopServices::MusicLocation) ), QString("*.*|"), this );

    if( !lastDirectory.isEmpty() )
        dlg.setUrl( lastDirectory );

    dlg.setCaption( directPlay ? i18n("Play Media (Files or URLs)") : i18n("Add Media (Files or URLs)") );
    dlg.setMode( KFile::Files | KFile::Directory );
    dlg.setObjectName( "PlayMedia" );
    dlg.exec();
    files = dlg.selectedUrls();

    lastDirectory = dlg.baseUrl();

    if( files.isEmpty() )
        return;

    The::playlistController()->insertOptioned( files, directPlay ? Playlist::AppendAndPlayImmediately : Playlist::AppendAndPlay );
}

void
MainWindow::slotAddStream() //SLOT
{
    bool ok;
    QString url = KInputDialog::getText( i18n("Add Stream"), i18n("Enter Stream URL:"), QString(), &ok, this );

    if( !ok )
        return;

    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl( url ) );

    The::playlistController()->insertOptioned( track, Playlist::Append );
}

void
MainWindow::slotJumpTo() // slot
{
    DEBUG_BLOCK

    m_playlistDock.data()->searchWidget()->focusInputLine();
}

void
MainWindow::showScriptSelector() //SLOT
{
    ScriptManager::instance()->show();
    ScriptManager::instance()->raise();
}

#ifdef DEBUG_BUILD_TYPE
void
MainWindow::showNetworkRequestViewer() //SLOT
{
    if( !m_networkViewer )
    {
        m_networkViewer = new NetworkAccessViewer( this );
        The::networkAccessManager()->setNetworkAccessViewer( m_networkViewer.data() );

    }
    The::networkAccessManager()->networkAccessViewer()->show();
}
#endif // DEBUG_BUILD_TYPE

/**
 * "Toggle Main Window" global shortcut connects to this slot
 */
void
MainWindow::showHide() //SLOT
{
    const KWindowInfo info = KWindowSystem::windowInfo( winId(), 0, 0 );
    const int currentDesktop = KWindowSystem::currentDesktop();

    if( !isVisible() )
    {
        setVisible( true );
    }
    else
    {
        if( !isMinimized() )
        {
            if( !isActiveWindow() ) // not minimised and without focus
            {
                KWindowSystem::setOnDesktop( winId(), currentDesktop );
                KWindowSystem::activateWindow( winId() );
            }
            else // Amarok has focus
            {
                setVisible( false );
            }
        }
        else // Amarok is minimised
        {
            setWindowState( windowState() & ~Qt::WindowMinimized );
            KWindowSystem::setOnDesktop( winId(), currentDesktop );
            KWindowSystem::activateWindow( winId() );
        }
    }
}

void
MainWindow::showNotificationPopup() // slot
{
    if( Amarok::KNotificationBackend::instance()->isEnabled()
            && !Amarok::OSD::instance()->isEnabled() )
        Amarok::KNotificationBackend::instance()->showCurrentTrack();
    else
        Amarok::OSD::instance()->forceToggleOSD();
}

void
MainWindow::slotFullScreen() // slot
{
    setWindowState( windowState() ^ Qt::WindowFullScreen );
}

void
MainWindow::slotLoveTrack()
{
    emit loveTrack( The::engineController()->currentTrack() );
}

void
MainWindow::activate()
{
#ifdef Q_WS_X11
    const KWindowInfo info = KWindowSystem::windowInfo( winId(), 0, 0 );

    if( KWindowSystem::activeWindow() != winId() )
        setVisible( true );
    else if( !info.isMinimized() )
        setVisible( true );
    if( !isHidden() )
        KWindowSystem::activateWindow( winId() );
#else
    setVisible( true );
#endif
}

bool
MainWindow::isReallyShown() const
{
#ifdef Q_WS_X11
    const KWindowInfo info = KWindowSystem::windowInfo( winId(), NET::WMDesktop, 0 );
    return !isHidden() && !info.isMinimized() && info.isOnDesktop( KWindowSystem::currentDesktop() );
#else
    return !isHidden();
#endif
}

void
MainWindow::createActions()
{
    KActionCollection* const ac = Amarok::actionCollection();
    const EngineController* const ec = The::engineController();
    const Playlist::Actions* const pa = The::playlistActions();
    const Playlist::Controller* const pc = The::playlistController();

    KStandardAction::keyBindings( kapp, SLOT( slotConfigShortcuts() ), ac );
    m_showMenuBar = KStandardAction::showMenubar(this, SLOT(slotShowMenuBar()), ac);
    KStandardAction::preferences( kapp, SLOT( slotConfigAmarok() ), ac );
    ac->action( KStandardAction::name( KStandardAction::KeyBindings ) )->setIcon( KIcon( "configure-shortcuts-amarok" ) );
    ac->action( KStandardAction::name( KStandardAction::Preferences ) )->setIcon( KIcon( "configure-amarok" ) );
    ac->action( KStandardAction::name( KStandardAction::Preferences ) )->setMenuRole(QAction::PreferencesRole); // Define OS X Prefs menu here, removes need for ifdef later

    KStandardAction::quit( kapp, SLOT( quit() ), ac );

    KAction *action = new KAction( KIcon( "folder-amarok" ), i18n("&Add Media..."), this );
    ac->addAction( "playlist_add", action );
    connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotAddLocation() ) );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_A ) );

    action = new KAction( KIcon( "edit-clear-list-amarok" ), i18nc( "clear playlist", "&Clear Playlist" ), this );
    connect( action, SIGNAL( triggered( bool ) ), pc, SLOT( clear() ) );
    ac->addAction( "playlist_clear", action );

    action = new KAction( i18nc( "Remove duplicate and dead (unplayable) tracks from the playlist", "Re&move Duplicates" ), this );
    connect( action, SIGNAL( triggered( bool ) ), pc, SLOT( removeDeadAndDuplicates() ) );
    ac->addAction( "playlist_remove_dead_and_duplicates", action );

    action = new Playlist::LayoutConfigAction( this );
    ac->addAction( "playlist_layout", action );

    action = new KAction( KIcon( "folder-amarok" ), i18n("&Add Stream..."), this );
    connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotAddStream() ) );
    ac->addAction( "stream_add", action );

    action = new KAction( KIcon( "document-export-amarok" ), i18n("&Export Playlist As..."), this );
    connect( action, SIGNAL( triggered( bool ) ), this, SLOT( exportPlaylist() ) );
    ac->addAction( "playlist_export", action );

    action = new KAction( KIcon( "bookmark-new" ), i18n( "Bookmark Media Sources View" ), this );
    ac->addAction( "bookmark_browser", action );
    connect( action, SIGNAL( triggered() ), The::amarokUrlHandler(), SLOT( bookmarkCurrentBrowserView() ) );

    action = new KAction( KIcon( "bookmarks-organize" ), i18n( "Bookmark Manager" ), this );
    ac->addAction( "bookmark_manager", action );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( slotShowBookmarkManager() ) );

    action = new KAction( KIcon( "view-media-equalizer" ), i18n( "Equalizer" ), this );
    ac->addAction( "equalizer_dialog", action );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( slotShowEqualizer() ) );

    action = new KAction( KIcon( "bookmark-new" ), i18n( "Bookmark Playlist Setup" ), this );
    ac->addAction( "bookmark_playlistview", action );
    connect( action, SIGNAL( triggered() ), The::amarokUrlHandler(), SLOT( bookmarkCurrentPlaylistView() ) );

    action = new KAction( KIcon( "bookmark-new" ), i18n( "Bookmark Context Applets" ), this );
    ac->addAction( "bookmark_contextview", action );
    connect( action, SIGNAL( triggered() ), The::amarokUrlHandler(), SLOT( bookmarkCurrentContextView() ) );

    action = new KAction( KIcon( "media-album-cover-manager-amarok" ), i18n( "Cover Manager" ), this );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( slotShowCoverManager() ) );
    ac->addAction( "cover_manager", action );

    action = new KAction( KIcon("folder-amarok"), i18n("Play Media..."), this );
    ac->addAction( "playlist_playmedia", action );
    action->setShortcut( Qt::CTRL + Qt::Key_O );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( slotPlayMedia() ) );

    action = new KAction( KIcon("preferences-plugin-script-amarok"), i18n("Script Manager"), this );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( showScriptSelector() ) );
    ac->addAction( "script_manager", action );

    action = new KAction( KIcon( "media-seek-forward-amarok" ), i18n("&Seek Forward"), this );
    ac->addAction( "seek_forward", action );
    action->setShortcut( Qt::Key_Right );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::SHIFT + Qt::Key_Plus ) );
    connect( action, SIGNAL( triggered( bool ) ), ec, SLOT( seekForward() ) );

    action = new KAction( KIcon( "media-seek-backward-amarok" ), i18n("&Seek Backward"), this );
    ac->addAction( "seek_backward", action );
    action->setShortcut( Qt::Key_Left );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::SHIFT + Qt::Key_Minus ) );
    connect( action, SIGNAL( triggered( bool ) ), ec, SLOT( seekBackward() ) );

    PERF_LOG( "MainWindow::createActions 6" )
    action = new KAction( KIcon("collection-refresh-amarok"), i18n( "Update Collection" ), this );
    connect ( action, SIGNAL( triggered( bool ) ), CollectionManager::instance(), SLOT( checkCollectionChanges() ) );
    ac->addAction( "update_collection", action );

    action = new KAction( this );
    ac->addAction( "prev", action );
    action->setIcon( KIcon("media-skip-backward-amarok") );
    action->setText( i18n( "Previous Track" ) );
    action->setGlobalShortcut( KShortcut( Qt::Key_MediaPrevious ) );
    connect( action, SIGNAL( triggered( bool ) ), pa, SLOT( back() ) );

    action = new KAction( this );
    ac->addAction( "replay", action );
    action->setIcon( KIcon("media-playback-start") );
    action->setText( i18n( "Restart current track" ) );
    action->setGlobalShortcut( KShortcut() );
    connect( action, SIGNAL( triggered( bool ) ), ec, SLOT(replay()) );

    action = new KAction( this );
    ac->addAction( "repopulate", action );
    action->setText( i18n( "Repopulate Playlist" ) );
    action->setIcon( KIcon("view-refresh-amarok") );
    connect( action, SIGNAL( triggered( bool ) ), pa, SLOT( repopulateDynamicPlaylist() ) );

    action = new KAction( this );
    ac->addAction( "disable_dynamic", action );
    action->setText( i18n( "Disable Dynamic Playlist" ) );
    action->setIcon( KIcon("edit-delete-amarok") );
    //this is connected inside the dynamic playlist category

    action = new KAction( KIcon("media-skip-forward-amarok"), i18n( "Next Track" ), this );
    ac->addAction( "next", action );
    action->setGlobalShortcut( KShortcut( Qt::Key_MediaNext ) );
    connect( action, SIGNAL( triggered( bool ) ), pa, SLOT( next() ) );

    action = new KAction( i18n( "Increase Volume" ), this );
    ac->addAction( "increaseVolume", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_Plus ) );
    action->setShortcut( Qt::Key_Plus );
    connect( action, SIGNAL( triggered() ), ec, SLOT( increaseVolume() ) );

    action = new KAction( i18n( "Decrease Volume" ), this );
    ac->addAction( "decreaseVolume", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_Minus ) );
    action->setShortcut( Qt::Key_Minus );
    connect( action, SIGNAL( triggered() ), ec, SLOT( decreaseVolume() ) );

    action = new KAction( i18n( "Toggle Main Window" ), this );
    ac->addAction( "toggleMainWindow", action );
    action->setGlobalShortcut( KShortcut() );
    connect( action, SIGNAL( triggered() ), SLOT( showHide() ) );

    action = new KAction( i18n( "Toggle Full Screen" ), this );
    ac->addAction( "toggleFullScreen", action );
    action->setShortcut( KShortcut( Qt::CTRL + Qt::SHIFT + Qt::Key_F ) );
    connect( action, SIGNAL( triggered() ), SLOT( slotFullScreen() ) );

    action = new KAction( i18n( "Jump to" ), this );
    ac->addAction( "jumpTo", action );
    action->setShortcut( KShortcut( Qt::CTRL + Qt::Key_J ) );
    connect( action, SIGNAL( triggered() ), SLOT( slotJumpTo() ) );

    action = new KAction( KIcon( "music-amarok" ), i18n("Show active track"), this );
    ac->addAction( "show_active_track", action );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( slotShowActiveTrack() ) );

    action = new KAction( i18n( "Show Notification Popup" ), this );
    ac->addAction( "showNotificationPopup", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_O ) );
    connect( action, SIGNAL( triggered() ), SLOT( showNotificationPopup() ) );

    action = new KAction( i18n( "Mute Volume" ), this );
    ac->addAction( "mute", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_M ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( toggleMute() ) );

    action = new KAction( i18n( "Last.fm: Love Current Track" ), this );
    ac->addAction( "loveTrack", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_L ) );
    connect( action, SIGNAL( triggered() ), SLOT( slotLoveTrack() ) );

    action = new KAction( i18n( "Last.fm: Ban Current Track" ), this );
    ac->addAction( "banTrack", action );
    //action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_B ) );
    connect( action, SIGNAL( triggered() ), SIGNAL( banTrack() ) );

    action = new KAction( i18n( "Last.fm: Skip Current Track" ), this );
    ac->addAction( "skipTrack", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_S ) );
    connect( action, SIGNAL( triggered() ), SIGNAL( skipTrack() ) );

    action = new KAction( KIcon( "media-track-queue-amarok" ), i18n( "Queue Track" ), this );
    ac->addAction( "queueTrack", action );
    action->setShortcut( KShortcut( Qt::CTRL + Qt::Key_D ) );
    connect( action, SIGNAL( triggered() ), SIGNAL( switchQueueStateShortcut() ) );

    action = new KAction( i18n( "Rate Current Track: 1" ), this );
    ac->addAction( "rate1", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_1 ) );
    connect( action, SIGNAL( triggered() ), SLOT( setRating1() ) );

    action = new KAction( i18n( "Rate Current Track: 2" ), this );
    ac->addAction( "rate2", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_2 ) );
    connect( action, SIGNAL( triggered() ), SLOT( setRating2() ) );

    action = new KAction( i18n( "Rate Current Track: 3" ), this );
    ac->addAction( "rate3", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_3 ) );
    connect( action, SIGNAL( triggered() ), SLOT( setRating3() ) );

    action = new KAction( i18n( "Rate Current Track: 4" ), this );
    ac->addAction( "rate4", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_4 ) );
    connect( action, SIGNAL( triggered() ), SLOT( setRating4() ) );

    action = new KAction( i18n( "Rate Current Track: 5" ), this );
    ac->addAction( "rate5", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_5 ) );
    connect( action, SIGNAL( triggered() ), SLOT( setRating5() ) );

#ifdef DEBUG_BUILD_TYPE
    action = new KAction( i18n( "Network Request Viewer" ), this );
    ac->addAction( "network_request_viewer", action );
    action->setIcon( KIcon( "utilities-system-monitor" ) );
    connect( action, SIGNAL( triggered() ), SLOT( showNetworkRequestViewer() ) );
#endif // DEBUG_BUILD_TYPE

    action = KStandardAction::redo( pc, SLOT( redo() ), this);
    ac->addAction( "playlist_redo", action );
    action->setEnabled( false );
    action->setIcon( KIcon( "edit-redo-amarok" ) );
    connect( pc, SIGNAL( canRedoChanged( bool ) ), action, SLOT( setEnabled( bool ) ) );

    action = KStandardAction::undo( pc, SLOT( undo() ), this);
    ac->addAction( "playlist_undo", action );
    action->setEnabled( false );
    action->setIcon( KIcon( "edit-undo-amarok" ) );
    connect( pc, SIGNAL( canUndoChanged( bool ) ), action, SLOT( setEnabled( bool ) ) );

    action = new KAction( KIcon( "amarok" ), i18n( "&About Amarok" ), this );
    ac->addAction( "extendedAbout", action );
    connect( action, SIGNAL( triggered() ), SLOT( showAbout() ) );

    action = new KAction( KIcon( "tools-report-bug" ), i18n("&Report Bug..."), this );
    ac->addAction( "reportBug", action );
    connect( action, SIGNAL( triggered() ), SLOT( showReportBug() ) );

    LikeBack *likeBack = new LikeBack( LikeBack::AllButBugs,
        LikeBack::isDevelopmentVersion( KGlobal::mainComponent().aboutData()->version() ) );
    likeBack->setServer( "likeback.kollide.net", "/send.php" );
    likeBack->setAcceptedLanguages( QStringList( "en" ) );
    likeBack->setWindowNamesListing( LikeBack::WarnUnnamedWindows );    //Notify if a window has no name

    KActionCollection *likeBackActions = new KActionCollection( this, KGlobal::mainComponent() );
    likeBackActions->addAssociatedWidget( this );
    likeBack->createActions( likeBackActions );

    ac->addAction( "likeBackSendComment", likeBackActions->action( "likeBackSendComment" ) );
    ac->addAction( "likeBackShowIcons", likeBackActions->action( "likeBackShowIcons" ) );

    PERF_LOG( "MainWindow::createActions 8" )
    new Amarok::MenuAction( ac, this );
    new Amarok::StopAction( ac, this );
    new Amarok::StopPlayingAfterCurrentTrackAction( ac, this );
    new Amarok::PlayPauseAction( ac, this );
    new Amarok::ReplayGainModeAction( ac, this );
    new Amarok::EqualizerAction( ac, this);

    ac->addAssociatedWidget( this );
    foreach( QAction* action, ac->actions() )
        action->setShortcutContext( Qt::WindowShortcut );
}

void
MainWindow::setRating( int n )
{
    n *= 2;

    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( track )
    {
        // if we're setting an identical rating then we really must
        // want to set the half-star below rating
        if( track->rating() == n )
            n -= 1;

        track->setRating( n );
        Amarok::OSD::instance()->OSDWidget::ratingChanged( track->rating() );
    }
}

void
MainWindow::createMenus()
{
    //BEGIN Actions menu
    KMenu *actionsMenu;
#ifdef Q_WS_MAC
    m_menubar = new QMenuBar( 0 );  // Fixes menubar in OS X
    actionsMenu = new KMenu( m_menubar.data() );
    // Add these functions to the dock icon menu in OS X
    //extern void qt_mac_set_dock_menu(QMenu *);
    //qt_mac_set_dock_menu(actionsMenu);
    // Change to avoid duplicate menu titles in OS X
    actionsMenu->setTitle( i18n("&Music") );
#else
    m_menubar = menuBar();
    actionsMenu = new KMenu( m_menubar.data() );
    actionsMenu->setTitle( i18n("&Amarok") );
#endif
    actionsMenu->addAction( Amarok::actionCollection()->action("playlist_playmedia") );
    actionsMenu->addSeparator();
    actionsMenu->addAction( Amarok::actionCollection()->action("prev") );
    actionsMenu->addAction( Amarok::actionCollection()->action("play_pause") );
    actionsMenu->addAction( Amarok::actionCollection()->action("stop") );
    actionsMenu->addAction( Amarok::actionCollection()->action("stop_after_current") );
    actionsMenu->addAction( Amarok::actionCollection()->action("next") );


#ifndef Q_WS_MAC    // Avoid duplicate "Quit" in OS X dock menu
    actionsMenu->addSeparator();
    actionsMenu->addAction( Amarok::actionCollection()->action( KStandardAction::name( KStandardAction::Quit ) ) );
#endif
    //END Actions menu

    //BEGIN View menu
    QMenu* viewMenu = new QMenu(this);
    addViewMenuItems(viewMenu);
    //END View menu

    //BEGIN Playlist menu
    KMenu *playlistMenu = new KMenu( m_menubar.data() );
    playlistMenu->setTitle( i18n("&Playlist") );
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_add") );
    playlistMenu->addAction( Amarok::actionCollection()->action("stream_add") );
    //playlistMenu->addAction( Amarok::actionCollection()->action("playlist_save") ); //FIXME: See FIXME in PlaylistDock.cpp
    playlistMenu->addAction( Amarok::actionCollection()->action( "playlist_export" ) );
    playlistMenu->addSeparator();
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_undo") );
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_redo") );
    playlistMenu->addSeparator();
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_clear") );
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_remove_dead_and_duplicates") );
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_layout") );
    //END Playlist menu

    //BEGIN Tools menu
    m_toolsMenu = new KMenu( m_menubar.data() );
    m_toolsMenu.data()->setTitle( i18n("&Tools") );

    m_toolsMenu.data()->addAction( Amarok::actionCollection()->action("bookmark_manager") );
    m_toolsMenu.data()->addAction( Amarok::actionCollection()->action("cover_manager") );
    m_toolsMenu.data()->addAction( Amarok::actionCollection()->action("equalizer_dialog") );
    m_toolsMenu.data()->addAction( Amarok::actionCollection()->action("script_manager") );
#ifdef DEBUG_BUILD_TYPE
    m_toolsMenu.data()->addAction( Amarok::actionCollection()->action("network_request_viewer") );
#endif // DEBUG_BUILD_TYPE
    m_toolsMenu.data()->addSeparator();
    m_toolsMenu.data()->addAction( Amarok::actionCollection()->action("update_collection") );
    //END Tools menu

    //BEGIN Settings menu
    m_settingsMenu = new KMenu( m_menubar.data() );
    m_settingsMenu.data()->setTitle( i18n("&Settings") );

    m_settingsMenu.data()->addAction( Amarok::actionCollection()->action( KStandardAction::name( KStandardAction::ShowMenubar ) ) );

    //TODO use KStandardAction or KXmlGuiWindow

    // the phonon-coreaudio  backend has major issues with either the VolumeFaderEffect itself
    // or with it in the pipeline. track playback stops every ~3-4 tracks, and on tracks >5min it
    // stops at about 5:40. while we get this resolved upstream, don't make playing amarok such on osx.
    // so we disable replaygain on osx

#ifndef Q_WS_MAC
    m_settingsMenu.data()->addAction( Amarok::actionCollection()->action("replay_gain_mode") );
    m_settingsMenu.data()->addSeparator();
#endif

    m_settingsMenu.data()->addAction( Amarok::actionCollection()->action( KStandardAction::name( KStandardAction::KeyBindings ) ) );
    m_settingsMenu.data()->addAction( Amarok::actionCollection()->action( KStandardAction::name( KStandardAction::Preferences ) ) );
    //END Settings menu

    m_menubar.data()->addMenu( actionsMenu );
    m_menubar.data()->addMenu( viewMenu );
    m_menubar.data()->addMenu( playlistMenu );
    m_menubar.data()->addMenu( m_toolsMenu.data() );
    m_menubar.data()->addMenu( m_settingsMenu.data() );

    KMenu *helpMenu = Amarok::Menu::helpMenu();
    helpMenu->insertAction( helpMenu->actions().first(),
                            Amarok::actionCollection()->action( "reportBug" ) );
    helpMenu->insertAction( helpMenu->actions().last(),
                            Amarok::actionCollection()->action( "extendedAbout" ) );
    helpMenu->insertAction( helpMenu->actions().at( 4 ),
                            Amarok::actionCollection()->action( "likeBackSendComment" ) );
    helpMenu->insertAction( helpMenu->actions().at( 5 ),
                            Amarok::actionCollection()->action( "likeBackShowIcons" ) );

    m_menubar.data()->addMenu( helpMenu );
}

void
MainWindow::slotShowMenuBar()
{
    if (!m_showMenuBar->isChecked())
    {
        //User have chosen to hide a menu. Lets warn him
        if (KMessageBox::warningContinueCancel(this,
            i18n("You have chosen to hide the menu bar.\n\nPlease remember that you can always use the shortcut \"%1\" to bring it back.")
                .arg(m_showMenuBar->shortcut().toString()),
            i18n("Hide Menu"), KStandardGuiItem::cont(), KStandardGuiItem::cancel(), "showMenubar") != KMessageBox::Continue)
        {
            //Cancel menu hiding. Revert menu item to checked state.
            m_showMenuBar->setChecked(true);
            return;
        }
    }
    menuBar()->setVisible(m_showMenuBar->isChecked());
}

void
MainWindow::showAbout()
{
    ExtendedAboutDialog dialog( &aboutData, &ocsData );
    dialog.exec();
}

void
MainWindow::showReportBug()
{
    KBugReport * rbDialog = new KBugReport( this, true, KGlobal::mainComponent().aboutData() );
    rbDialog->setObjectName( "KBugReport" );
    rbDialog->exec();
}

void
MainWindow::paletteChange( const QPalette & oldPalette )
{
    Q_UNUSED( oldPalette )

    KPixmapCache cache( "Amarok-pixmaps" );
    cache.discard();
    The::paletteHandler()->setPalette( palette() );
}

QSize
MainWindow::backgroundSize() const
{
    const QPoint topLeft = mapToGlobal( QPoint( 0, 0 ) );
    const QPoint bottomRight1 = mapToGlobal( QPoint( width(), height() ) );

    return QSize( bottomRight1.x() - topLeft.x() + 1, bottomRight1.y() - topLeft.y() );
}

//BEGIN DOCK LAYOUT FIXING HACK ====================================================================

/**
    Docks that are either hidden, floating or tabbed shall not be taken into account for ratio calculations
    unfortunately docks that are hiddeen in tabs are not "hidden", but just have an invalid geometry
*/
bool
MainWindow::LH_isIrrelevant( const QDockWidget *dock )
{
    bool ret = !dock->isVisibleTo( this ) || dock->isFloating();
    if( !ret )
    {
        const QRect r = dock->geometry();
        ret = r.right() < 1 || r.bottom() < 1;
    }
    return  ret;
}

/**
    We've atm three dock, ie. 0 or 1 tabbar. It's also the only bar as direct child.
    We need to dig it as it constructed and deleted on UI changes by the user
*/
QTabBar *
MainWindow::LH_dockingTabbar()
{
    QObjectList kids = children();
    foreach ( QObject *kid, kids )
    {
        if( kid->isWidgetType() )
            if( QTabBar *bar = qobject_cast<QTabBar*>( kid ) )
                return bar;
    }
    return 0;
}

/**
    this function calculates the area covered by the docks - i.e. rect() minus menubar and toolbar
*/
void
MainWindow::LH_extend( QRect &target, const QDockWidget *dock )
{
    if( LH_isIrrelevant( dock ) )
        return;
    if( target.isNull() )
    {
        target = dock->geometry();
        return;
    }
    const QRect source = dock->geometry();
    if( source.left() < target.left() ) target.setLeft( source.left() );
    if( source.right() > target.right() ) target.setRight( source.right() );
    if( source.top() < target.top() ) target.setTop( source.top() );
    if( source.bottom() > target.bottom() ) target.setBottom( source.bottom() );
}

/**
    which size the dock should have in our opinion, based upon the ratios and the docking area.
    takes size constraints into account
*/
QSize
MainWindow::LH_desiredSize( QDockWidget *dock, const QRect &area, float rx, float ry, int splitter )
{
    if( LH_isIrrelevant( dock ) )
        return QSize( 0, 0 );
    QSize ret;
    int tabHeight = 0;
    if( !tabifiedDockWidgets( dock ).isEmpty() )
    {
        if( QTabBar *bar = LH_dockingTabbar() )
            tabHeight = bar->height();
    }
    const QSize min = dock->minimumSize();
    ret.setWidth( qMax( min.width(), (int)( rx == 1.0 ? area.width() : rx*area.width() - splitter/2 ) ) );
    ret.setHeight( qMax( min.height(), (int)( ry == 1.0 ? area.height() - tabHeight : ry*area.height() - ( splitter/2 + tabHeight ) ) ) );
    return ret;
}

/**
    used to check whether the current dock size "more or less" matches our opinion
    if one of the sizes isNull() it's invalid and the current size is ok.
*/
bool
MainWindow::LH_fuzzyMatch( const QSize &sz1, const QSize &sz2 )
{
    return sz1.isNull() || sz2.isNull() ||
           ( sz1.width() < sz2.width() + 6 && sz1.width() > sz2.width() - 6 &&
             sz1.height() < sz2.height() + 6 && sz1.height() > sz2.height() - 6 );
}

/**
    whether the dock has reached it's minimum width OR height
*/
bool
MainWindow::LH_isConstrained( const QDockWidget *dock )
{
    if( LH_isIrrelevant( dock ) )
        return false;
    return dock->minimumWidth() == dock->width() || dock->minimumHeight() == dock->height();
}

#define FREE_SIZE(_DOCK_,_INDEX_)   if( !_DOCK_->isFloating() ) { \
                                        _DOCK_->setMinimumSize( mins[_INDEX_] );\
                                        _DOCK_->setMaximumSize( maxs[_INDEX_] ); } //

void
MainWindow::initLayoutHack()
{
    // the init _should_ be superflous, but hey: why risk it ;-)
    m_playlistRatio.x = 0.0; m_playlistRatio.y = 0.0;
    m_browserRatio = m_contextRatio = m_playlistRatio;

    m_dockingRect = QRect();
    LH_extend( m_dockingRect, m_browserDock.data() );
    LH_extend( m_dockingRect, m_contextDock.data() );
    LH_extend( m_dockingRect, m_playlistDock.data() );

    // initially calculate ratios
    updateDockRatio( m_browserDock.data() );
    updateDockRatio( m_contextDock.data() );
    updateDockRatio( m_playlistDock.data() );

    m_LH_initialized = true;

    // connect to dock changes (esp. tab selection)
    // this _is_ required...
    connect ( m_browserDock.data(), SIGNAL( visibilityChanged( bool ) ), this, SLOT( updateDockRatio() ) );
    connect ( m_contextDock.data(), SIGNAL( visibilityChanged( bool ) ), this, SLOT( updateDockRatio() ) );
    connect ( m_playlistDock.data(), SIGNAL( visibilityChanged( bool ) ), this, SLOT( updateDockRatio() ) );

    // but i think these can be omitted (move along a show event for tech. reasons)
//     topLevelChanged( bool )
//     dockLocationChanged(Qt::DockWidgetArea)

    slotShowActiveTrack();    // See comment in 'playlist/view/PrettyListView.cpp constructor'.
}

void
MainWindow::resizeEvent( QResizeEvent * event )
{
    DEBUG_BLOCK
    // NOTICE: uncomment this to test default behaviour
//     QMainWindow::resizeEvent( event );
//     return;

    // reset timer for saving changes
    // 30 secs - this is no more crucial and we don't have to store every sh** ;-)
    m_saveLayoutChangesTimer->start( 30000 );

    // prevent flicker, NOTICE to prevent all flicker, this must be done by an (bug prone) passing-on eventfiler :-(
    setUpdatesEnabled( false );

    // stop listening to resize events, we're gonna trigger them now
    m_browserDock.data()->removeEventFilter( this );
    m_contextDock.data()->removeEventFilter( this );
    m_playlistDock.data()->removeEventFilter( this );

    QMainWindow::resizeEvent( event );

    // ok, the window was resized and our little docklings fill the entire docking area
    // -> m_dockingRect is their bounding rect
    m_dockingRect = QRect();
    LH_extend( m_dockingRect, m_browserDock.data() );
    LH_extend( m_dockingRect, m_contextDock.data() );
    LH_extend( m_dockingRect, m_playlistDock.data() );

    // if we hit a minimumSize constraint, we can no more keep aspects at all atm. and just hope
    // that Qt distributed the lacking space evenly ;-)
    if( !m_LH_initialized ||
        LH_isConstrained( m_contextDock.data() ) || LH_isConstrained( m_browserDock.data() ) || LH_isConstrained( m_playlistDock.data() ) )
    {
        setUpdatesEnabled( true );

        // continue to listen to resize events
        m_browserDock.data()->installEventFilter( this );
        m_contextDock.data()->installEventFilter( this );
        m_playlistDock.data()->installEventFilter( this );

        return;
    }

#define DESIRED_SIZE(_DOCK_) LH_desiredSize( _DOCK_##Dock.data(), m_dockingRect, _DOCK_##Ratio.x, _DOCK_##Ratio.y, splitterSize )
    int splitterSize = style()->pixelMetric( QStyle::PM_DockWidgetSeparatorExtent, 0, 0 );
    const QSize dContextSz = DESIRED_SIZE( m_context );
    const QSize dBrowsersSz = DESIRED_SIZE( m_browser );
    const QSize dPlaylistSz = DESIRED_SIZE( m_playlist );
#undef DESIRED_SIZE

    // good god: prevent recursive resizes ;-)
    setFixedSize( size() );

    layout()->setEnabled( false );
    QCoreApplication::sendPostedEvents( this, QEvent::LayoutRequest );
    layout()->invalidate();
    layout()->setEnabled( true );

    // don't be super picky - there's an integer imprecision anyway and we just want to fix
    // major resize errors w/o being to intrusive or flickering
    if( !( LH_fuzzyMatch( dContextSz, m_contextDock.data()->size() ) &&
           LH_fuzzyMatch( dBrowsersSz, m_browserDock.data()->size() ) &&
           LH_fuzzyMatch( dPlaylistSz, m_playlistDock.data()->size() ) ) )
    {
//         debug() << "SIZE MISMATCH, FORCING";
        const QSize mins[3] = { m_contextDock.data()->minimumSize(), m_browserDock.data()->minimumSize(), m_playlistDock.data()->minimumSize() };
        const QSize maxs[3] = { m_contextDock.data()->maximumSize(), m_browserDock.data()->maximumSize(), m_playlistDock.data()->maximumSize() };

        // fix sizes to our idea, so the layout has few options left ;-)
        if( !m_contextDock.data()->isFloating() ) m_contextDock.data()->setFixedSize( dContextSz );
        if( !m_browserDock.data()->isFloating() ) m_browserDock.data()->setFixedSize( dBrowsersSz );
        if( !m_playlistDock.data()->isFloating() ) m_playlistDock.data()->setFixedSize( dPlaylistSz );

        // trigger a no-choice layouting
        layout()->activate();
        QCoreApplication::sendPostedEvents( this, QEvent::LayoutRequest );

        // unleash sizes
        layout()->setEnabled( false );
        FREE_SIZE( m_contextDock.data(), 0 );
        FREE_SIZE( m_browserDock.data(), 1 );
        FREE_SIZE( m_playlistDock.data(), 2 );
        layout()->setEnabled( true );
    }

    // unleash size
    setMinimumSize( 0, 0 );
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

    // continue to listen to resize events
    m_browserDock.data()->installEventFilter( this );
    m_contextDock.data()->installEventFilter( this );
    m_playlistDock.data()->installEventFilter( this );

    // update on screen
    setUpdatesEnabled( true );
}

#undef FREE_SIZE

// i hate these slots - but we get no show/hide events on tab selection :-(
void MainWindow::updateDockRatio( QDockWidget *dock )
{
    if( !LH_isIrrelevant( dock ) )
    {
        QRect area = m_dockingRect;
        int tabHeight = 0;
        if( !tabifiedDockWidgets( dock ).isEmpty() )
        {
            if( QTabBar *bar = LH_dockingTabbar() )
                tabHeight = bar->height();
        }
        int splitterSize = style()->pixelMetric( QStyle::PM_DockWidgetSeparatorExtent, 0, dock );

        Ratio r;
        if( dock->width() == area.width() )
            r.x = 1.0;
        else
            r.x = ( (float)( dock->width() + splitterSize/2) ) / area.width();
        if( dock->height() == area.height() - tabHeight )
            r.y = 1.0;
        else
            r.y = ( (float)( dock->height() + splitterSize/2 + tabHeight ) ) / area.height();

        if( dock == m_browserDock.data() )
            m_browserRatio = r;
        else if( dock == m_contextDock.data() )
            m_contextRatio = r;
        else
            m_playlistRatio = r;
//         debug() << "==>" << r.x << r.y;
    }
    // UI changed -> trigger delayed storage
    m_saveLayoutChangesTimer->start( 30000 );
}

// this slot is connected to the visibilityChange event, fired on show/hide (in most cases... *sigh*) and tab changes
void MainWindow::updateDockRatio()
{
    if( QDockWidget *dock = qobject_cast<QDockWidget*>( sender() ) )
        updateDockRatio( dock );
}

bool MainWindow::eventFilter( QObject *o, QEvent *e )
{
    // NOTICE this _must_ be handled by an eventfilter, as otherwise the "spliters" eventfilter
    // will eat and we don't receive it
    if( o == this )
    {
        if( e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonRelease )
        {
            QMouseEvent *me = static_cast<QMouseEvent*>( e );
            if( me->button() == Qt::LeftButton )
                m_mouseDown = ( e->type() == QEvent::MouseButtonPress );
        }

        return false;


    }

    if( ( ( e->type() == QEvent::Resize && m_mouseDown ) || // only when resized by the splitter :)
             e->type() == QEvent::Show || e->type() == QEvent::Hide ) && // show/hide is _NOT_ sufficient for tab changes
        ( o == m_browserDock.data() || o == m_contextDock.data() || o == m_playlistDock.data() ) )
    {
        QDockWidget *dock = static_cast<QDockWidget*>( o );
//         if(e->type() == QEvent::Resize)
//             debug() << dock << dock->size() << m_dockingRect.size();
//         else
//             debug() << "other!" << dock << dock->size() << m_dockingRect.size();
        updateDockRatio( dock );
    }
    return false;
}

//END DOCK LAYOUT FIXING HACK ======================================================================

QPoint
MainWindow::globalBackgroundOffset()
{
    return menuBar()->mapToGlobal( QPoint( 0, 0 ) );
}

QRect
MainWindow::contextRectGlobal() const
{
    //debug() << "pos of context vidget within main window is: " << m_contextWidget->pos();
    //FIXME
    QPoint contextPos = mapToGlobal( m_contextDock.data()->pos() );
    return QRect( contextPos.x(), contextPos.y(), m_contextDock.data()->width(), m_contextDock.data()->height() );
}

void
MainWindow::slotStopped()
{
    setPlainCaption( i18n( AMAROK_CAPTION ) );
}

void
MainWindow::slotPaused()
{
    setPlainCaption( i18n( "Paused  ::  %1", QString( AMAROK_CAPTION ) ) );
}

void
MainWindow::slotNewTrackPlaying()
{
    slotMetadataChanged( The::engineController()->currentTrack() );
}

void
MainWindow::slotMetadataChanged( Meta::TrackPtr track )
{
    if( track && The::engineController()->currentTrack() == track )
        setPlainCaption( i18n( "%1 - %2  ::  %3", track->artist() ? track->artist()->prettyName() : i18n( "Unknown" ), track->prettyName(), AMAROK_CAPTION ) );
}

CollectionWidget *
MainWindow::collectionBrowser()
{
    return m_collectionBrowser;
}

QString
MainWindow::activeBrowserName()
{
    if( m_browserDock.data()->list()->activeCategory() )
        return m_browserDock.data()->list()->activeCategory()->name();
    else
        return QString();
}

PlaylistBrowserNS::PlaylistBrowser *
MainWindow::playlistBrowser()
{
    return m_playlistBrowser;
}

void
MainWindow::setLayoutLocked( bool locked )
{
    DEBUG_BLOCK

    if( locked )
    {
        m_browserDock.data()->setMovable( false );
        m_contextDock.data()->setMovable( false );
        m_playlistDock.data()->setMovable( false );

        m_slimToolbar.data()->setFloatable( false );
        m_slimToolbar.data()->setMovable( false );

        m_mainToolbar.data()->setFloatable( false );
        m_mainToolbar.data()->setMovable( false );
    }
    else
    {
        m_browserDock.data()->setMovable( true );
        m_contextDock.data()->setMovable( true );
        m_playlistDock.data()->setMovable( true );

        m_slimToolbar.data()->setFloatable( true );
        m_slimToolbar.data()->setMovable( true );

        m_mainToolbar.data()->setFloatable( true );
        m_mainToolbar.data()->setMovable( true );
    }

    AmarokConfig::setLockLayout( locked );
    AmarokConfig::self()->writeConfig();
    m_layoutLocked = locked;
}

bool
MainWindow::isLayoutLocked() const
{
    return m_layoutLocked;
}

void
MainWindow::restoreLayout()
{
    DEBUG_BLOCK

    // Do not restore the layout if the main window is hidden
    // Qt takes widgets out of the layout if they're not visible.
    // So this is not going to work. Also see bug 244583
    if (!isVisible())
        return;

    QFile file( Amarok::saveLocation() + "layout" );
    QByteArray layout;
    if( file.open( QIODevice::ReadOnly ) )
    {
        layout = file.readAll();
        file.close();
    }

    if( !restoreState( layout, LAYOUT_VERSION ) )
    {
        //since no layout has been loaded, we know that the items are all placed next to each other in the main window
        //so get the combined size of the widgets, as this is the space we have to play with. Then figure out
        //how much to give to each. Give the context view any pixels leftover from the integer division.

        //int totalWidgetWidth = m_browsersDock->width() + m_contextView->width() + m_playlistDock->width();
        int totalWidgetWidth = contentsRect().width();

        //get the width of the splitter handles, we need to subtract these...
        const int splitterHandleWidth = style()->pixelMetric( QStyle::PM_DockWidgetSeparatorExtent, 0, 0 );
        debug() << "splitter handle widths " << splitterHandleWidth;

        totalWidgetWidth -= ( splitterHandleWidth * 2 );

        debug() << "mainwindow width" <<  contentsRect().width();
        debug() << "totalWidgetWidth" <<  totalWidgetWidth;

        const int widgetWidth = totalWidgetWidth / 3;
        const int leftover = totalWidgetWidth - 3*widgetWidth;

        //We need to set fixed widths initially, just until the main window has been properly layed out. As soon as this has
        //happened, we will unlock these sizes again so that the elements can be resized by the user.
        const int mins[3] = { m_browserDock.data()->minimumWidth(), m_contextDock.data()->minimumWidth(), m_playlistDock.data()->minimumWidth() };
        const int maxs[3] = { m_browserDock.data()->maximumWidth(), m_contextDock.data()->maximumWidth(), m_playlistDock.data()->maximumWidth() };

        m_browserDock.data()->setFixedWidth( widgetWidth );
        m_contextDock.data()->setFixedWidth( widgetWidth + leftover );
        m_playlistDock.data()->setFixedWidth( widgetWidth );
        this->layout()->activate();

        m_browserDock.data()->setMinimumWidth( mins[0] ); m_browserDock.data()->setMaximumWidth( maxs[0] );
        m_contextDock.data()->setMinimumWidth( mins[1] ); m_contextDock.data()->setMaximumWidth( maxs[1] );
        m_playlistDock.data()->setMinimumWidth( mins[2] ); m_playlistDock.data()->setMaximumWidth( maxs[2] );
    }

    // Ensure that only one toolbar is visible
    if( !m_mainToolbar.data()->isHidden() && !m_slimToolbar.data()->isHidden() )
        m_slimToolbar.data()->hide();

    // Ensure that we don't end up without any toolbar (can happen after upgrading)
    if( m_mainToolbar.data()->isHidden() && m_slimToolbar.data()->isHidden() )
        m_mainToolbar.data()->show();

    m_layoutEverRestored = true;
}


bool
MainWindow::playAudioCd()
{
    DEBUG_BLOCK
    //drop whatever we are doing and play auidocd

    QList<Collections::Collection*> collections = CollectionManager::instance()->viewableCollections();

    foreach( Collections::Collection *collection, collections )
    {
        if( collection->collectionId() == "AudioCd" )
        {

            debug() << "got audiocd collection";

            Collections::MemoryCollection * cdColl = dynamic_cast<Collections::MemoryCollection *>( collection );

            if( !cdColl || cdColl->trackMap().count() == 0 )
            {
                debug() << "cd collection not ready yet (track count = 0 )";
                m_waitingForCd = true;
                return false;
            }

            The::engineController()->stop( true );
            The::playlistController()->clear();

            Collections::QueryMaker * qm = collection->queryMaker();
            qm->setQueryType( Collections::QueryMaker::Track );
            The::playlistController()->insertOptioned( qm, Playlist::DirectPlay );

            m_waitingForCd = false;
            return true;
        }
    }

    debug() << "waiting for cd...";
    m_waitingForCd = true;
    return false;
}

bool
MainWindow::isWaitingForCd() const
{
    DEBUG_BLOCK
    debug() << "waiting?: " << m_waitingForCd;
    return m_waitingForCd;
}

void
MainWindow::resizeWindowHack()
{
    // HACK
    // This code works around a bug in KDE 4.5, which causes our Plasma applets to show
    // with a wrong initial size. Remove when this bug is fixed in Plasma.
    resize( width(), height() - 1 );
    resize( width(), height() + 1 );
}

#include "MainWindow.moc"
