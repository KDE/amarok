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

#define DEBUG_PREFIX "MainWindow"

#include "MainWindow.h"

#include <config-amarok.h>    //HAVE_LIBVISUAL definition

#include "ActionClasses.h"
#include "Amarok.h"
#include "Debug.h"
#include "EngineController.h" //for actions in ctor
#include "MainToolbar.h"
#include "Osd.h"
#include "PaletteHandler.h"
#include "ScriptManager.h"
#include "SearchWidget.h"
#include "Sidebar.h"
#include "Sidebar.moc"
#include "statusbar_ng/StatusBar.h"
#include "amarokconfig.h"
#include "browsers/collectionbrowser/CollectionWidget.h"
#include "browsers/filebrowser/FileBrowser.h"
#include "browsers/playlistbrowser/PlaylistBrowser.h"
#include "browsers/servicebrowser/ServiceBrowser.h"
#include "services/ServicePluginManager.h"
#include "services/scriptable/ScriptableService.h"
#include "collection/CollectionManager.h"
#include "context/ContextScene.h"
#include "context/ContextView.h"
#include "context/plasma/plasma.h"
#include "covermanager/CoverManager.h" // for actions
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModel.h"
#include "playlist/PlaylistWidget.h"
#include "playlist/view/graphic/PlaylistGraphicsView.h"
#include "playlistmanager/PlaylistFileProvider.h"
#include "playlistmanager/PlaylistManager.h"
#include "queuemanager/QueueManager.h"
#include "widgets/Splitter.h"
#include "widgets/TrackTooltip.h"
//#include "mediabrowser.h"

#include <QCheckBox>
#include <QDesktopWidget>
#include <QList>
#include <QVBoxLayout>
#include <QPixmapCache>

#include <kabstractfilewidget.h> //savePlaylist()
#include <KAction>          //m_actionCollection
#include <KActionCollection>
#include <KActionMenu>
#include <KApplication>     //kapp
#include <KFileDialog>      //savePlaylist(), openPlaylist()
#include <KInputDialog>     //slotAddStream()
#include <KLocale>
#include <KMenuBar>
#include <KMessageBox>      //savePlaylist()
#include <KMenu>
#include <KPushButton>
#include <KStandardAction>
#include <KWindowSystem>

#ifdef Q_WS_X11
#include <fixx11h.h>
#endif

#define AMAROK_CAPTION "Amarok 2 beta"

class ContextWidget : public KVBox
{
    // Set a useful size default of the center tab.
    public:
        ContextWidget( QWidget *parent ) : KVBox( parent ) {}
        QSize sizeHint() const { return QSize( static_cast<QWidget*>(parent())->size().width() / 3, 300 ); }
};

MainWindow *MainWindow::s_instance = 0;

MainWindow::MainWindow()
    : KXmlGuiWindow( 0 )
    , EngineObserver( The::engineController() )
    , m_lastBrowser( 0 )
{
    DEBUG_BLOCK

    setObjectName( "MainWindow" );
    s_instance = this;

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
}

MainWindow::~MainWindow()
{
    DEBUG_BLOCK

    KConfigGroup config = Amarok::config();
    config.writeEntry( "MainWindow Size", size() );
    config.writeEntry( "MainWindow Position", pos() );

    delete m_playlistFiles;
    delete m_contextView;
    delete m_corona;
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
    DEBUG_BLOCK

    //this function is necessary because Amarok::actionCollection() returns our actionCollection
    //via the App::m_pMainWindow pointer since App::m_pMainWindow is not defined until
    //the above ctor returns it causes a crash unless we do the initialisation in 2 stages.

    QBoxLayout * toolbarSpacer = new QHBoxLayout( this );
    toolbarSpacer->setContentsMargins( 0, 0, 0, 10 );

    m_controlBar = new MainToolbar( 0 );
    m_controlBar->layout()->setContentsMargins( 0, 0, 0, 0 );
    m_controlBar->layout()->setSpacing( 0 );

    toolbarSpacer->addSpacing( 3 );
    toolbarSpacer->addWidget( m_controlBar, 20);
    toolbarSpacer->addSpacing( 3 );

    PERF_LOG( "Create sidebar" )
    m_browsers = new SideBar( this, new KVBox );
    m_browsers->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );

    PERF_LOG( "Create Playlist" )
    Playlist::Widget *playlistWidget = new Playlist::Widget( this );
    playlistWidget->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );
    playlistWidget->setFocus( Qt::ActiveWindowFocusReason );
    PERF_LOG( "Playlist created" )

    createMenus();

    PERF_LOG( "Creating ContextWidget" )
    m_contextWidget = new ContextWidget( this );
    PERF_LOG( "ContextWidget created" )
    m_contextWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Maximum );
    m_contextWidget->setFrameShape( QFrame::NoFrame );
    m_contextWidget->setFrameShadow( QFrame::Sunken );
    PERF_LOG( "Creating ContexScene" )

    m_corona = new Context::ContextScene( this );
    connect( m_corona, SIGNAL( containmentAdded( Plasma::Containment* ) ),
            this, SLOT( createContextView( Plasma::Containment* ) ) );

    PERF_LOG( "ContextScene created" )
#if 0
    {
        if( AmarokConfig::useCoverBling() && QGLFormat::hasOpenGL() )
            new CoverBling( m_contextWidget );
    }
#endif

    PERF_LOG( "Loading default contextScene" )
    m_corona->loadDefaultSetup(); // this method adds our containment to the scene
    PERF_LOG( "Loaded default contextScene" )

    connect( m_browsers, SIGNAL( widgetActivated( int ) ), SLOT( slotShrinkBrowsers( int ) ) );

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins( 0, 0, 0, 0 );
    mainLayout->setSpacing( 0 );

    QWidget *centralWidget = new QWidget( this );
    centralWidget->setLayout( mainLayout );

    m_splitter = new Amarok::Splitter( Qt::Horizontal, centralWidget );
    m_splitter->setHandleWidth( 0 );
    m_splitter->addWidget( m_browsers );
    m_splitter->addWidget( m_contextWidget );
    m_splitter->addWidget( playlistWidget );

    //make room for a full width statusbar at the bottom of everything
    KVBox *m_statusbarArea = new KVBox( this );
    //figure out the needed height based on system font settings
    //do make sure that it is at least 26 pixels tall though
    //or progress bars will not fit...
    QFont currentFont = font();
    currentFont.setBold( true );
    QFontMetrics fm( currentFont );
    int fontHeight = qMax( 26, fm.height() );
    m_statusbarArea->setMinimumHeight( fontHeight );
    m_statusbarArea->setMaximumHeight( fontHeight );
    //new Amarok::StatusBar( m_statusbarArea );
    new StatusBarNG( m_statusbarArea );

    mainLayout->addLayout( toolbarSpacer );
    mainLayout->addWidget( m_splitter );
    mainLayout->addWidget( m_statusbarArea);

    setCentralWidget( centralWidget );

    //<Browsers>
    {
        Debug::Block block( "Creating browsers. Please report long start times!" );

        #define addBrowserMacro( Type, name, text, icon ) { \
            Debug::Block block( name ); \
             m_browsers->addWidget( KIcon( icon ), text, new Type( name , m_browsers ) ); \
             m_browserNames.append( name ); }

        #define addInstBrowserMacro( Type, name, text, icon ) { \
             m_browsers->addWidget( KIcon( icon ), text, Type::instance() ); \
             m_browserNames.append( name ); }

        PERF_LOG( "Creating CollectionWidget" )
        addBrowserMacro( CollectionWidget, "CollectionBrowser", i18n("Collection"), "collection-amarok" )
        PERF_LOG( "Created CollectionWidget" )

        //cant use macros here since we need access to the browsers directly
        PERF_LOG( "Creating ServiceBrowser" )
        ServiceBrowser *internetContentServiceBrowser = ServiceBrowser::instance();
        internetContentServiceBrowser->setParent( this );
        m_browsers->addWidget( KIcon( "services-amarok" ), i18n("Internet"), internetContentServiceBrowser );
        m_browserNames.append( "Internet" );
        PERF_LOG( "Created ServiceBrowser" )

        m_playlistFiles = new PlaylistFileProvider();
        The::playlistManager()->addProvider( m_playlistFiles, PlaylistManager::UserPlaylist );

        PERF_LOG( "Creating PlaylistBrowser" )
        addBrowserMacro( PlaylistBrowserNS::PlaylistBrowser, "PlaylistBrowser", i18n("Playlists"), "view-media-playlist-amarok" )
        PERF_LOG( "CreatedPlaylsitBrowser" )

        PERF_LOG( "Creating FileBrowser" )
        addBrowserMacro( FileBrowser::Widget, "FileBrowser::Widget",  i18n("Files"), "folder-amarok" )
        PERF_LOG( "Created FileBrowser" )

        sideBar()->sideBarWidget()->restoreSession();


        //get the plugin manager
        ServicePluginManager::instance()->setBrowser( internetContentServiceBrowser );
        PERF_LOG( "Initialising ServicePluginManager" )
        ServicePluginManager::instance()->init();
        PERF_LOG( "Initialised ServicePluginManager" )

        internetContentServiceBrowser->setScriptableServiceManager( The::scriptableServiceManager() );
        PERF_LOG( "ScriptableServiceManager done" )
        /*new MediaBrowser( "MediaBrowser" );
        PERF_LOG( "created mediabrowser" )
        if( MediaBrowser::isAvailable() )
        {
            addInstBrowserMacro( MediaBrowser, "MediaBrowser", i18n("Devices"), "multimedia-player-amarok" )
        }*/
        #undef addBrowserMacro
        #undef addInstBrowserMacro
        PERF_LOG( "finished MainWindow::init" )
    }
    //</Browsers>

    TrackToolTip::instance(); //Instantiate
    //Amarok::MessageQueue::instance()->sendMessages();
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
    PERF_LOG( "ContexView created" )
}

void
MainWindow::deleteBrowsers()
{
    m_browsers->deleteBrowsers();
}

void
MainWindow::slotSetFilter( const QString &filter ) //SLOT
{
    Q_UNUSED( filter );
//    m_lineEdit->setText( filter );
}

void
MainWindow::slotShrinkBrowsers( int index )
{
    DEBUG_BLOCK

    // Because QSplitter sucks and will not recompute sizes if a pane is shrunk and not hidden.
    if( index == -1 )
    {
        m_splitterState = m_splitter->saveState();

        QList<int> sizes;
        sizes << m_browsers->sideBarWidget()->width() // browser bar
              << m_splitter->sizes()[1] + m_splitter->sizes()[0] - m_browsers->sideBarWidget()->width() // context view
              << m_splitter->sizes()[2]; // playlist
        m_splitter->setSizes( sizes );
    }
    else
        m_splitter->restoreState( m_splitterState );
}

void
MainWindow::addBrowser( const QString &name, QWidget *browser, const QString &text, const QString &icon )
{
    if( !m_browserNames.contains( name ) )
    {
        m_browsers->addWidget( KIcon( icon ), text, browser );
        m_browserNames.append( name );
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
    if( index >= 0 && index != m_browsers->currentIndex() )
        m_browsers->showWidget( index );
}

void
MainWindow::keyPressEvent( QKeyEvent *e )
{
    if( !( e->modifiers() & Qt::ControlModifier ) )
        return KXmlGuiWindow::keyPressEvent( e );

    int n = -1;
    switch( e->key() )
    {
        case Qt::Key_0: n = 0; break;
        case Qt::Key_1: n = 1; break;
        case Qt::Key_2: n = 2; break;
        case Qt::Key_3: n = 3; break;
        case Qt::Key_4: n = 4; break;
        default:
            return KXmlGuiWindow::keyPressEvent( e );
    }
    if( n == 0 && m_browsers->currentIndex() >= 0 )
        m_browsers->showWidget( m_browsers->currentIndex() );
    else if( n > 0 )
        showBrowser( n - 1 ); // map from human to computer counting
}

void
MainWindow::closeEvent( QCloseEvent *e )
{
#ifdef Q_WS_MAC
    Q_UNUSED( e );
    hide();
#else

    //KDE policy states we should hide to tray and not quit() when the
    //close window button is pushed for the main widget

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


void
MainWindow::showEvent( QShowEvent* )
{
}

QSize
MainWindow::sizeHint() const
{
    return QApplication::desktop()->screenGeometry( (QWidget*)this ).size() / 1.5;
}

void
MainWindow::exportPlaylist() const //SLOT
{
    KFileDialog fileDialog( KUrl("kfiledialog:///amarok-playlist-export"), QString(), 0 );
    QCheckBox *saveRelativeCheck = new QCheckBox( i18n("Use relative path for &saving") );

    fileDialog.fileWidget()->setCustomWidget( saveRelativeCheck );
    fileDialog.setOperationMode( KFileDialog::Saving );
    fileDialog.setMode( KFile::File );
    fileDialog.setCaption( i18n("Save As") );

    fileDialog.exec();

    QString playlistName = fileDialog.selectedFile();

    if( !playlistName.isEmpty() )
    {
        AmarokConfig::setRelativePlaylist( saveRelativeCheck->isChecked() );
        The::playlistModel()->exportPlaylist( playlistName );
    }
}

void
MainWindow::savePlaylist() const
{
    DEBUG_BLOCK
    //TODO make a nice dialog for choosing name and potentially parent group
    //if( !playlistName.isEmpty() )

    QString playlistName( i18n("Playlist") );
    The::playlistModel()->savePlaylist( playlistName );
}


void
MainWindow::slotBurnPlaylist() const //SLOT
{
    //K3bExporter::instance()->exportCurrentPlaylist();
}

void
MainWindow::slotShowCoverManager() const //SLOT
{
    CoverManager::showOnce();
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
    KFileDialog dlg( KUrl(QString()), QString("*.*|"), this );
    dlg.setCaption( directPlay ? i18n("Play Media (Files or URLs)") : i18n("Add Media (Files or URLs)") );
    dlg.setMode( KFile::Files | KFile::Directory );
    dlg.exec();
    files = dlg.selectedUrls();
    if( files.isEmpty() ) return;

    The::playlistController()->insertOptioned( files , Playlist::Append );
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
MainWindow::playAudioCD() //SLOT
{
    KUrl::List urls;
    if( The::engineController()->getAudioCDContents(QString(), urls) )
    {
        Meta::TrackList tracks = CollectionManager::instance()->tracksForUrls( urls );
        if( !tracks.isEmpty() )
            The::playlistController()->insertOptioned( tracks, Playlist::Replace );
    }
    else
    { // Default behaviour
        showBrowser( "FileBrowser" );
    }
}

void
MainWindow::showScriptSelector() //SLOT
{
    ScriptManager::instance()->show();
    ScriptManager::instance()->raise();
}

void
MainWindow::showQueueManager() //SLOT
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

void
MainWindow::showStatistics() //SLOT
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

void
MainWindow::slotToggleFocus() //SLOT
{
    //Port 2.0
//     if( m_browsers->currentWidget() && ( Playlist::instance()->hasFocus() /*|| m_searchWidget->lineEdit()->hasFocus()*/ ) )
//         m_browsers->currentWidget()->setFocus();
}

void
MainWindow::toolsMenuAboutToShow() //SLOT
{
    Amarok::actionCollection()->action( "equalizer" )->setEnabled( false ); //TODO phonon
}

/**
 * "Toggle Main Window" global shortcut connects to this slot
 */
void
MainWindow::showHide() //SLOT
{
    DEBUG_BLOCK

    setVisible( !isVisible() );
}

void
MainWindow::loveTrack()
{
    Meta::TrackPtr cTrack = The::engineController()->currentTrack();
    if( cTrack )
        emit loveTrack( cTrack );
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
    return isHidden();
#endif
}

void
MainWindow::createActions()
{
    KActionCollection* const ac = actionCollection();
    const EngineController* const ec = The::engineController();
    const Playlist::Actions* const pa = The::playlistActions();
    const Playlist::Controller* const pc = The::playlistController();

    KStandardAction::keyBindings( kapp, SLOT( slotConfigShortcuts() ), ac );
    KStandardAction::preferences( kapp, SLOT( slotConfigAmarok() ), ac );
    ac->action(KStandardAction::name(KStandardAction::KeyBindings))->setIcon( KIcon( "configure-shortcuts-amarok" ) );
    ac->action(KStandardAction::name(KStandardAction::Preferences))->setIcon( KIcon( "configure-amarok" ) );
    ac->action(KStandardAction::name(KStandardAction::Preferences))->setMenuRole(QAction::PreferencesRole); // Define OS X Prefs menu here, removes need for ifdef later

    KStandardAction::quit( kapp, SLOT( quit() ), ac );

    // FIXME FIXME FIXME
    // Remove this block before Final release. It is needed only to cleanup
    // global shortcut registers due to prior mishandling. Must be included in our
    // next release so that everyone with beta1/beta2 gets cleanup up.
    //
    //
    // During kde4 development amarok handled global shortcuts wrong for some
    // time. You could see this by opening 'kcmshell4 keys'. The problem was
    // change the actions objectName() after assigning a global shortcut. The
    // following code removes the duplicate global Shortcuts.
    {
        KAction action(NULL);

        action.setObjectName("addMedia");
        action.setGlobalShortcut(KShortcut());
        action.forgetGlobalShortcut();

        action.setObjectName("seekForward");
        action.setGlobalShortcut(KShortcut());
        action.forgetGlobalShortcut();

        action.setObjectName("seekBackward");
        action.setGlobalShortcut(KShortcut());
        action.forgetGlobalShortcut();

        action.setObjectName("previousTrack");
        action.setGlobalShortcut(KShortcut());
        action.forgetGlobalShortcut();

        action.setObjectName("nextTrack");
        action.setGlobalShortcut(KShortcut());
        action.forgetGlobalShortcut();

        action.setObjectName("Toggle Main Window");
        action.setGlobalShortcut(KShortcut());
        action.forgetGlobalShortcut();

        action.setObjectName("muteVolume");
        action.setGlobalShortcut(KShortcut());
        action.forgetGlobalShortcut();

        action.setObjectName("play-pause");
        action.setGlobalShortcut(KShortcut());
        action.forgetGlobalShortcut();

        action.setObjectName("showOSD");
        action.setGlobalShortcut(KShortcut());
        action.forgetGlobalShortcut();
    }

    KAction *action = new KAction( KIcon( "folder-amarok" ), i18n("&Add Media..."), this );
    ac->addAction( "playlist_add", action );
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( slotAddLocation() ) );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_A ) );

    action = new KAction( KIcon( "edit-clear-list-amarok" ), i18nc( "clear playlist", "&Clear Playlist" ), this );
    connect( action, SIGNAL( triggered( bool ) ), pc, SLOT( clear() ) );
    ac->addAction( "playlist_clear", action );

    action = new KAction( KIcon( "folder-amarok" ), i18n("&Add Stream..."), this );
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( slotAddStream() ) );
    ac->addAction( "stream_add", action );

    action = new KAction( KIcon( "document-export-amarok" ), i18n("&Export Playlist As..."), this );
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( exportPlaylist() ) );
    ac->addAction( "playlist_export", action );

    action = new KAction( KIcon( "document-save-amarok" ), i18n("&Save Playlist"), this );
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( savePlaylist() ) );
    ac->addAction( "playlist_save", action );

    /*
    action = new KAction( KIcon( "tools-media-optical-burn-amarok" ), i18n( "Burn Current Playlist" ), this );
    connect( action, SIGNAL( triggered(bool) ), SLOT( slotBurnPlaylist() ) );
    action->setEnabled( K3bExporter::isAvailable() );
    ac->addAction( "playlist_burn", action );
    */

    action = new KAction( KIcon( "media-album-cover-manager-amarok" ), i18n( "Cover Manager" ), this );
    connect( action, SIGNAL( triggered(bool) ), SLOT( slotShowCoverManager() ) );
    ac->addAction( "cover_manager", action );

    action = new KAction( KIcon( "view-media-visualization-amarok" ), i18n("&Visualizations"), this );
    // connect( visuals, SIGNAL( triggered(bool) ), Vis::Selector::instance(), SLOT( show() ) );
    ac->addAction( "visualizations", action );

    action = new KAction( KIcon( "view-media-equalizer-amarok" ), i18n( "E&qualizer"), this );
    connect( action, SIGNAL( triggered(bool) ), kapp, SLOT( slotConfigEqualizer() ) );
    ac->addAction( "equalizer", action );

//     KAction *update_podcasts = new KAction( this );
//     update_podcasts->setText( i18n( "Update Podcasts" ) );
//     //update_podcasts->setIcon( KIcon("view-refresh-amarok") );
//     ac->addAction( "podcasts_update", update_podcasts );
//     connect(update_podcasts, SIGNAL(triggered(bool)),
//             The::podcastCollection(), SLOT(slotUpdateAll()));

    action = new KAction( KIcon("folder-amarok"), i18n("Play Media..."), this );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotPlayMedia()));
    ac->addAction( "playlist_playmedia", action );

    action = new KAction( KIcon( "media-optical-audio-amarok" ), i18n("Play Audio CD"), this );
    connect(action, SIGNAL(triggered(bool)), SLOT(playAudioCD()));
    ac->addAction( "play_audiocd", action );

    action = new KAction( KIcon("preferences-plugin-script-amarok"), i18n("Script Manager"), this );
    connect(action, SIGNAL(triggered(bool)), SLOT(showScriptSelector()));
    ac->addAction( "script_manager", action );

    action = new KAction( KIcon( "go-bottom-amarok"), i18n( "Queue Manager" ), this );
    connect(action, SIGNAL(triggered(bool)), SLOT(showQueueManager()));
    ac->addAction( "queue_manager", action );

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

    action = new KAction( KIcon("view-statistics-amarok"), i18n( "Statistics" ), this );
    connect(action, SIGNAL(triggered(bool)), SLOT(showStatistics()));
    ac->addAction( "statistics", action );

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

    action = new KAction(i18n( "Toggle Focus" ), this);
    action->setShortcut( Qt::ControlModifier + Qt::Key_Tab );
    connect( action, SIGNAL(triggered(bool)), SLOT( slotToggleFocus() ));

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

    action = new KAction( i18n( "Show On Screen Display" ), this );
    ac->addAction( "showOsd", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_O ) );
    connect( action, SIGNAL( triggered() ), Amarok::OSD::instance(), SLOT( forceToggleOSD() ) );

    action = new KAction( i18n( "Mute Volume" ), this );
    ac->addAction( "mute", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_M ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( mute() ) );

    action = new KAction( i18n( "Love Current Track" ), this );
    ac->addAction( "loveTrack", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_L ) );
    connect( action, SIGNAL(triggered()), SLOT(loveTrack()) );

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
    new Amarok::PlayPauseAction( ac, this );
    new Amarok::RepeatAction( ac, this );
    new Amarok::RandomAction( ac, this );
    new Amarok::FavorAction( ac, this );

    /*
    PERF_LOG( "MainWindow::createActions 9" )
    if( K3bExporter::isAvailable() )
        new Amarok::BurnMenuAction( ac );
    */

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
    extern void qt_mac_set_dock_menu(QMenu *);
    qt_mac_set_dock_menu(actionsMenu);
    // Change to avoid duplicate menu titles in OS X
    actionsMenu->setTitle( i18n("&Music") );
#else
    m_menubar = menuBar();
    actionsMenu = new KMenu( m_menubar );
    actionsMenu->setTitle( i18n("&Amarok") );
#endif
    actionsMenu->addAction( actionCollection()->action("playlist_playmedia") );
    actionsMenu->addAction( actionCollection()->action("play_audiocd") );
    actionsMenu->addSeparator();
    actionsMenu->addAction( actionCollection()->action("prev") );
    actionsMenu->addAction( actionCollection()->action("play_pause") );
    actionsMenu->addAction( actionCollection()->action("stop") );
    actionsMenu->addAction( actionCollection()->action("next") );


#ifndef Q_WS_MAC    // Avoid duplicate "Quit" in OS X dock menu
    actionsMenu->addSeparator();
    actionsMenu->addAction( actionCollection()->action(KStandardAction::name(KStandardAction::Quit)) );
#endif
    //END Actions menu

    //BEGIN Playlist menu
    KMenu *playlistMenu = new KMenu( m_menubar );
    playlistMenu->setTitle( i18n("&Playlist") );
    playlistMenu->addAction( actionCollection()->action("playlist_add") );
    playlistMenu->addAction( actionCollection()->action("stream_add") );
    playlistMenu->addAction( actionCollection()->action("playlist_save") );
    playlistMenu->addAction( actionCollection()->action("playlist_burn") );
    playlistMenu->addAction( actionCollection()->action("podcasts_update") );
    playlistMenu->addSeparator();
    playlistMenu->addAction( actionCollection()->action("playlist_undo") );
    playlistMenu->addAction( actionCollection()->action("playlist_redo") );
    playlistMenu->addSeparator();
    playlistMenu->addAction( actionCollection()->action("playlist_clear") );
    playlistMenu->addAction( actionCollection()->action("playlist_shuffle") );

    QAction *repeat = actionCollection()->action("repeat");
    playlistMenu->addAction( repeat );

    KSelectAction *random = static_cast<KSelectAction*>( actionCollection()->action("random_mode") );
    playlistMenu->addAction( random );
    random->menu()->addSeparator();
    random->menu()->addAction( actionCollection()->action("favor_tracks") );

    playlistMenu->addSeparator();
    //FIXME: REENABLE When ported
//     playlistMenu->addAction( actionCollection()->action("queue_selected") );
    playlistMenu->addAction( actionCollection()->action("playlist_remove_duplicates") );
    playlistMenu->addAction( actionCollection()->action("playlist_select_all") );

    //END Playlist menu

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

    m_settingsMenu->addAction( actionCollection()->action(KStandardAction::name(KStandardAction::ConfigureToolbars)) );
    m_settingsMenu->addAction( actionCollection()->action(KStandardAction::name(KStandardAction::KeyBindings)) );
    m_settingsMenu->addAction( actionCollection()->action(KStandardAction::name(KStandardAction::Preferences)) );

    connect( m_settingsMenu, SIGNAL( activated(int) ), SLOT( slotMenuActivated(int) ) );
    //END Settings menu

    m_menubar->addMenu( actionsMenu );
    m_menubar->addMenu( playlistMenu );
    m_menubar->addMenu( m_toolsMenu );
    m_menubar->addMenu( m_settingsMenu );
    m_menubar->addMenu( Amarok::Menu::helpMenu() );
}

void
MainWindow::paletteChange(const QPalette & oldPalette)
{
    Q_UNUSED( oldPalette )

    QPixmapCache::clear();
    The::paletteHandler()->setPalette( palette() );
}

QSize
MainWindow::backgroundSize()
{
    QPoint topLeft = mapToGlobal( QPoint( 0, 0 ) );
    QPoint bottomRight1 = mapToGlobal( QPoint( width(), height() ) );

    return QSize( bottomRight1.x() - topLeft.x() + 1, bottomRight1.y() - topLeft.y() );
}

int
MainWindow::contextXOffset()
{
    QPoint topLeft1 = mapToGlobal( m_controlBar->pos() );
    QPoint topLeft2 = mapToGlobal( m_contextWidget->pos() );

    return topLeft2.x() - topLeft1.x();
}

void MainWindow::resizeEvent( QResizeEvent * event )
{
    QWidget::resizeEvent( event );
    m_controlBar->reRender();
}

QPoint MainWindow::globalBackgroundOffset()
{
    return menuBar()->mapToGlobal( QPoint( 0, 0 ) );
}

QRect MainWindow::contextRectGlobal()
{
    //debug() << "pos of context vidget within main window is: " << m_contextWidget->pos();
    QPoint contextPos = m_splitter->mapToGlobal( m_contextWidget->pos() );
    return QRect( contextPos.x(), contextPos.y(), m_contextWidget->width(), m_contextWidget->height() );
}

void MainWindow::engineStateChanged( Phonon::State state, Phonon::State oldState )
{
    Q_UNUSED( oldState )

    Meta::TrackPtr track = The::engineController()->currentTrack();
    //track is 0 if the engien state is Empty. we check that in the switch
    switch( state )
    {
    case Phonon::StoppedState:
        setPlainCaption( i18n( AMAROK_CAPTION ) );
        break;

    case Phonon::PlayingState:
        if ( track && !track->prettyName().isEmpty() ) {
            unsubscribeFrom( m_currentTrack );
            m_currentTrack = track;
            subscribeTo( track );
            setPlainCaption( i18n( "%1 - %2  ::  %3", track->artist() ? track->artist()->prettyName() : i18n( "Unknown" ), track->prettyName(), AMAROK_CAPTION ) );
        }
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

void MainWindow::metadataChanged( Meta::Track *track )
{
    DEBUG_BLOCK

    setPlainCaption( i18n( "%1 - %2  ::  %3", track->artist() ? track->artist()->prettyName() : i18n( "Unknown" ), track->prettyName(), AMAROK_CAPTION ) );
}
 
namespace The {
    MainWindow* mainWindow() { return MainWindow::s_instance; }
}

#include "MainWindow.moc"

