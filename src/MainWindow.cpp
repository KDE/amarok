/****************************************************************************************
 * Copyright (c) 2002-2009 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2002 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2002 Gabor Lehel <illissius@gmail.com>                                 *
 * Copyright (c) 2002 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Artur Szymiec <artur.szymiec@gmail.com>                           *
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

#include <config-amarok.h>    //HAVE_LIBVISUAL definition

#include "aboutdialog/ExtendedAboutDialog.h"
#include "ActionClasses.h"
#include "Amarok.h"
#include "Debug.h"
#include "EngineController.h" //for actions in ctor
#include "MainToolbar.h"
#include "Osd.h"
#include "PaletteHandler.h"
#include "ScriptManager.h"
#include "SearchWidget.h"
#include "amarokconfig.h"
#include "aboutdialog/OcsData.h"
#include "amarokurls/AmarokUrlHandler.h"
#include "amarokurls/BookmarkManager.h"
#include "browsers/collectionbrowser/CollectionWidget.h"
#include "browsers/filebrowser/FileBrowser.h"
#include "browsers/playlistbrowser/PlaylistBrowser.h"
#include "browsers/servicebrowser/ServiceBrowser.h"
#include "collection/CollectionManager.h"
#include "context/ContextScene.h"
#include "context/ContextView.h"
#include "context/ToolbarView.h"
#include "covermanager/CoverManager.h" // for actions
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModelStack.h"
#include "playlist/PlaylistWidget.h"
#include "playlist/ProgressiveSearchWidget.h"
#include "playlistmanager/file/PlaylistFileProvider.h"
#include "playlistmanager/PlaylistManager.h"
#include "services/ServicePluginManager.h"
#include "services/scriptable/ScriptableService.h"
#include "statusbar/StatusBar.h"
#include "toolbar/MainToolbarNG.h"
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
#include <QSysInfo>
#include <QVBoxLayout>

#ifdef Q_WS_X11
#include <fixx11h.h>
#endif

#ifdef Q_WS_MAC
#include "mac/GrowlInterface.h"
#endif

// Let people know OS X and Windows versions are still work-in-progress
#if defined(Q_WS_MAC) || defined(Q_WS_WIN)
#define AMAROK_CAPTION "Amarok 2 beta"
#else
#define AMAROK_CAPTION "Amarok"
#endif

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

QPointer<MainWindow> MainWindow::s_instance = 0;

MainWindow::MainWindow()
    : KMainWindow( 0 )
    , EngineObserver( The::engineController() )
    , m_lastBrowser( 0 )
    , m_dockWidthsLocked( false )
{
    DEBUG_BLOCK

    setObjectName( "MainWindow" );
    s_instance = this;

#ifdef Q_WS_MAC
    QSizeGrip* grip = new QSizeGrip( this );
    GrowlInterface* growl = new GrowlInterface( qApp->applicationName() );
#endif

    StatusBar * statusBar = new StatusBar( this );

    setStatusBar( statusBar );

    // Sets caption and icon correctly (needed e.g. for GNOME)
    kapp->setTopWidget( this );
    PERF_LOG( "Set Top Widget" )
    createActions();
    PERF_LOG( "Created actions" )

    //new K3bExporter();

    KConfigGroup config = Amarok::config();
    const QSize size = config.readEntry( "MainWindow Size", QSize() );
    const QPoint pos = config.readEntry( "MainWindow Position", QPoint() );
    if( size.isValid() )
    {
        resize( size );
        move( pos );
    }

    The::paletteHandler()->setPalette( palette() );
    setPlainCaption( i18n( AMAROK_CAPTION ) );

    init();  // We could as well move the code from init() here, but meh.. getting a tad long

    //restore active category ( as well as filters and levels and whatnot.. )
    const QString path = config.readEntry( "Browser Path", QString() );
    if ( !path.isEmpty() )
        browserWidget()->list()->navigate( path );

}

MainWindow::~MainWindow()
{
    DEBUG_BLOCK

    KConfigGroup config = Amarok::config();
    config.writeEntry( "MainWindow Size", size() );
    config.writeEntry( "MainWindow Position", pos() );

    //save currently active category

    config.writeEntry( "Browser Path", browserWidget()->list()->path() );


    QList<int> sPanels;

    //foreach( int a, m_splitter->saveState() )
    //    sPanels.append( a );


    //save layout to file. Does not go into to rc as it is binary data.
    QFile file( Amarok::saveLocation() + "layout" );
    if ( file.open( QIODevice::ReadWrite | QIODevice::Unbuffered | QIODevice::Truncate ) )
    {
        file.write( saveState( LAYOUT_VERSION ) );
        file.close();
    }

    //AmarokConfig::setPanelsSavedState( sPanels );

    delete m_browserDummyTitleBarWidget;
    delete m_contextDummyTitleBarWidget;
    delete m_playlistDummyTitleBarWidget;

    delete m_contextView;
    delete m_corona;
    //delete m_splitter;
    delete The::statusBar();
    delete m_controlBar;
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

    m_controlBar = new MainToolbar( 0 );
    m_controlBar->layout()->setContentsMargins( 0, 0, 0, 0 );
    m_controlBar->layout()->setSpacing( 0 );
    m_controlBar->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
    m_controlBar->setMovable ( true );

    addToolBar( Qt::TopToolBarArea, m_controlBar );


    m_newToolbar = new MainToolbarNG( 0 );
    //newToolbar->setAllowedAreas( Qt::AllToolBarAreas );
    m_newToolbar->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
    m_newToolbar->setMovable ( true );

    addToolBar( Qt::TopToolBarArea, m_newToolbar );
    m_newToolbar->hide();


    PERF_LOG( "Create sidebar" )
    m_browsers = new BrowserWidget( this );
    m_browsers->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );

    m_browserDummyTitleBarWidget = new QWidget();
    m_contextDummyTitleBarWidget = new QWidget();
    m_playlistDummyTitleBarWidget = new QWidget();

    m_browsersDock = new QDockWidget( i18n( "Content" ), this );
    m_browsersDock->setObjectName( "Content dock" );
    m_browsersDock->setWidget( m_browsers );
    m_browsersDock->setAllowedAreas( Qt::AllDockWidgetAreas );


    PERF_LOG( "Create Playlist" )
    m_playlistWidget = new Playlist::Widget( 0 );
    m_playlistWidget->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );
    m_playlistWidget->setFocus( Qt::ActiveWindowFocusReason );

    m_playlistDock = new QDockWidget( i18n( "Playlist" ), this );
    m_playlistDock->setObjectName( "Playlist dock" );
    m_playlistDock->setWidget( m_playlistWidget );
    m_playlistDock->setAllowedAreas( Qt::AllDockWidgetAreas );

    PERF_LOG( "Playlist created" )

    createMenus();

    PERF_LOG( "Creating ContextWidget" )
    m_contextWidget = new ContextWidget( this );
    PERF_LOG( "ContextWidget created" )
    m_contextWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_contextWidget->setSpacing( 0 );
    m_contextWidget->setFrameShape( QFrame::NoFrame );
    m_contextWidget->setFrameShadow( QFrame::Sunken );
    m_contextWidget->setMinimumSize( 100, 100 );
    PERF_LOG( "Creating ContexScene" )

    m_corona = new Context::ContextScene( this );
    connect( m_corona, SIGNAL( containmentAdded( Plasma::Containment* ) ),
            this, SLOT( createContextView( Plasma::Containment* ) ) );

    m_contextDock = new QDockWidget( i18n( "Context" ), this );
    m_contextDock->setObjectName( "Context dock" );
    m_contextDock->setWidget( m_contextWidget );
    m_contextDock->setAllowedAreas( Qt::AllDockWidgetAreas );

    PERF_LOG( "ContextScene created" )

    PERF_LOG( "Loading default contextScene" )
    m_corona->loadDefaultSetup(); // this method adds our containment to the scene
    PERF_LOG( "Loaded default contextScene" )

    setDockOptions ( QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::AnimatedDocks );

    addDockWidget( Qt::LeftDockWidgetArea, m_browsersDock );
    addDockWidget( Qt::LeftDockWidgetArea, m_contextDock, Qt::Horizontal );
    addDockWidget( Qt::RightDockWidgetArea, m_playlistDock );

    setLayoutLocked( AmarokConfig::lockLayout() );

    //<Browsers>
    {
        Debug::Block block( "Creating browsers. Please report long start times!" );

        PERF_LOG( "Creating CollectionWidget" )
        m_collectionBrowser = new CollectionWidget( "collections", 0 );
        m_collectionBrowser->setPrettyName( i18n( "Local Music" ) );
        m_collectionBrowser->setIcon( KIcon( "collection-amarok" ) );
        m_collectionBrowser->setShortDescription( i18n( "Local sources of content" ) );
        m_browsers->list()->addCategory( m_collectionBrowser );
        PERF_LOG( "Created CollectionWidget" )


        PERF_LOG( "Creating ServiceBrowser" )
        ServiceBrowser *internetContentServiceBrowser = ServiceBrowser::instance();
        internetContentServiceBrowser->setParent( 0 );
        internetContentServiceBrowser->setPrettyName( i18n( "Internet" ) );
        internetContentServiceBrowser->setIcon( KIcon( "applications-internet" ) );
        internetContentServiceBrowser->setShortDescription( i18n( "Online sources of content" ) );
        m_browsers->list()->addCategory( internetContentServiceBrowser );
        PERF_LOG( "Created ServiceBrowser" )

        PERF_LOG( "Creating PlaylistBrowser" )
        m_playlistBrowser = new PlaylistBrowserNS::PlaylistBrowser( "playlists", 0 );
        m_playlistBrowser->setPrettyName( i18n("Playlists") );
        m_playlistBrowser->setIcon( KIcon( "view-media-playlist-amarok" ) );
        m_playlistBrowser->setShortDescription( i18n( "Various types of playlists" ) );
        m_browsers->list()->addCategory( m_playlistBrowser );
        PERF_LOG( "CreatedPlaylsitBrowser" )


        PERF_LOG( "Creating FileBrowser" )
        FileBrowser::Widget * fileBrowser = new FileBrowser::Widget( "files", 0 );
        fileBrowser->setPrettyName( i18n("Files") );
        fileBrowser->setIcon( KIcon( "folder-amarok" ) );
        fileBrowser->setShortDescription( i18n( "Browse local hard drive for content" ) );
        m_browsers->list()->addCategory( fileBrowser );
        PERF_LOG( "Created FileBrowser" )

        PERF_LOG( "Initialising ServicePluginManager" )
        ServicePluginManager::instance()->init();
        PERF_LOG( "Initialised ServicePluginManager" )

        internetContentServiceBrowser->setScriptableServiceManager( The::scriptableServiceManager() );
        PERF_LOG( "ScriptableServiceManager done" )

        PERF_LOG( "finished MainWindow::init" )
    }

    The::amarokUrlHandler(); //Instantiate

    //restore the layout
    restoreLayout();

}

void
MainWindow::createContextView( Plasma::Containment *containment )
{
    DEBUG_BLOCK
    disconnect( m_corona, SIGNAL( containmentAdded( Plasma::Containment* ) ),
            this, SLOT( createContextView( Plasma::Containment* ) ) );
    PERF_LOG( "Creating ContexView" )
    m_contextView = new Context::ContextView( containment, m_corona, m_contextWidget );
    m_contextView->setFrameShape( QFrame::NoFrame );
    m_contextToolbarView = new Context::ToolbarView( containment, m_corona, m_contextWidget );
    m_contextToolbarView->setFrameShape( QFrame::NoFrame );
    m_contextView->showHome();
    PERF_LOG( "ContexView created" )

    bool hide = AmarokConfig::hideContextView();
    hideContextView( hide );
}

QMenu*
MainWindow::createPopupMenu()
{
    QMenu* menu = new QMenu( this );

    // Layout locking:
    QAction* lockAction = new QAction( i18n( "Lock layout" ), this );
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
            menu->addAction( dockWidget->toggleViewAction());
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

    return menu;
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
    //if( index >= 0 && index != m_browsers->currentIndex() )
    //    m_browsers->showWidget( index );
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
    if( n == 0 && m_browsers->currentIndex() >= 0 )
        m_browsers->showWidget( m_browsers->currentIndex() );
    else if( n > 0 )
        showBrowser( n - 1 ); // map from human to computer counting*/
}

void
MainWindow::closeEvent( QCloseEvent *e )
{
    DEBUG_BLOCK

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

QSize
MainWindow::sizeHint() const
{
    return QApplication::desktop()->screenGeometry( (QWidget*)this ).size() / 1.5;
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

    fileDialog.exec();

    QString playlistPath = fileDialog.selectedFile();

    if( !playlistPath.isEmpty() )
    {
        AmarokConfig::setRelativePlaylist( saveRelativeCheck->isChecked() );
        The::playlist()->exportPlaylist( playlistPath );
    }
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

void
MainWindow::slotPlayMedia() //SLOT
{
    // Request location and immediately start playback
    slotAddLocation( true );
}

void
MainWindow::slotAddLocation( bool directPlay ) //SLOT
{
    // open a file selector to add media to the playlist
    KUrl::List files;
    KFileDialog dlg( KUrl(QDesktopServices::storageLocation(QDesktopServices::MusicLocation)), QString("*.*|"), this );
    dlg.setCaption( directPlay ? i18n("Play Media (Files or URLs)") : i18n("Add Media (Files or URLs)") );
    dlg.setMode( KFile::Files | KFile::Directory );
    dlg.exec();
    files = dlg.selectedUrls();

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

    m_playlistWidget->searchWidget()->focusInputLine();
}

void
MainWindow::showScriptSelector() //SLOT
{
    ScriptManager::instance()->show();
    ScriptManager::instance()->raise();
}

/**
 * "Toggle Main Window" global shortcut connects to this slot
 */
void
MainWindow::showHide() //SLOT
{
    setVisible( !isVisible() );
}

void
MainWindow::activate()
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

bool
MainWindow::isReallyShown() const
{
#ifdef Q_WS_X11
    const KWindowInfo info = KWindowSystem::windowInfo( winId(), 0, 0 );
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
    KStandardAction::configureNotifications( kapp, SLOT( slotConfigNotifications() ), ac );
    KStandardAction::preferences( kapp, SLOT( slotConfigAmarok() ), ac );
    ac->action(KStandardAction::name(KStandardAction::KeyBindings))->setIcon( KIcon( "configure-shortcuts-amarok" ) );
    ac->action(KStandardAction::name(KStandardAction::ConfigureNotifications) );
    ac->action(KStandardAction::name(KStandardAction::Preferences))->setIcon( KIcon( "configure-amarok" ) );
    ac->action(KStandardAction::name(KStandardAction::Preferences))->setMenuRole(QAction::PreferencesRole); // Define OS X Prefs menu here, removes need for ifdef later

    KStandardAction::quit( kapp, SLOT( quit() ), ac );

    KAction *action = new KAction( KIcon( "folder-amarok" ), i18n("&Add Media..."), this );
    ac->addAction( "playlist_add", action );
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( slotAddLocation() ) );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_A ) );

    action = new KAction( KIcon( "edit-clear-list-amarok" ), i18nc( "clear playlist", "&Clear Playlist" ), this );
    connect( action, SIGNAL( triggered( bool ) ), pc, SLOT( clear() ) );
    ac->addAction( "playlist_clear", action );

    action = new KAction( i18nc( "Remove duplicate and dead (unplayable) tracks from the playlist", "Re&move Duplicates" ), this );
    connect( action, SIGNAL( triggered( bool ) ), pc, SLOT( removeDeadAndDuplicates() ) );
    ac->addAction( "playlist_remove_dead_and_duplicates", action );

    action = new KAction( KIcon( "folder-amarok" ), i18n("&Add Stream..."), this );
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( slotAddStream() ) );
    ac->addAction( "stream_add", action );

    action = new KAction( KIcon( "document-export-amarok" ), i18n("&Export Playlist As..."), this );
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( exportPlaylist() ) );
    ac->addAction( "playlist_export", action );

    action = new KAction( KIcon( "bookmark-new" ), i18n( "Bookmark This Location" ), this );
    ac->addAction( "bookmark_browser", action );
    connect( action, SIGNAL( triggered() ), The::amarokUrlHandler(), SLOT( bookmarkCurrentBrowserView() ) );

    action = new KAction( KIcon( "bookmarks-organize" ), i18n( "Bookmark Manager" ), this );
    ac->addAction( "bookmark_manager", action );
    connect( action, SIGNAL( triggered(bool) ), SLOT( slotShowBookmarkManager() ) );

    action = new KAction( KIcon( "bookmark-new" ), i18n( "Bookmark This Location" ), this );
    ac->addAction( "bookmark_playlistview", action );
    connect( action, SIGNAL( triggered() ), The::amarokUrlHandler(), SLOT( bookmarkCurrentPlaylistView() ) );

    action = new KAction( KIcon( "bookmark-new" ), i18n( "Bookmark Context Applets" ), this );
    ac->addAction( "bookmark_contextview", action );
    connect( action, SIGNAL( triggered() ), The::amarokUrlHandler(), SLOT( bookmarkCurrentContextView() ) );

    action = new KAction( KIcon( "media-album-cover-manager-amarok" ), i18n( "Cover Manager" ), this );
    connect( action, SIGNAL( triggered(bool) ), SLOT( slotShowCoverManager() ) );
    ac->addAction( "cover_manager", action );

    action = new KAction( KIcon("folder-amarok"), i18n("Play Media..."), this );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotPlayMedia()));
    ac->addAction( "playlist_playmedia", action );

    action = new KAction( KIcon("preferences-plugin-script-amarok"), i18n("Script Manager"), this );
    connect(action, SIGNAL(triggered(bool)), SLOT(showScriptSelector()));
    ac->addAction( "script_manager", action );

    action = new KAction( KIcon( "media-seek-forward-amarok" ), i18n("&Seek Forward"), this );
    ac->addAction( "seek_forward", action );
    action->setShortcut( Qt::Key_Right );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::SHIFT + Qt::Key_Plus ) );
    connect(action, SIGNAL(triggered(bool)), ec, SLOT(seekForward()));

    action = new KAction( KIcon( "media-seek-backward-amarok" ), i18n("&Seek Backward"), this );
    ac->addAction( "seek_backward", action );
    action->setShortcut( Qt::Key_Left );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::SHIFT + Qt::Key_Minus ) );
    connect(action, SIGNAL(triggered(bool)), ec, SLOT(seekBackward()));

    PERF_LOG( "MainWindow::createActions 6" )
    action = new KAction( KIcon("collection-refresh-amarok"), i18n( "Update Collection" ), this );
    connect(action, SIGNAL(triggered(bool)), CollectionManager::instance(), SLOT(checkCollectionChanges()));
    ac->addAction( "update_collection", action );

    action = new KAction( this );
    ac->addAction( "prev", action );
    action->setIcon( KIcon("media-skip-backward-amarok") );
    action->setText( i18n( "Previous Track" ) );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_Z ) );
    connect( action, SIGNAL(triggered(bool)), pa, SLOT( back() ) );

    action = new KAction( this );
    ac->addAction( "next", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_B ) );
    action->setIcon( KIcon("media-skip-forward-amarok") );
    action->setText( i18n( "Next Track" ) );
    connect( action, SIGNAL(triggered(bool)), pa, SLOT( next() ) );

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
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_P ) );
    connect( action, SIGNAL( triggered() ), SLOT( showHide() ) );

    action = new KAction( i18n( "Jump to" ), this );
    ac->addAction( "jumpTo", action );
    action->setShortcut( KShortcut( Qt::CTRL + Qt::Key_J ) );
    connect( action, SIGNAL( triggered() ), SLOT( slotJumpTo() ) );

    action = new KAction( i18n( "Show On Screen Display" ), this );
    ac->addAction( "showOsd", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_O ) );
    connect( action, SIGNAL( triggered() ), Amarok::OSD::instance(), SLOT( forceToggleOSD() ) );

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

    action = KStandardAction::redo(pc, SLOT(redo()), this);
    ac->addAction( "playlist_redo", action );
    action->setEnabled(false);
    action->setIcon( KIcon( "edit-redo-amarok" ) );
    connect(pc, SIGNAL(canRedoChanged(bool)), action, SLOT(setEnabled(bool)));

    action = KStandardAction::undo(pc, SLOT(undo()), this);
    ac->addAction( "playlist_undo", action );
    action->setEnabled(false);
    action->setIcon( KIcon( "edit-undo-amarok" ) );
    connect(pc, SIGNAL(canUndoChanged(bool)), action, SLOT(setEnabled(bool)));

    PERF_LOG( "MainWindow::createActions 8" )
    new Amarok::MenuAction( ac, this );
    new Amarok::StopAction( ac, this );
    new Amarok::StopPlayingAfterCurrentTrackAction( ac, this );
    new Amarok::PlayPauseAction( ac, this );
    new Amarok::RepeatAction( ac, this );
    new Amarok::RandomAction( ac, this );
    new Amarok::FavorAction( ac, this );
    new Amarok::ReplayGainModeAction( ac, this );
    new Amarok::EqualizerAction( ac, this);

    ac->addAssociatedWidget( this );
    foreach (QAction* action, ac->actions())
        action->setShortcutContext(Qt::WindowShortcut);
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
    m_menubar = new QMenuBar(0);  // Fixes menubar in OS X
    actionsMenu = new KMenu( m_menubar );
    // Add these functions to the dock icon menu in OS X
    //extern void qt_mac_set_dock_menu(QMenu *);
    //qt_mac_set_dock_menu(actionsMenu);
    // Change to avoid duplicate menu titles in OS X
    actionsMenu->setTitle( i18n("&Music") );
#else
    m_menubar = menuBar();
    actionsMenu = new KMenu( m_menubar );
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
    actionsMenu->addAction( Amarok::actionCollection()->action(KStandardAction::name(KStandardAction::Quit)) );
#endif
    //END Actions menu

    //BEGIN Playlist menu
    KMenu *playlistMenu = new KMenu( m_menubar );
    playlistMenu->setTitle( i18n("&Playlist") );
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_add") );
    playlistMenu->addAction( Amarok::actionCollection()->action("stream_add") );
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_save") );
    playlistMenu->addAction( Amarok::actionCollection()->action( "playlist_export" ) );
    playlistMenu->addSeparator();
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_undo") );
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_redo") );
    playlistMenu->addSeparator();
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_clear") );
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_remove_dead_and_duplicates") );
    playlistMenu->addSeparator();

    QAction *repeat = Amarok::actionCollection()->action("repeat");
    playlistMenu->addAction( repeat );

    KSelectAction *random = static_cast<KSelectAction*>( Amarok::actionCollection()->action("random_mode") );
    playlistMenu->addAction( random );
    random->menu()->addSeparator();
    random->menu()->addAction( Amarok::actionCollection()->action("favor_tracks") );

    playlistMenu->addSeparator();
    //END Playlist menu

    //BEGIN Tools menu
    m_toolsMenu = new KMenu( m_menubar );
    m_toolsMenu->setTitle( i18n("&Tools") );

    m_toolsMenu->addAction( Amarok::actionCollection()->action("bookmark_manager") );
    m_toolsMenu->addAction( Amarok::actionCollection()->action("cover_manager") );
//FIXME: Reenable when ported//working
//     m_toolsMenu->addAction( Amarok::actionCollection()->action("queue_manager") );
    m_toolsMenu->addAction( Amarok::actionCollection()->action("script_manager") );
    m_toolsMenu->addSeparator();
    m_toolsMenu->addAction( Amarok::actionCollection()->action("update_collection") );
    //END Tools menu

    //BEGIN Settings menu
    m_settingsMenu = new KMenu( m_menubar );
    m_settingsMenu->setTitle( i18n("&Settings") );
    //TODO use KStandardAction or KXmlGuiWindow

    // the phonon-coreaudio  backend has major issues with either the VolumeFaderEffect itself
    // or with it in the pipeline. track playback stops every ~3-4 tracks, and on tracks >5min it
    // stops at about 5:40. while we get this resolved upstream, don't make playing amarok such on osx.
    // so we disable replaygain on osx


#ifndef Q_WS_MAC
    m_settingsMenu->addAction( Amarok::actionCollection()->action("replay_gain_mode") );
    m_settingsMenu->addSeparator();
#endif

    // Add equalizer action - a list with all equalizer presets available
    m_settingsMenu->addAction( Amarok::actionCollection()->action("equalizer_mode") );
    m_settingsMenu->addSeparator();

    m_settingsMenu->addAction( Amarok::actionCollection()->action(KStandardAction::name(KStandardAction::ConfigureNotifications)) );
    m_settingsMenu->addAction( Amarok::actionCollection()->action(KStandardAction::name(KStandardAction::KeyBindings)) );
    m_settingsMenu->addAction( Amarok::actionCollection()->action(KStandardAction::name(KStandardAction::Preferences)) );
    //END Settings menu

    m_menubar->addMenu( actionsMenu );
    m_menubar->addMenu( playlistMenu );
    m_menubar->addMenu( m_toolsMenu );
    m_menubar->addMenu( m_settingsMenu );
    KMenu *helpMenu = Amarok::Menu::helpMenu();
    m_menubar->addMenu( helpMenu );
}

void
MainWindow::showAbout()
{
    ExtendedAboutDialog *dialog = new ExtendedAboutDialog( &aboutData, &ocsData, this );
    dialog->exec();
    delete dialog;
}

void
MainWindow::paletteChange(const QPalette & oldPalette)
{
    Q_UNUSED( oldPalette )

    KPixmapCache cache( "Amarok-pixmaps" );
    cache.discard();
    The::paletteHandler()->setPalette( palette() );
}

QSize
MainWindow::backgroundSize()
{
    const QPoint topLeft = mapToGlobal( QPoint( 0, 0 ) );
    const QPoint bottomRight1 = mapToGlobal( QPoint( width(), height() ) );

    return QSize( bottomRight1.x() - topLeft.x() + 1, bottomRight1.y() - topLeft.y() );
}

int
MainWindow::contextXOffset()
{
    const QPoint topLeft1 = mapToGlobal( m_controlBar->pos() );
    const QPoint topLeft2 = mapToGlobal( m_contextWidget->pos() );

    return topLeft2.x() - topLeft1.x();
}

void
MainWindow::resizeEvent( QResizeEvent * event )
{
    QWidget::resizeEvent( event );
    m_controlBar->reRender();

    if ( m_dockWidthsLocked )
    {
        m_dockWidthsLocked = false;
        m_browsers->setMinimumWidth( 0 );
        m_contextWidget->setMinimumWidth( 0 );
        m_playlistWidget->setMinimumWidth( 0 );

        m_browsers->setMaximumWidth( 9999 );
        m_contextWidget->setMaximumWidth( 9999 );
        m_playlistWidget->setMaximumWidth( 9999 );
    }
}

QPoint
MainWindow::globalBackgroundOffset()
{
    return menuBar()->mapToGlobal( QPoint( 0, 0 ) );
}

QRect
MainWindow::contextRectGlobal()
{
    //debug() << "pos of context vidget within main window is: " << m_contextWidget->pos();
    QPoint contextPos = mapToGlobal( m_contextWidget->pos() );
    return QRect( contextPos.x(), contextPos.y(), m_contextWidget->width(), m_contextWidget->height() );
}

void
MainWindow::engineStateChanged( Phonon::State state, Phonon::State oldState )
{
    Q_UNUSED( oldState )
    DEBUG_BLOCK

    debug() << "Phonon state: " << state;

    Meta::TrackPtr track = The::engineController()->currentTrack();
    //track is 0 if the engine state is Empty. we check that in the switch
    switch( state )
    {
    case Phonon::StoppedState:
        setPlainCaption( i18n( AMAROK_CAPTION ) );
        break;

    case Phonon::PlayingState:
        if( track ) {
            unsubscribeFrom( m_currentTrack );
            m_currentTrack = track;
            subscribeTo( track );
            metadataChanged( track );
        }
        else
            warning() << "currentTrack is 0. Can't subscribe to it!";
        break;

    case Phonon::PausedState:
        setPlainCaption( i18n( "Paused  ::  %1", QString( AMAROK_CAPTION ) ) );
        break;

    case Phonon::LoadingState:
    case Phonon::ErrorState:
    case Phonon::BufferingState:
        break;
    }
}

void
MainWindow::engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged )
{
    Q_UNUSED( newMetaData )
    Q_UNUSED( trackChanged )

    Meta::TrackPtr track = The::engineController()->currentTrack();
    if ( track )
        metadataChanged( track );
}

void
MainWindow::metadataChanged( Meta::TrackPtr track )
{
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
    if ( m_browsers->list()->activeCategory() )
        return m_browsers->list()->activeCategory()->name();
    else
        return QString();
}

PlaylistBrowserNS::PlaylistBrowser *
MainWindow::playlistBrowser()
{
    return m_playlistBrowser;
}

void
MainWindow::hideContextView( bool hide )
{
    DEBUG_BLOCK

    if ( hide )
        m_contextWidget->hide();
    else
        m_contextWidget->show();
}

void MainWindow::setLayoutLocked( bool locked )
{
    DEBUG_BLOCK

    if( locked )
    {
        debug() << "locked!";
        const QFlags<QDockWidget::DockWidgetFeature> features = QDockWidget::NoDockWidgetFeatures;

        m_browsersDock->setFeatures( features );
        m_browsersDock->setTitleBarWidget( m_browserDummyTitleBarWidget );

        m_contextDock->setFeatures( features );
        m_contextDock->setTitleBarWidget( m_contextDummyTitleBarWidget );

        m_playlistDock->setFeatures( features );
        m_playlistDock->setTitleBarWidget( m_playlistDummyTitleBarWidget );

        m_controlBar->setFloatable( false );
        m_controlBar->setMovable( false );

        m_newToolbar->setFloatable( false );
        m_newToolbar->setMovable( false );
    }
    else
    {
        debug() << "unlocked!";
        const QFlags<QDockWidget::DockWidgetFeature> features = QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable;

        m_browsersDock->setFeatures( features );
        m_contextDock->setFeatures( features );
        m_playlistDock->setFeatures( features );


        m_browsersDock->setTitleBarWidget( 0 );
        m_contextDock->setTitleBarWidget( 0 );
        m_playlistDock->setTitleBarWidget( 0 );

        m_controlBar->setFloatable( true );
        m_controlBar->setMovable( true );

        m_newToolbar->setFloatable( true );
        m_newToolbar->setMovable( true );
    }

    AmarokConfig::setLockLayout( locked );
    AmarokConfig::self()->writeConfig();
    m_layoutLocked = locked;
}

bool
MainWindow::isLayoutLocked()
{
    return m_layoutLocked;
}

void
MainWindow::restoreLayout()
{
    DEBUG_BLOCK

    QFile file( Amarok::saveLocation() + "layout" );

    QByteArray layout;
    if ( file.open( QIODevice::ReadOnly ) )
    {
        layout = file.readAll();
        file.close();
    }

    if ( !restoreState( layout, LAYOUT_VERSION ) )
    {
        //since no layout has been loaded, we know that the items are all placed next to each other in the main window
        //so get the combined size of the widgets, as this is the space we have to play with. Then figure out
        //how much to give to each. Give the context view any pixels leftover from the integer division.

        //int totalWidgetWidth = m_browsers->width() + m_contextView->width() + m_playlistWidget->width();
        int totalWidgetWidth = contentsRect().width();

        //get the width of the splitter handles, we need to subtract these...

        const QSplitter dummySplitter;
        const int splitterHandleWidth = dummySplitter.handleWidth();

        debug() << "splitter handle widths " << splitterHandleWidth;

        totalWidgetWidth -= ( splitterHandleWidth * 2 );

        debug() << "mainwindow width" <<  contentsRect().width();
        debug() << "totalWidgetWidth" <<  totalWidgetWidth;

        const int widgetWidth = totalWidgetWidth / 3;
        const int leftover = totalWidgetWidth % 3;

        //We need to set fixed widths initially, just until the main window has been properly layed out. As soon as this has
        //happened, we will unlock these sizes again so that the elements can be resized by the user.
        m_browsers->setFixedWidth( widgetWidth );
        m_contextWidget->setFixedWidth( widgetWidth + leftover );
        m_playlistWidget->setFixedWidth( widgetWidth );

        m_dockWidthsLocked = true;
    }

    // Ensure that only one toolbar is visible
    if( !m_controlBar->isHidden() && !m_newToolbar->isHidden() )
        m_newToolbar->hide();
}

namespace The {
    MainWindow* mainWindow() { return MainWindow::s_instance; }
}

#include "MainWindow.moc"
