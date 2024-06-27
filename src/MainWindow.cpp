 /****************************************************************************************
 * Copyright (c) 2002-2013 Mark Kretschmann <kretschmann@kde.org>                       *
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

#include "App.h"
#include "ActionClasses.h"
#include "EngineController.h" //for actions in ctor
#include "KNotificationBackend.h"
#include "PaletteHandler.h"
#include "PluginManager.h"
#include "SvgHandler.h"
#include "amarokconfig.h"
#include "aboutdialog/ExtendedAboutDialog.h"
#include "aboutdialog/OcsData.h"
#include "amarokurls/AmarokUrlHandler.h"
#include "amarokurls/BookmarkManager.h"
#include "browsers/BrowserDock.h"
#include "browsers/collectionbrowser/CollectionWidget.h"
#include "browsers/filebrowser/FileBrowser.h"
#include "browsers/playlistbrowser/PlaylistBrowser.h"
#include "browsers/playlistbrowser/PodcastCategory.h"
#include "browsers/servicebrowser/ServiceBrowser.h"
#include "context/ContextDock.h"
#include "core/meta/Statistics.h"
#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core-impl/collections/mediadevicecollection/MediaDeviceCollection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "covermanager/CoverManager.h" // for actions
#include "dialogs/DiagnosticDialog.h"
#include "dialogs/EqualizerDialog.h"
#include "moodbar/MoodbarManager.h"
#include "network/NetworkAccessManagerProxy.h"
#ifdef DEBUG_BUILD_TYPE
#include "network/NetworkAccessViewer.h"
#endif // DEBUG_BUILD_TYPE
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistDock.h"
#include "playlist/PlaylistModelStack.h"
#include "playlist/ProgressiveSearchWidget.h"
#include "playlist/layouts/LayoutConfigAction.h"
#include "playlistmanager/PlaylistManager.h"
#include "playlistmanager/file/PlaylistFileProvider.h"
#include "services/scriptable/ScriptableService.h"
#include "statsyncing/Controller.h"
#include "toolbar/MainToolbar.h"
#include "toolbar/SlimToolbar.h"
#include "widgets/Osd.h"

#include <QAction>          //m_actionCollection
#include <QApplication>     //qApp
#include <QCheckBox>
#include <QClipboard>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QFileDialog>      //openPlaylist()
#include <QInputDialog>     //slotAddStream()
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QStandardPaths>
#include <QStyle>
#include <QVBoxLayout>

#include <KActionCollection>
#include <KBugReport>
#include <KFileWidget>
#include <KFormat>
#include <KGlobalAccel>
#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardAction>
#include <KWindowInfo>
#include <KWindowSystem>

#include <iostream>

#ifdef Q_OS_APPLE
#include "mac/GrowlInterface.h"
#ifdef HAVE_NOTIFICATION_CENTER
#include "mac/MacSystemNotify.h"
#endif
#endif

#define AMAROK_CAPTION I18N_NOOP( "Amarok" )

extern OcsData ocsData;

namespace The {
    MainWindow* mainWindow() { return pApp->mainWindow(); }
}

MainWindow::MainWindow()
    : KMainWindow( nullptr )
    , m_showMenuBar( nullptr )
    , m_lastBrowser( 0 )
    , m_waitingForCd( false )
    , m_expensiveDrawingPaused( false )
{
    DEBUG_BLOCK

    setObjectName( "MainWindow" );

#ifdef Q_OS_APPLE
    (void)new GrowlInterface( qApp->applicationName() );
#ifdef HAVE_NOTIFICATION_CENTER
    (void)new OSXNotify( qApp->applicationName() );
#endif
#endif

    PERF_LOG( "Instantiate Collection Manager" )
    CollectionManager::instance();
    PERF_LOG( "Started Collection Manager instance" )

    /* The PluginManager needs to be loaded before the playlist model
    * (which gets started by "statusBar::connectPlaylist" below so that it can handle any
    * tracks in the saved playlist that are associated with services. Eg, if
    * the playlist has a Magnatune track in it when Amarok is closed, then the
    * Magnatune service needs to be initialized before the playlist is loaded
    * here. */
    PERF_LOG( "Instantiate Plugin Manager" )
    The::pluginManager();
    PERF_LOG( "Started Plugin Manager instance" )

    createActions();
    PERF_LOG( "Created actions" )

    The::paletteHandler()->setPalette( palette() );
    setPlainCaption( i18n( AMAROK_CAPTION ) );

    init();  // We could as well move the code from init() here, but meh.. getting a tad long

    //restore active category ( as well as filters and levels and whatnot.. )
    const QString path = Amarok::config().readEntry( "Browser Path", QString() );
    if( !path.isEmpty() )
        m_browserDock->list()->navigate( path );

    setAutoSaveSettings();

    m_showMenuBar->setChecked(!menuBar()->isHidden());  // workaround for bug #171080

    EngineController *engine = The::engineController();
    connect( engine, &EngineController::stopped, this, &MainWindow::slotStopped );
    connect( engine, &EngineController::paused, this, &MainWindow::slotPaused );
    connect( engine, &EngineController::trackPlaying, this, &MainWindow::slotNewTrackPlaying );
    connect( engine, &EngineController::trackMetadataChanged, this, &MainWindow::slotMetadataChanged );

    connect( KWindowSystem::self(), &KWindowSystem::currentDesktopChanged, this, &MainWindow::checkIfExpensivesShouldBeDrawn );
    connect( KWindowSystem::self(), qOverload<WId, NET::Properties, NET::Properties2>(&KWindowSystem::windowChanged),
             [this](WId id, NET::Properties prop, NET::Properties2 ) {
                 if ( id == winId() && ( prop & NET::WMDesktop || prop & NET::WMState || prop & NET::XAWMState ) )
                     checkIfExpensivesShouldBeDrawn();
            } );
}

MainWindow::~MainWindow()
{
    DEBUG_BLOCK

    //save currently active category
    Amarok::config().writeEntry( "Browser Path", m_browserDock->list()->path() );

#ifdef DEBUG_BUILD_TYPE
    delete m_networkViewer.data();
#endif // DEBUG_BUILD_TYPE
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
    layout()->setContentsMargins( 0, 0, 0, 0 );
    layout()->setSpacing( 0 );

    //create main toolbar
    m_mainToolbar = new MainToolbar( this );
    m_mainToolbar.data()->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
    m_mainToolbar.data()->setMovable ( true );
    addToolBar( Qt::TopToolBarArea, m_mainToolbar.data() );

    //create slim toolbar
    m_slimToolbar = new SlimToolbar( this );
    m_slimToolbar.data()->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
    m_slimToolbar.data()->setMovable ( true );
    addToolBar( Qt::TopToolBarArea, m_slimToolbar.data() );
    m_slimToolbar->hide();

    //BEGIN Creating Widgets
    PERF_LOG( "Create sidebar" )
    m_browserDock = new BrowserDock( this );
    m_browserDock->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );

    m_browserDock->installEventFilter( this );
    PERF_LOG( "Sidebar created" )

    PERF_LOG( "Create Playlist" )
    m_playlistDock = new Playlist::Dock( this );
    m_playlistDock->installEventFilter( this );
    //HACK, need to connect after because of order in MainWindow()
    connect( Amarok::actionCollection()->action( "playlist_edit_queue" ),
             &QAction::triggered, m_playlistDock.data(), &Playlist::Dock::slotEditQueue );
    PERF_LOG( "Playlist created" )

    PERF_LOG( "Creating ContextWidget" )
    m_contextDock = new ContextDock( this );
    m_contextDock->installEventFilter( this );
    PERF_LOG( "ContextScene created" )
    //END Creating Widgets

    createMenus();

    setDockOptions( QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks
                    | QMainWindow::AnimatedDocks | QMainWindow::VerticalTabs );

    addDockWidget( Qt::LeftDockWidgetArea, m_browserDock.data() );
    addDockWidget( Qt::LeftDockWidgetArea, m_contextDock.data(), Qt::Horizontal );
    addDockWidget( Qt::LeftDockWidgetArea, m_playlistDock.data(), Qt::Horizontal );

    setLayoutLocked( AmarokConfig::lockLayout() );

    //<Browsers>
    {
        Debug::Block block( "Creating browsers. Please report long start times!" );

        //TODO: parent these browsers?
        PERF_LOG( "Creating CollectionWidget" )
        m_collectionBrowser = new CollectionWidget( "collections", nullptr );
        //TODO: rename "Music Collections
        m_collectionBrowser->setPrettyName( i18n( "Local Music" ) );
        m_collectionBrowser->setIcon( QIcon::fromTheme( "drive-harddisk" ) );
        m_collectionBrowser->setShortDescription( i18n( "Local sources of content" ) );
        m_browserDock->list()->addCategory( m_collectionBrowser );
        PERF_LOG( "Created CollectionWidget" )

        PERF_LOG( "Creating ServiceBrowser" )
        ServiceBrowser *serviceBrowser = ServiceBrowser::instance();
        serviceBrowser->setParent( nullptr );
        serviceBrowser->setPrettyName( i18n( "Internet" ) );
        serviceBrowser->setIcon( QIcon::fromTheme( "internet-services" ) );
        serviceBrowser->setShortDescription( i18n( "Online sources of content" ) );
        m_browserDock->list()->addCategory( serviceBrowser );
        PERF_LOG( "Created ServiceBrowser" )

        PERF_LOG( "Creating PlaylistBrowser" )
        m_playlistBrowser = new PlaylistBrowserNS::PlaylistBrowser( "playlists", nullptr );
        m_playlistBrowser->setPrettyName( i18n("Playlists") );
        m_playlistBrowser->setIcon( QIcon::fromTheme( "view-media-playlist-amarok" ) );
        m_playlistBrowser->setShortDescription( i18n( "Various types of playlists" ) );
        m_browserDock->list()->addCategory( m_playlistBrowser );
        PERF_LOG( "CreatedPlaylsitBrowser" )

        PERF_LOG( "Creating FileBrowser" )
        FileBrowser *fileBrowser = new FileBrowser( "files", nullptr );
        fileBrowser->setPrettyName( i18n("Files") );
        fileBrowser->setIcon( QIcon::fromTheme( "folder-amarok" ) );

        fileBrowser->setShortDescription( i18n( "Browse local hard drive for content" ) );
        m_browserDock->list()->addCategory( fileBrowser );
        PERF_LOG( "Created FileBrowser" )

        serviceBrowser->setScriptableServiceManager( The::scriptableServiceManager() );
        PERF_LOG( "ScriptableServiceManager done" )

        PERF_LOG( "Creating Podcast Category" )
        m_browserDock->list()->addCategory( The::podcastCategory() );
        PERF_LOG( "Created Podcast Category" )

    // If Amarok is started for the first time, set initial dock widget sizes
        if( !Amarok::config( "MainWindow" ).hasKey( "State" ) )
            QTimer::singleShot( 0, this, &MainWindow::setDefaultDockSizes );

        PERF_LOG( "finished MainWindow::init" )
    }
    The::amarokUrlHandler(); //Instantiate
    The::coverFetcher(); //Instantiate

    // we must filter ourself to get mouseevents on the "splitter" - what is us, but filtered by the layouter
    installEventFilter( this );
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
    menu->setTitle( i18nc("@item:inmenu", "&View" ) );

    // Layout locking:
    QAction* lockAction = new QAction( i18n( "Lock Layout" ), this );
    lockAction->setCheckable( true );
    lockAction->setChecked( AmarokConfig::lockLayout() );
    connect( lockAction, &QAction::toggled, this, &MainWindow::setLayoutLocked );
    menu->addAction( lockAction );

    menu->addSeparator();

    // Dock widgets:
    QList<QDockWidget *> dockwidgets = findChildren<QDockWidget *>();

    for( QDockWidget* dockWidget : dockwidgets )
    {
        if( dockWidget->parentWidget() == this )
            menu->addAction( dockWidget->toggleViewAction() );
    }

    menu->addSeparator();

    // Toolbars:
    QList<QToolBar *> toolbars = findChildren<QToolBar *>();
    QActionGroup* toolBarGroup = new QActionGroup( this );
    toolBarGroup->setExclusive( true );

    for( QToolBar* toolBar : toolbars )
    {
        if( toolBar->parentWidget() == this )
        {
            QAction* action = toolBar->toggleViewAction();
            connect( action, &QAction::toggled, toolBar, &QToolBar::setVisible );
            toolBarGroup->addAction( action );
            menu->addAction( action );
        }
    }

    menu->addSeparator();

    QAction *resetAction = new QAction( i18n( "Reset Layout" ), this );
    connect( resetAction, &QAction::triggered, this, &MainWindow::resetLayout );
    menu->addAction( resetAction );
}

void
MainWindow::showBrowser( const QString &name )
{
    Q_UNUSED( name );
    // showBrowser( index ); // FIXME
}

void
MainWindow::showDock( AmarokDockId dockId )
{
    QString name;
    switch( dockId )
    {
        case AmarokDockNavigation:
            name = m_browserDock->windowTitle();
            break;
        case AmarokDockContext:
            name = m_contextDock->windowTitle();
            break;
        case AmarokDockPlaylist:
            name = m_playlistDock->windowTitle();
            break;
    }

    QList < QTabBar * > tabList = findChildren < QTabBar * > ();

    for( QTabBar *bar : tabList )
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
MainWindow::closeEvent( QCloseEvent *e )
{
#ifdef Q_OS_APPLE
    Q_UNUSED( e );
    hide();
#else

    //KDE policy states we should hide to tray and not quit() when the
    //close window button is pushed for the main widget
    if( AmarokConfig::showTrayIcon() && e->spontaneous() && !qApp->isSavingSession() )
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
    pApp->quit();
#endif
}

void
MainWindow::exportPlaylist() //SLOT
{
    DEBUG_BLOCK

    QFileDialog fileDialog;
    fileDialog.restoreState( Amarok::config( "playlist-export-dialog" ).readEntry( "state", QByteArray() ) );

    // FIXME: Make checkbox visible in dialog
    QCheckBox *saveRelativeCheck = new QCheckBox( i18n("Use relative path for &saving"), &fileDialog );
    saveRelativeCheck->setChecked( AmarokConfig::relativePlaylist() );

    QStringList supportedMimeTypes;

    supportedMimeTypes << "audio/x-ms-asx"; //ASX
    supportedMimeTypes << "audio/x-mpegurl"; //M3U
    supportedMimeTypes << "audio/x-scpls"; //PLS
    supportedMimeTypes << "application/xspf+xml"; //XSPF

    fileDialog.setMimeTypeFilters( supportedMimeTypes );
    fileDialog.setAcceptMode( QFileDialog::AcceptSave );
    fileDialog.setFileMode( QFileDialog::AnyFile );
    fileDialog.setWindowTitle( i18n("Save As") );
    fileDialog.setObjectName( "PlaylistExport" );

    int result = fileDialog.exec();
    QString playlistPath = fileDialog.selectedFiles().value( 0 );
    if( result == QDialog::Accepted && !playlistPath.isEmpty() )
        The::playlist()->exportPlaylist( playlistPath, saveRelativeCheck->isChecked() );

    Amarok::config( "playlist-export-dialog" ).writeEntry( "state", fileDialog.saveState() );
}

void
MainWindow::slotShowActiveTrack() const
{
    m_playlistDock->showActiveTrack();
}

void
MainWindow::slotEditTrackInfo() const
{
    m_playlistDock->editTrackInfo();
}

void
MainWindow::slotShowCoverManager() //SLOT
{
    CoverManager::showOnce( QString(), this );
}

void
MainWindow::slotShowDiagnosticsDialog()
{
    DiagnosticDialog *dialog = new DiagnosticDialog( KAboutData::applicationData(), this );
    dialog->show();
}

void MainWindow::slotShowBookmarkManager()
{
    BookmarkManager::showOnce( this );
}

void MainWindow::slotShowEqualizer()
{
    EqualizerDialog::showOnce( this );
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
    static QUrl lastDirectory;

    // open a file selector to add media to the playlist
    QList<QUrl> files;
    QFileDialog dlg;
    dlg.setDirectory( QStandardPaths::writableLocation(QStandardPaths::MusicLocation) );

    if( !lastDirectory.isEmpty() )
        dlg.setDirectoryUrl( lastDirectory );

    dlg.setWindowTitle( directPlay ? i18n("Play Media (Files or URLs)") : i18n("Add Media (Files or URLs)") );
    dlg.setFileMode( QFileDialog::ExistingFiles );
    dlg.setObjectName( "PlayMedia" );
    int accepted = dlg.exec();
    files = dlg.selectedUrls();

    lastDirectory = dlg.directoryUrl();

    if( accepted != QDialog::Accepted || files.isEmpty() )
        return;

    Playlist::AddOptions options = directPlay ? Playlist::OnPlayMediaAction
                                              : Playlist::OnAppendToPlaylistAction;
    The::playlistController()->insertOptioned( files, options );
}

void
MainWindow::slotAddStream() //SLOT
{
    bool ok;
    QString url = QInputDialog::getText( this, i18n( "Add Stream" ), i18n( "Enter Stream URL:" ),
                                         QLineEdit::Normal, QString(), &ok );
    if( !ok )
        return;

    The::playlistController()->insertOptioned( QUrl( url ),
            Playlist::OnAppendToPlaylistAction | Playlist::RemotePlaylistsAreStreams );
}

void
MainWindow::slotFocusPlaylistSearch()
{
    showDock( AmarokDockPlaylist );  // ensure that the dock is visible if tabbed
    m_playlistDock->searchWidget()->focusInputLine();
}

void
MainWindow::slotFocusCollectionSearch()
{
    // ensure collection browser is activated within navigation dock:
    browserDock()->list()->navigate( QStringLiteral("collections") );
    showDock( AmarokDockNavigation );  // ensure that the dock is visible if tabbed
    m_collectionBrowser->focusInputLine();
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
    const KWindowInfo info( winId(), {}, {} );
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
    Q_EMIT loveTrack( The::engineController()->currentTrack() );
}

void
MainWindow::slotBanTrack()
{
    Q_EMIT banTrack( The::engineController()->currentTrack() );
}

void
MainWindow::slotShufflePlaylist()
{
    m_playlistDock->sortWidget()->trimToLevel();
    The::playlistActions()->shuffle();
}

void
MainWindow::slotSeekForwardShort()
{
    EngineController* ec = The::engineController();
    ec->seekBy( AmarokConfig::seekShort() * 1000 );
}

void
MainWindow::slotSeekForwardMedium()
{
    EngineController* ec = The::engineController();
    ec->seekBy( AmarokConfig::seekMedium() * 1000 );
}

void
MainWindow::slotSeekForwardLong()
{
    EngineController* ec = The::engineController();
    ec->seekBy( AmarokConfig::seekLong() * 1000 );
}

void
MainWindow::slotSeekBackwardShort()
{
    EngineController* ec = The::engineController();
    ec->seekBy( AmarokConfig::seekShort() * -1000 );
}

void
MainWindow::slotSeekBackwardMedium()
{
    EngineController* ec = The::engineController();
    ec->seekBy( AmarokConfig::seekMedium() * -1000 );
}

void
MainWindow::slotSeekBackwardLong()
{
    EngineController* ec = The::engineController();
    ec->seekBy( AmarokConfig::seekLong() * -1000 );
}

void MainWindow::slotPutCurrentTrackToClipboard()
{
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if ( currentTrack )
    {
        QString text;
        Meta::ArtistPtr artist = currentTrack->artist();
        if( artist )
            text = artist->prettyName() + " - ";
        text += currentTrack->prettyName();

        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText( text );
    }
}

void
MainWindow::activate()
{
    const KWindowInfo info( winId(), NET::WMState | NET::XAWMState | NET::WMDesktop );

    if( KWindowSystem::activeWindow() != winId() )
        setVisible( true );
    else if( !info.isMinimized() )
        setVisible( true );
    if( !isHidden() )
        KWindowSystem::activateWindow( winId() );
}

void
MainWindow::createActions()
{
    KActionCollection* const ac = Amarok::actionCollection();
    const EngineController* const ec = The::engineController();
    const Playlist::Actions* const pa = The::playlistActions();
    const Playlist::Controller* const pc = The::playlistController();

    KStandardAction::keyBindings( pApp, &App::slotConfigShortcuts, ac );
    KStandardAction::preferences( pApp, &App::slotConfigAmarokWithEmptyPage, ac );

    m_showMenuBar = KStandardAction::showMenubar(this, &MainWindow::slotShowMenuBar, ac);
    ac->action( KStandardAction::name( KStandardAction::KeyBindings ) )->setIcon( QIcon::fromTheme( QStringLiteral("configure-shortcuts-amarok") ) );
    ac->action( KStandardAction::name( KStandardAction::Preferences ) )->setIcon( QIcon::fromTheme( QStringLiteral("configure-amarok") ) );
    ac->action( KStandardAction::name( KStandardAction::Preferences ) )->setMenuRole(QAction::PreferencesRole); // Define OS X Prefs menu here, removes need for ifdef later

    KStandardAction::quit( pApp, &App::quit, ac );

    QAction *action = new QAction( QIcon::fromTheme( QStringLiteral("document-open") ), i18n("&Add Media..."), this );
    ac->addAction( "playlist_add", action );
    connect( action, &QAction::triggered, this, &MainWindow::slotAddLocation );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence( Qt::META + Qt::Key_A ) );

    action = new QAction( QIcon::fromTheme( QStringLiteral("edit-clear-list") ), i18nc( "clear playlist", "&Clear Playlist" ), this );
    connect( action, &QAction::triggered, pc, &Playlist::Controller::clear );
    ac->addAction( "playlist_clear", action );

    action = new QAction( QIcon::fromTheme( QStringLiteral("format-list-ordered") ),
                          i18nc( "edit play queue of playlist", "Edit &Queue" ), this );
    //Qt::META+Qt::Key_Q is taken by Plasma as a global
    action->setShortcut( QKeySequence( Qt::META + Qt::Key_U ) );
    ac->addAction( "playlist_edit_queue", action );

    action = new QAction( i18nc( "Remove duplicate and dead (unplayable) tracks from the playlist", "Re&move Duplicates" ), this );
    connect( action, &QAction::triggered, pc, &Playlist::Controller::removeDeadAndDuplicates );
    ac->addAction( "playlist_remove_dead_and_duplicates", action );

    action = new Playlist::LayoutConfigAction( this );
    ac->addAction( "playlist_layout", action );

    action = new QAction( QIcon::fromTheme( QStringLiteral("document-open-remote") ), i18n("&Add Stream..."), this );
    connect( action, &QAction::triggered, this, &MainWindow::slotAddStream );
    ac->addAction( "stream_add", action );

    action = new QAction( QIcon::fromTheme( QStringLiteral("document-export-amarok") ), i18n("&Export Playlist As..."), this );
    connect( action, &QAction::triggered, this, &MainWindow::exportPlaylist );
    ac->addAction( "playlist_export", action );

    action = new QAction( QIcon::fromTheme( QStringLiteral("bookmark-new") ), i18n( "Bookmark Media Sources View" ), this );
    ac->addAction( "bookmark_browser", action );
    connect( action, &QAction::triggered, The::amarokUrlHandler(), &AmarokUrlHandler::bookmarkCurrentBrowserView );

    action = new QAction( QIcon::fromTheme( QStringLiteral("bookmarks-organize") ), i18n( "Bookmark Manager" ), this );
    ac->addAction( "bookmark_manager", action );
    connect( action, &QAction::triggered, this, &MainWindow::slotShowBookmarkManager );

    action = new QAction( QIcon::fromTheme( QStringLiteral("view-media-equalizer") ), i18n( "Equalizer" ), this );
    ac->addAction( "equalizer_dialog", action );
    connect( action, &QAction::triggered, this, &MainWindow::slotShowEqualizer );

    action = new QAction( QIcon::fromTheme( QStringLiteral("bookmark-new") ), i18n( "Bookmark Playlist Setup" ), this );
    ac->addAction( "bookmark_playlistview", action );
    connect( action, &QAction::triggered, The::amarokUrlHandler(), &AmarokUrlHandler::bookmarkCurrentPlaylistView );

    action = new QAction( QIcon::fromTheme( QStringLiteral("bookmark-new") ), i18n( "Bookmark Context Applets" ), this );
    ac->addAction( "bookmark_contextview", action );
    connect( action, &QAction::triggered, The::amarokUrlHandler(), &AmarokUrlHandler::bookmarkCurrentContextView );

    action = new QAction( QIcon::fromTheme( QStringLiteral("media-album-cover-manager-amarok") ), i18n( "Cover Manager" ), this );
    connect( action, &QAction::triggered, this, &MainWindow::slotShowCoverManager );
    ac->addAction( "cover_manager", action );

    action = new QAction( QIcon::fromTheme(QStringLiteral("document-open")), i18n("Play Media..."), this );
    ac->addAction( "playlist_playmedia", action );
    action->setShortcut( Qt::CTRL + Qt::Key_O );
    connect( action, &QAction::triggered, this, &MainWindow::slotPlayMedia );

    action = new QAction( QIcon::fromTheme(QStringLiteral("media-track-edit-amarok")), i18n("Edit Details of Currently Selected Track"), this );
    ac->addAction( "trackdetails_edit", action );
    action->setShortcut( Qt::CTRL + Qt::Key_E );
    connect( action, &QAction::triggered, this, &MainWindow::slotEditTrackInfo );

    action = new QAction( QIcon::fromTheme( QStringLiteral("media-seek-forward-amarok") ),
                          i18n( "Seek Forward by %1 seconds", KFormat().formatDecimalDuration( AmarokConfig::seekShort() * 1000 ) ), this );
    ac->addAction( "seek_forward_short", action );
    action->setShortcut( Qt::CTRL + Qt::Key_Right );
    connect( action, &QAction::triggered, this, &MainWindow::slotSeekForwardShort );

    action = new QAction( QIcon::fromTheme( QStringLiteral("media-seek-forward-amarok") ),
                          i18n( "Seek Forward by %1 seconds", KFormat().formatDecimalDuration( AmarokConfig::seekMedium() * 1000 ) ), this );
    ac->addAction( "seek_forward_medium", action );
    action->setShortcut( Qt::Key_Right );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence( Qt::META + Qt::SHIFT + Qt::Key_Plus ) );
    connect( action, &QAction::triggered, this, &MainWindow::slotSeekForwardMedium );

    action = new QAction( QIcon::fromTheme( QStringLiteral("media-seek-forward-amarok") ),
                          i18n( "Seek Forward by %1 seconds", KFormat().formatDecimalDuration( AmarokConfig::seekLong() * 1000 ) ), this );
    ac->addAction( "seek_forward_long", action );
    action->setShortcut( Qt::SHIFT + Qt::Key_Right );
    connect( action, &QAction::triggered, this, &MainWindow::slotSeekForwardLong );


    action = new QAction( QIcon::fromTheme( QStringLiteral("media-seek-backward-amarok") ),
                          i18n( "Seek Backward by %1 seconds", KFormat().formatDecimalDuration( AmarokConfig::seekShort() * 1000 ) ), this );
    ac->addAction( "seek_backward_short", action );
    action->setShortcut( Qt::CTRL + Qt::Key_Left );
    connect( action, &QAction::triggered, this, &MainWindow::slotSeekBackwardShort );

    action = new QAction( QIcon::fromTheme( QStringLiteral("media-seek-backward-amarok") ),
                          i18n( "Seek Backward by %1 seconds", KFormat().formatDecimalDuration( AmarokConfig::seekMedium() * 1000 ) ), this );
    ac->addAction( "seek_backward_medium", action );
    action->setShortcut( Qt::Key_Left );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence( Qt::META + Qt::SHIFT + Qt::Key_Minus ) );
    connect( action, &QAction::triggered, this, &MainWindow::slotSeekBackwardMedium );

    action = new QAction( QIcon::fromTheme( QStringLiteral("media-seek-backward-amarok") ),
                          i18n( "Seek Backward by %1 seconds", KFormat().formatDecimalDuration( AmarokConfig::seekLong() * 1000 ) ), this );
    ac->addAction( "seek_backward_long", action );
    action->setShortcut( Qt::SHIFT + Qt::Key_Left );
    connect( action, &QAction::triggered, this, &MainWindow::slotSeekBackwardLong );

    PERF_LOG( "MainWindow::createActions 6" )
    action = new QAction( QIcon::fromTheme(QStringLiteral("view-refresh")), i18n( "Update Collection" ), this );
    connect ( action, &QAction::triggered, CollectionManager::instance(), &CollectionManager::checkCollectionChanges );
    ac->addAction( "update_collection", action );

    action =  new QAction( QIcon::fromTheme( QStringLiteral("amarok_playcount") ), i18n( "Synchronize Statistics..." ), this );
    ac->addAction( "synchronize_statistics", action );
    connect( action, &QAction::triggered, Amarok::Components::statSyncingController(),
             &StatSyncing::Controller::synchronize );
    Amarok::Components::statSyncingController();
    action = new QAction( this );
    ac->addAction( "prev", action );
    action->setIcon( QIcon::fromTheme(QStringLiteral("media-skip-backward-amarok")) );
    action->setText( i18n( "Previous Track" ) );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence() );
    connect( action, &QAction::triggered, pa, &Playlist::Actions::back );

    action = new QAction( this );
    ac->addAction( "replay", action );
    action->setIcon( QIcon::fromTheme(QStringLiteral("media-playback-start")) );
    action->setText( i18n( "Restart current track" ) );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence() );
    connect( action, &QAction::triggered, ec, &EngineController::replay );

    action = new QAction( this );
    ac->addAction( "shuffle_playlist", action );
    action->setIcon( QIcon::fromTheme(QStringLiteral("media-playlist-shuffle")) );
    action->setText( i18n( "Shuffle Playlist" ) );
    action->setShortcut( Qt::CTRL + Qt::Key_H );
    connect( action, &QAction::triggered, this, &MainWindow::slotShufflePlaylist );

    action = new QAction( this );
    ac->addAction( QStringLiteral("repopulate"), action );
    action->setText( i18n( "Repopulate Playlist" ) );
    action->setIcon( QIcon::fromTheme("view-refresh-amarok") );
    connect( action, &QAction::triggered, pa, &Playlist::Actions::repopulateDynamicPlaylist );

    action = new QAction( this );
    ac->addAction( QStringLiteral("disable_dynamic"), action );
    action->setText( i18n( "Disable Dynamic Playlist" ) );
    action->setIcon( QIcon::fromTheme("edit-delete-amarok") );
    //this is connected inside the dynamic playlist category

    action = new QAction( QIcon::fromTheme(QStringLiteral("media-skip-forward-amarok")), i18n( "Next Track" ), this );
    ac->addAction( "next", action );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence() );
    connect( action, &QAction::triggered, pa, &Playlist::Actions::next );

    action = new QAction( i18n( "Increase Volume" ), this );
    ac->addAction( "increaseVolume", action );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence( Qt::META + Qt::Key_Plus ) );
    action->setShortcut( Qt::Key_Plus );
    connect( action, &QAction::triggered, ec, &EngineController::regularIncreaseVolume );

    action = new QAction( i18n( "Decrease Volume" ), this );
    ac->addAction( "decreaseVolume", action );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence( Qt::META + Qt::Key_Minus ) );
    action->setShortcut( Qt::Key_Minus );
    connect( action, &QAction::triggered, ec, &EngineController::regularDecreaseVolume );

    action = new QAction( i18n( "Toggle Main Window" ), this );
    ac->addAction( "toggleMainWindow", action );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence() );
    connect( action, &QAction::triggered, this, &MainWindow::showHide );

    action = new QAction( i18n( "Toggle Full Screen" ), this );
    ac->addAction( "toggleFullScreen", action );
    action->setShortcut( QKeySequence( Qt::CTRL + Qt::SHIFT + Qt::Key_F ) );
    connect( action, &QAction::triggered, this, &MainWindow::slotFullScreen );

    action = new QAction( i18n( "Search playlist" ), this );
    ac->addAction( "searchPlaylist", action );
    action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_J ) );
    connect( action, &QAction::triggered, this, &MainWindow::slotFocusPlaylistSearch );

    action = new QAction( i18n( "Search collection" ), this );
    ac->addAction( "searchCollection", action );
    action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_F ) );
    connect( action, &QAction::triggered, this, &MainWindow::slotFocusCollectionSearch );

    action = new QAction( QIcon::fromTheme( "music-amarok" ), i18n("Show active track"), this );
    ac->addAction( "show_active_track", action );
    connect( action, &QAction::triggered, this, &MainWindow::slotShowActiveTrack );

    action = new QAction( i18n( "Show Notification Popup" ), this );
    ac->addAction( "showNotificationPopup", action );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence( Qt::META + Qt::Key_O ) );
    connect( action, &QAction::triggered, this, &MainWindow::showNotificationPopup );

    action = new QAction( i18n( "Mute Volume" ), this );
    ac->addAction( "mute", action );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence( Qt::META + Qt::Key_M ) );
    connect( action, &QAction::triggered, ec, &EngineController::toggleMute );

    action = new QAction( i18n( "Last.fm: Love Current Track" ), this );
    ac->addAction( "loveTrack", action );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence( Qt::META + Qt::Key_L ) );
    connect( action, &QAction::triggered, this, &MainWindow::slotLoveTrack );

    action = new QAction( i18n( "Last.fm: Ban Current Track" ), this );
    ac->addAction( "banTrack", action );
    //KGlobalAccel::setGlobalShortcut(action, QKeySequence( Qt::META + Qt::Key_B ) );
    connect( action, &QAction::triggered, this, &MainWindow::slotBanTrack );

    action = new QAction( i18n( "Last.fm: Skip Current Track" ), this );
    ac->addAction( "skipTrack", action );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence( Qt::META + Qt::Key_S ) );
    connect( action, &QAction::triggered, this, &MainWindow::skipTrack );

    action = new QAction( QIcon::fromTheme( "media-track-queue-amarok" ), i18n( "Queue Track" ), this );
    ac->addAction( "queueTrack", action );
    action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_D ) );
    connect( action, &QAction::triggered, this, &MainWindow::switchQueueStateShortcut );

    action = new QAction( i18n( "Put Artist - Title of the current track to the clipboard" ), this  );
    ac->addAction("artistTitleClipboard", action);
    action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_C ) );
    connect( action, &QAction::triggered, this, &MainWindow::slotPutCurrentTrackToClipboard );

    action = new QAction( i18n( "Rate Current Track: 1" ), this );
    ac->addAction( "rate1", action );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence( Qt::META + Qt::Key_1 ) );
    connect( action, &QAction::triggered, this, &MainWindow::setRating1 );

    action = new QAction( i18n( "Rate Current Track: 2" ), this );
    ac->addAction( "rate2", action );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence( Qt::META + Qt::Key_2 ) );
    connect( action, &QAction::triggered, this, &MainWindow::setRating2 );

    action = new QAction( i18n( "Rate Current Track: 3" ), this );
    ac->addAction( "rate3", action );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence( Qt::META + Qt::Key_3 ) );
    connect( action, &QAction::triggered, this, &MainWindow::setRating3 );

    action = new QAction( i18n( "Rate Current Track: 4" ), this );
    ac->addAction( "rate4", action );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence( Qt::META + Qt::Key_4 ) );
    connect( action, &QAction::triggered, this, &MainWindow::setRating4 );

    action = new QAction( i18n( "Rate Current Track: 5" ), this );
    ac->addAction( "rate5", action );
    KGlobalAccel::setGlobalShortcut(action, QKeySequence( Qt::META + Qt::Key_5 ) );
    connect( action, &QAction::triggered, this, &MainWindow::setRating5 );

#ifdef DEBUG_BUILD_TYPE
    action = new QAction( i18n( "Network Request Viewer" ), this );
    ac->addAction( "network_request_viewer", action );
    action->setIcon( QIcon::fromTheme( "utilities-system-monitor" ) );
    connect( action, &QAction::triggered, this, &MainWindow::showNetworkRequestViewer );
#endif // DEBUG_BUILD_TYPE

    action = KStandardAction::redo( pc, &Playlist::Controller::redo, this);
    ac->addAction( "playlist_redo", action );
    action->setEnabled( false );
    action->setIcon( QIcon::fromTheme( "edit-redo" ) );
    connect( pc, &Playlist::Controller::canRedoChanged, action, &QAction::setEnabled );

    action = KStandardAction::undo( pc, &Playlist::Controller::undo, this);
    ac->addAction( "playlist_undo", action );
    action->setEnabled( false );
    action->setIcon( QIcon::fromTheme( "edit-undo" ) );
    connect( pc, &Playlist::Controller::canUndoChanged, action, &QAction::setEnabled );

    action = new QAction( QIcon::fromTheme( "amarok" ), i18n( "&About Amarok" ), this );
    ac->addAction( "extendedAbout", action );
    connect( action, &QAction::triggered, this, &MainWindow::showAbout );

    action = new QAction ( QIcon::fromTheme( "info-amarok" ), i18n( "&Diagnostics" ), this );
    ac->addAction( "diagnosticDialog", action );
    connect( action, &QAction::triggered, this, &MainWindow::slotShowDiagnosticsDialog );

    action = new QAction( QIcon::fromTheme( "tools-report-bug" ), i18n("&Report Bug..."), this );
    ac->addAction( "reportBug", action );
    connect( action, &QAction::triggered, this, &MainWindow::showReportBug );

    PERF_LOG( "MainWindow::createActions 8" )
    new Amarok::MenuAction( ac, this );
    new Amarok::StopAction( ac, this );
    new Amarok::StopPlayingAfterCurrentTrackAction( ac, this );
    new Amarok::PlayPauseAction( ac, this );
    new Amarok::ReplayGainModeAction( ac, this );

    ac->addAssociatedWidget( this );
    for( QAction* action : ac->actions() )
        action->setShortcutContext( Qt::WindowShortcut );
}

void
MainWindow::setRating( int n )
{
    n *= 2;

    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( track )
    {
        Meta::StatisticsPtr statistics = track->statistics();
        // if we're setting an identical rating then we really must
        // want to set the half-star below rating
        if( statistics->rating() == n )
            n -= 1;

        statistics->setRating( n );
        Amarok::OSD::instance()->OSDWidget::ratingChanged( statistics->rating() );
    }
}

void
MainWindow::createMenus()
{
    m_menubar = menuBar();

    //BEGIN Actions menu
    QMenu *actionsMenu = new QMenu( m_menubar );
#ifdef Q_OS_APPLE
    // Add these functions to the dock icon menu in OS X
    //extern void qt_mac_set_dock_menu(QMenu *);
    //qt_mac_set_dock_menu(actionsMenu);
    // Change to avoid duplicate menu titles in OS X
    actionsMenu->setTitle( i18n("&Music") );
#else
    actionsMenu->setTitle( i18n("&Amarok") );
#endif
    actionsMenu->addAction( Amarok::actionCollection()->action("playlist_playmedia") );
    actionsMenu->addSeparator();
    actionsMenu->addAction( Amarok::actionCollection()->action("prev") );
    actionsMenu->addAction( Amarok::actionCollection()->action("play_pause") );
    actionsMenu->addAction( Amarok::actionCollection()->action("stop") );
    actionsMenu->addAction( Amarok::actionCollection()->action("stop_after_current") );
    actionsMenu->addAction( Amarok::actionCollection()->action("next") );


#ifndef Q_OS_APPLE    // Avoid duplicate "Quit" in OS X dock menu
    actionsMenu->addSeparator();
    actionsMenu->addAction( Amarok::actionCollection()->action( KStandardAction::name( KStandardAction::Quit ) ) );
#endif
    //END Actions menu

    //BEGIN View menu
    QMenu* viewMenu = new QMenu(this);
    addViewMenuItems(viewMenu);
    //END View menu

    //BEGIN Playlist menu
    QMenu *playlistMenu = new QMenu( m_menubar.data() );
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
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_edit_queue") );
    //END Playlist menu

    //BEGIN Tools menu
    m_toolsMenu = new QMenu( m_menubar.data() );
    m_toolsMenu->setTitle( i18n("&Tools") );

    m_toolsMenu->addAction( Amarok::actionCollection()->action("bookmark_manager") );
    m_toolsMenu->addAction( Amarok::actionCollection()->action("cover_manager") );
    m_toolsMenu->addAction( Amarok::actionCollection()->action("equalizer_dialog") );
#ifdef DEBUG_BUILD_TYPE
    m_toolsMenu->addAction( Amarok::actionCollection()->action("network_request_viewer") );
#endif // DEBUG_BUILD_TYPE
    m_toolsMenu->addSeparator();
    m_toolsMenu->addAction( Amarok::actionCollection()->action("update_collection") );
    m_toolsMenu->addAction( Amarok::actionCollection()->action("synchronize_statistics") );
    //END Tools menu

    //BEGIN Settings menu
    m_settingsMenu = new QMenu( m_menubar.data() );
    m_settingsMenu->setTitle( i18n("&Settings") );

    m_settingsMenu->addAction( Amarok::actionCollection()->action( KStandardAction::name( KStandardAction::ShowMenubar ) ) );

    //TODO use KStandardAction or KXmlGuiWindow

    // the phonon-coreaudio  backend has major issues with either the VolumeFaderEffect itself
    // or with it in the pipeline. track playback stops every ~3-4 tracks, and on tracks >5min it
    // stops at about 5:40. while we get this resolved upstream, don't make playing amarok such on osx.
    // so we disable replaygain on osx

#ifndef Q_OS_APPLE
    m_settingsMenu->addAction( Amarok::actionCollection()->action("replay_gain_mode") );
    m_settingsMenu->addSeparator();
#endif

    m_settingsMenu->addAction( Amarok::actionCollection()->action( KStandardAction::name( KStandardAction::KeyBindings ) ) );
    m_settingsMenu->addAction( Amarok::actionCollection()->action( KStandardAction::name( KStandardAction::Preferences ) ) );
    //END Settings menu

    m_menubar->addMenu( actionsMenu );
    m_menubar->addMenu( viewMenu );
    m_menubar->addMenu( playlistMenu );
    m_menubar->addMenu( m_toolsMenu.data() );
    m_menubar->addMenu( m_settingsMenu.data() );

    QMenu *helpMenu = Amarok::Menu::helpMenu();
    helpMenu->insertAction( helpMenu->actions().last(),
                            Amarok::actionCollection()->action( "extendedAbout" ) );
    helpMenu->insertAction( helpMenu->actions().last(),
                            Amarok::actionCollection()->action( "diagnosticDialog" ) );

    m_menubar->addSeparator();
    m_menubar->addMenu( helpMenu );
}

void
MainWindow::slotShowMenuBar()
{
    if (!m_showMenuBar->isChecked())
    {
        //User have chosen to hide a menu. Lets warn him
        if (KMessageBox::warningContinueCancel(this,
            i18n("You have chosen to hide the menu bar.\n\nPlease remember that you can always use the shortcut \"%1\" to bring it back.", m_showMenuBar->shortcut().toString() ),
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
    ExtendedAboutDialog dialog( KAboutData::applicationData(), &ocsData );
    dialog.exec();
}

void
MainWindow::showReportBug()
{
    KBugReport * rbDialog = new KBugReport( KAboutData::applicationData() ,this  );
    rbDialog->setObjectName( "KBugReport" );
    rbDialog->exec();
    delete rbDialog;
}

void
MainWindow::changeEvent( QEvent *event )
{
    if( event->type() == QEvent::PaletteChange )
        The::paletteHandler()->setPalette( palette() );
}

void
MainWindow::slotStopped()
{
    setPlainCaption( i18n( AMAROK_CAPTION ) );
}

void
MainWindow::slotPaused()
{
    setPlainCaption( i18n( "Paused  ::  %1", i18n( AMAROK_CAPTION ) ) );
}

void
MainWindow::slotNewTrackPlaying()
{
    slotMetadataChanged( The::engineController()->currentTrack() );
}

void
MainWindow::slotMetadataChanged( Meta::TrackPtr track )
{
    if( track )
        setPlainCaption( i18n( "%1 - %2  ::  %3", track->artist() ? track->artist()->prettyName() : i18n( "Unknown" ), track->prettyName(), i18n( AMAROK_CAPTION ) ) );
}

CollectionWidget *
MainWindow::collectionBrowser()
{
    return m_collectionBrowser;
}

QString
MainWindow::activeBrowserName()
{
    if(m_browserDock->list()->activeCategory() )
        return m_browserDock->list()->activeCategory()->name();
    else
        return QString();
}

void
MainWindow::setLayoutLocked( bool locked )
{
    DEBUG_BLOCK

    if( m_browserDock )
        m_browserDock.data()->setMovable( !locked );

    if( m_contextDock )
        m_contextDock.data()->setMovable( !locked );

    if( m_playlistDock )
        m_playlistDock.data()->setMovable( !locked );

    if( m_slimToolbar )
    {
        m_slimToolbar.data()->setFloatable( !locked );
        m_slimToolbar.data()->setMovable( !locked );
    }

    if( m_mainToolbar )
    {
        m_mainToolbar.data()->setFloatable( !locked );
        m_mainToolbar.data()->setMovable( !locked );
    }

    AmarokConfig::setLockLayout( locked );
    AmarokConfig::self()->save();
}

void
MainWindow::resetLayout()
{
    // Store current state, so that we can undo the operation
    const QByteArray state = saveState();

    // Remove all dock widgets, then add them again. This resets their state completely.
    removeDockWidget( m_browserDock.data() );
    removeDockWidget( m_contextDock.data() );
    removeDockWidget( m_playlistDock.data() );

    addDockWidget( Qt::LeftDockWidgetArea, m_browserDock.data() );
    addDockWidget( Qt::LeftDockWidgetArea, m_contextDock.data(), Qt::Horizontal );
    addDockWidget( Qt::LeftDockWidgetArea, m_playlistDock.data(), Qt::Horizontal );

    m_browserDock->setFloating( false );
    m_contextDock->setFloating( false );
    m_playlistDock->setFloating( false );

    m_browserDock->show();
    m_contextDock->show();
    m_playlistDock->show();

    // Now set Amarok's default dockwidget sizes
    setDefaultDockSizes();

    if( KMessageBox::warningContinueCancel( this, i18n( "Apply this layout change?" ), i18n( "Reset Layout" ) ) == KMessageBox::Cancel )
        restoreState( state );
}

void
MainWindow::setDefaultDockSizes() // SLOT
{
    int totalWidgetWidth = contentsRect().width();

    //get the width of the splitter handles, we need to subtract these...
    const int splitterHandleWidth = style()->pixelMetric( QStyle::PM_DockWidgetSeparatorExtent, nullptr, nullptr );

    totalWidgetWidth -= ( splitterHandleWidth * 2 );

    const int widgetWidth = totalWidgetWidth / 3;
    const int leftover = totalWidgetWidth - 3 * widgetWidth;

    //We need to set fixed widths initially, just until the main window has been properly laid out. As soon as this has
    //happened, we will unlock these sizes again so that the elements can be resized by the user.
    const int mins[3] = { m_browserDock->minimumWidth(), m_contextDock->minimumWidth(), m_playlistDock->minimumWidth() };
    const int maxs[3] = { m_browserDock->maximumWidth(), m_contextDock->maximumWidth(), m_playlistDock->maximumWidth() };

    m_browserDock->setFixedWidth( widgetWidth * 0.65 );
    m_contextDock->setFixedWidth( widgetWidth * 1.7 + leftover );
    m_playlistDock->setFixedWidth( widgetWidth * 0.65 );

    // Important: We need to activate the layout we have just set
    layout()->activate();

    m_browserDock->setMinimumWidth( mins[0] ); m_browserDock->setMaximumWidth( maxs[0] );
    m_contextDock->setMinimumWidth( mins[1] ); m_contextDock->setMaximumWidth( maxs[1] );
    m_playlistDock->setMinimumWidth( mins[2] ); m_playlistDock->setMaximumWidth( maxs[2] );
}

bool
MainWindow::playAudioCd()
{
    DEBUG_BLOCK
    //drop whatever we are doing and play auidocd

    QList<Collections::Collection*> collections = CollectionManager::instance()->viewableCollections();

    // Search a non-empty MemoryCollection with the id: AudioCd
    for( Collections::Collection *collection : collections )
    {
        if( collection->collectionId() == "AudioCd" )
        {

            debug() << "got audiocd collection";

            Collections::MediaDeviceCollection * cdColl = dynamic_cast<Collections::MediaDeviceCollection *>( collection );

            if( !cdColl || cdColl->memoryCollection()->trackMap().isEmpty() )
            {
                debug() << "cd collection not ready yet (track count = 0 )";
                m_waitingForCd = true;
                return false;
            }

            The::playlistController()->insertOptioned( cdColl->memoryCollection()->trackMap().values(), Playlist::OnPlayMediaAction );
            m_waitingForCd = false;
            return true;
        }
    }

    debug() << "waiting for cd...";
    if( !The::mainWindow() )  // this is not yet created, touching member variables now will crash us
        QTimer::singleShot(1000, []() { The::mainWindow()->playAudioCd(); });
    else
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
MainWindow::checkIfExpensivesShouldBeDrawn()
{
    const KWindowInfo info( winId(), NET::WMState | NET::XAWMState | NET::WMDesktop );

    bool newNeed = true;
    if( !info.isOnCurrentDesktop() )
        newNeed = false;
    else if( info.isMinimized() )
        newNeed = false;
    if ( newNeed != m_expensiveDrawingPaused )
    {
        debug() << "need to draw expensive elements changed, new state"<< newNeed;
        m_expensiveDrawingPaused = newNeed;
        Q_EMIT drawNeedChanged( newNeed );
    }
}


