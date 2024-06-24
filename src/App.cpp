/****************************************************************************************
 * Copyright (c) 2002 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "App.h"

#include <config.h>
#include "EngineController.h"
#include "KNotificationBackend.h"
#include "MainWindow.h"
#include "PluginManager.h"
#include "scripting/scriptmanager/ScriptManager.h"
#include "TrayIcon.h"
#include "amarokconfig.h"
#include "aboutdialog/OcsData.h"
#include "amarokurls/AmarokUrl.h"
#include "configdialog/ConfigDialog.h"
#include "configdialog/dialogs/PlaybackConfig.h"
#include "core/capabilities/SourceInfoCapability.h"
#include "core/logger/Logger.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "core/meta/support/MetaUtility.h"
#include "core/playlists/Playlist.h"
#include "core/playlists/PlaylistFormat.h"
#include "core/podcasts/PodcastProvider.h"
#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core/transcoding/TranscodingController.h"
#include "core-impl/collections/support/CollectionManager.h"
#ifdef DEBUG_BUILD_TYPE
#include "core-impl/logger/DebugLogger.h"
#endif // DEBUG_BUILD_TYPE
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "core-impl/storage/StorageManager.h"
#include "covermanager/CoverCache.h"
#include "covermanager/CoverFetcher.h"
#include "dbus/CollectionDBusHandler.h"
#include "dbus/mpris2/Mpris2.h"
#include "network/NetworkAccessManagerProxy.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModelStack.h"
#include "playlistmanager/PlaylistManager.h"
#include "services/ServicePluginManager.h"
#include "scripting/scriptconsole/ScriptConsole.h"
#include "statemanagement/ApplicationController.h"
#include "statemanagement/DefaultApplicationController.h"
#include "statsyncing/Controller.h"
#include "widgets/Osd.h"

#include <iostream>

#include <KDirLister>
#include <KEditToolBar>                  //slotConfigToolbars()
#include <KIO/CopyJob>
#include <KJobUiDelegate>
#include <KLocalizedString>
#include <KMessageBox>
#include <KShortcutsDialog>              //slotConfigShortcuts()
#include <ThreadWeaver/Queue>

#include <QAction>
#include <QByteArray>
#include <QCommandLineParser>
#include <QDesktopServices>
#include <QFile>
#include <QFileOpenEvent>
#include <QStringList>
#include <QTimer>                       //showHyperThreadingWarning()

#ifdef Q_OS_APPLE
#include <CoreFoundation/CoreFoundation.h>
extern void setupEventHandler_mac(SRefCon);
#endif

AMAROK_EXPORT OcsData ocsData;

App::App(int &argc, char **argv)
    : QApplication(argc, argv)
    , m_tray(nullptr)
    , m_args(nullptr)
{
    DEBUG_BLOCK
    PERF_LOG( "Begin Application ctor" )

    KLocalizedString::setApplicationDomain("amarok");

    // required for last.fm plugin to grab app version
    setApplicationVersion( AMAROK_VERSION );

    qRegisterMetaType<Meta::DataPtr>();
    qRegisterMetaType<Meta::DataList>();
    qRegisterMetaType<Meta::TrackPtr>();
    qRegisterMetaType<Meta::TrackList>();
    qRegisterMetaType<Meta::AlbumPtr>();
    qRegisterMetaType<Meta::AlbumList>();
    qRegisterMetaType<Meta::ArtistPtr>();
    qRegisterMetaType<Meta::ArtistList>();
    qRegisterMetaType<Meta::GenrePtr>();
    qRegisterMetaType<Meta::GenreList>();
    qRegisterMetaType<Meta::ComposerPtr>();
    qRegisterMetaType<Meta::ComposerList>();
    qRegisterMetaType<Meta::YearPtr>();
    qRegisterMetaType<Meta::YearList>();
    qRegisterMetaType<Meta::LabelPtr>();
    qRegisterMetaType<Meta::LabelList>();
    qRegisterMetaType<Playlists::PlaylistPtr>();
    qRegisterMetaType<Playlists::PlaylistList>();

#ifdef Q_OS_APPLE
    // this is inspired by OpenSceneGraph: osgDB/FilePath.cpp

    // Start with the Bundle PlugIns directory.

    // Get the main bundle first. No need to retain or release it since
    //  we are not keeping a reference
    CFBundleRef myBundle = CFBundleGetMainBundle();
    if( myBundle )
    {
        // CFBundleGetMainBundle will return a bundle ref even if
        //  the application isn't part of a bundle, so we need to
        //  check
        //  if the path to the bundle ends in ".app" to see if it is
        //  a
        //  proper application bundle. If it is, the plugins path is
        //  added
        CFURLRef urlRef = CFBundleCopyBundleURL(myBundle);
        if(urlRef)
        {
            char bundlePath[1024];
            if( CFURLGetFileSystemRepresentation( urlRef, true, (UInt8 *)bundlePath, sizeof(bundlePath) ) )
            {
                QByteArray bp( bundlePath );
                size_t len = bp.length();
                if( len > 4 && bp.right( 4 ) == ".app" )
                {
                    bp.append( "/Contents/MacOS" );
                    QByteArray path = qgetenv( "PATH" );
                    if( path.length() > 0 )
                    {
                        path.prepend( ":" );
                    }
                    path.prepend( bp );
                    debug() << "setting PATH=" << path;
                    setenv("PATH", path, 1);
                }
            }
            // docs say we are responsible for releasing CFURLRef
            CFRelease(urlRef);
        }
    }

    setupEventHandler_mac(this);
#endif

    PERF_LOG( "Done App ctor" )
}

App::~App()
{
    DEBUG_BLOCK

    //delete m_args;
    CollectionManager::instance()->stopScan();

    // Hiding the OSD before exit prevents crash
    Amarok::OSD::instance()->hide();

    // This following can't go in the PlaylistModel destructor, because by the time that
    // happens, the Config has already been written.

    // Use the bottom model because that provides the most dependable/invariable row
    // number to save in an external file.
    AmarokConfig::setLastPlaying( Playlist::ModelStack::instance()->bottom()->activeRow() );

    if ( AmarokConfig::resumePlayback() )
    {
        Meta::TrackPtr engineTrack = The::engineController()->currentTrack();
        if( engineTrack )
        {
            AmarokConfig::setResumeTrack( engineTrack->playableUrl().toDisplayString() );
            AmarokConfig::setResumeTime( The::engineController()->trackPositionMs() );
            AmarokConfig::setResumePaused( The::engineController()->isPaused() );
        }
        else
            AmarokConfig::setResumeTrack( QString() ); //otherwise it'll play previous resume next time!
    }

    The::engineController()->endSession(); //records final statistics

#ifndef Q_OS_APPLE
    // do even if trayicon is not shown, it is safe
    Amarok::config().writeEntry( "HiddenOnExit", mainWindow()->isHidden() );
    AmarokConfig::self()->save();
#else
    // for some reason on OS X the main window always reports being hidden
    // this means if you have the tray icon enabled, amarok will always open minimized
    Amarok::config().writeEntry( "HiddenOnExit", false );
    AmarokConfig::self()->save();
#endif

    // wait for threads to finish
    ThreadWeaver::Queue::instance()->requestAbort();
    ThreadWeaver::Queue::instance()->finish();
    ThreadWeaver::Queue::instance()->shutDown();

    ScriptManager::destroy();

    // this must be deleted before the connection to the Xserver is
    // severed, or we risk a crash when the QApplication is exited,
    // I asked Trolltech! *smug*
    Amarok::OSD::destroy();
    Amarok::KNotificationBackend::destroy();

    AmarokConfig::self()->save();

    delete m_mainWindow.data();

    Playlist::Controller::destroy();
    Playlist::ModelStack::destroy();
    Playlist::Actions::destroy();
    PlaylistManager::destroy();
    CoverFetcher::destroy();
    CoverCache::destroy();
    ServicePluginManager::destroy();
    CollectionManager::destroy();
    StorageManager::destroy();
    NetworkAccessManagerProxy::destroy();
    Plugins::PluginManager::destroy();

    //this should be moved to App::quit() I guess
    Amarok::Components::applicationController()->shutdown();

#ifdef Q_OS_WINDOWS
    // work around for KUniqueApplication being not completely implemented on windows
    QDBusConnectionInterface* dbusService;
    if (QDBusConnection::sessionBus().isConnected() && (dbusService = QDBusConnection::sessionBus().interface()))
    {
        dbusService->unregisterService("org.mpris.amarok");
        dbusService->unregisterService("org.mpris.MediaPlayer2.amarok");
    }
#endif
}

void
App::handleCliArgs(const QString &cwd)
{
    DEBUG_BLOCK

    //TODO Resolve positional arguments using cwd
    if( m_args->isSet( "cwd" ) ) {
        m_cwd = m_args->value( "cwd" );
    } else {
        m_cwd = cwd;
    }

    bool haveArgs = true; // assume having args in first place
    if( !m_args->positionalArguments().isEmpty() )
    {
        QList<QUrl> list;
        for( int i = 0; i < m_args->positionalArguments().count() ; i++ )
        {
            QUrl url( QUrl::fromUserInput( m_args->positionalArguments().at( i ), QString(), QUrl::AssumeLocalFile ) );
            if( Podcasts::PodcastProvider::couldBeFeed( url.url() ) )
            {
                QUrl feedUrl = Podcasts::PodcastProvider::toFeedUrl( url.url() );
                The::playlistManager()->defaultPodcasts()->addPodcast( feedUrl );
            }
            else if( url.scheme() == "amarok" )
                s_delayedAmarokUrls.append( url.url() );
            else
                list << url;
        }

        Playlist::AddOptions options;
        if( m_args->isSet( "queue" ) )
           options = Playlist::OnQueueToPlaylistAction;
        else if( m_args->isSet( "append" ) )
           options = Playlist::OnAppendToPlaylistAction;
        else if( m_args->isSet( "load" ) )
            options = Playlist::OnReplacePlaylistAction;
        else
            options = Playlist::OnPlayMediaAction;

        The::playlistController()->insertOptioned( list, options );
    }
    else if ( m_args->isSet( "cdplay" ) )
        The::mainWindow()->playAudioCd();

    //we shouldn't let the user specify two of these since it is pointless!
    //so we prioritise, pause > stop > play > next > prev
    //thus pause is the least destructive, followed by stop as brakes are the most important bit of a car(!)
    //then the others seemed sensible. Feel free to modify this order, but please leave justification in the cvs log
    //I considered doing some sanity checks (eg only stop if paused or playing), but decided it wasn't worth it
    else if ( m_args->isSet( "pause" ) )
        The::engineController()->pause();
    else if ( m_args->isSet( "stop" ) )
        The::engineController()->stop();
    else if ( m_args->isSet( "play-pause" ) )
        The::engineController()->playPause();
    else if ( m_args->isSet( "play" ) ) //will restart if we are playing
        The::engineController()->play();
    else if ( m_args->isSet( "next" ) )
        The::playlistActions()->next();
    else if ( m_args->isSet( "previous" ) )
        The::playlistActions()->back();
    else // no args given
        haveArgs = false;

    static bool firstTime = true;

    //allows debugging on OS X. Bundles have to be started with "open". Therefore it is not possible to pass an argument
    const bool forceDebug = Amarok::config().readEntry( "Force Debug", false );

    if( firstTime && !Debug::debugEnabled() && !forceDebug )
    {
        qDebug() << "**********************************************************************************************";
        qDebug() << "** AMAROK WAS STARTED IN NORMAL MODE. IF YOU WANT TO SEE DEBUGGING INFORMATION, PLEASE USE: **";
        qDebug() << "** amarok --debug                                                                           **";
        qDebug() << "**********************************************************************************************";
    }

    if( !firstTime && !haveArgs )
    {
        // mainWindow() can be 0 if another instance is loading, see https://bugs.kde.org/show_bug.cgi?id=202713
        if( pApp->mainWindow() )
            pApp->mainWindow()->activate();
    }

    firstTime = false;
    m_args->clearPositionalArguments();    //free up memory
}


/////////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////////

void
App::initCliArgs(QCommandLineParser *parser)
{
    m_args = parser;
    // Update main.cpp (below KUniqueApplication::start() wrt instanceOptions) aswell if needed!
    QList<QCommandLineOption> options;

    options.append(QCommandLineOption("+[URL(s)]", i18n( "Files/URLs to open" )));
    options.append(QCommandLineOption("cdplay", i18n("Immediately start playing an audio cd")));
    options.append(QCommandLineOption(QStringList() << "r" << "previous", i18n( "Skip backwards in playlist" )));
    options.append(QCommandLineOption(QStringList() << "p" << "play", i18n( "Start playing current playlist" )));
    options.append(QCommandLineOption(QStringList() << "t" << "play-pause", i18n( "Play if stopped, pause if playing" )));
    options.append(QCommandLineOption("pause", i18n( "Pause playback" )));
    options.append(QCommandLineOption(QStringList() << "s" << "stop", i18n( "Stop playback" )));
    options.append(QCommandLineOption(QStringList() << "f" << "next", i18n( "Skip forwards in playlist" )
    + "\n\n\n"
    + i18n("Additional options:")));
    options.append(QCommandLineOption(QStringList() << "a" << "append", i18n( "Append files/URLs to playlist" )));
    options.append(QCommandLineOption("queue", i18n("Queue URLs after the currently playing track")));
    options.append(QCommandLineOption(QStringList() << "l" << "load", i18n("Load URLs, replacing current playlist")));
    options.append(QCommandLineOption(QStringList() << "d" << "debug", i18n("Print verbose debugging information")));
    // HACK should https://github.com/lastfm/liblastfm/pull/37 ever get merged, this option can be removed
    options.append(QCommandLineOption("debug-with-lastfm", i18n("Print verbose debugging information")));
    options.append(QCommandLineOption("debug-audio", i18n("Print verbose debugging information from the audio system")));
    options.append(QCommandLineOption(QStringList() << "c" << "coloroff", i18n("Disable colorization for debug output.")));
    options.append(QCommandLineOption(QStringList() << "m" << "multipleinstances", i18n("Allow running multiple Amarok instances")));
    options.append(QCommandLineOption("cwd", i18n( "Base for relative filenames/URLs" )));

    parser->addOptions(options);      //add our own options
}


/////////////////////////////////////////////////////////////////////////////////////
// METHODS
/////////////////////////////////////////////////////////////////////////////////////


//SLOT
void App::applySettings()
{
    DEBUG_BLOCK

    if( AmarokConfig::showTrayIcon() && ! m_tray )
    {
        m_tray = new Amarok::TrayIcon( m_mainWindow.data() );
    }
    else if( !AmarokConfig::showTrayIcon() && m_tray )
    {
        delete m_tray;
        m_tray = nullptr;
    }

    Amarok::OSD::instance()->applySettings();

    Q_EMIT settingsChanged();

    if( AmarokConfig::enableScriptConsole() && !m_scriptConsole )
        m_scriptConsole = ScriptConsoleNS::ScriptConsole::instance();
    else if( !AmarokConfig::enableScriptConsole() && m_scriptConsole )
        m_scriptConsole.data()->deleteLater();
}

//SLOT
void App::applySettingsFirstTime()
{
    DEBUG_BLOCK

    if( AmarokConfig::showTrayIcon() && ! m_tray )
    {
        m_tray = new Amarok::TrayIcon( m_mainWindow.data() );
    }
    else if( !AmarokConfig::showTrayIcon() && m_tray )
    {
        delete m_tray;
        m_tray = nullptr;
    }

    if( AmarokConfig::enableScriptConsole() && !m_scriptConsole )
        m_scriptConsole = ScriptConsoleNS::ScriptConsole::instance();
    else if( !AmarokConfig::enableScriptConsole() && m_scriptConsole )
        m_scriptConsole.data()->deleteLater();
}

//SLOT
void
App::continueInit()
{
    DEBUG_BLOCK

    PERF_LOG( "Begin App::continueInit" )

    AmarokConfig::instance( "amarokrc" );
    newInstance();

    const bool restoreSession = m_args->positionalArguments().isEmpty() || m_args->isSet( "append" )
                                || m_args->isSet( "queue" )
                                || Amarok::config().readEntry( "AppendAsDefault", false );

#ifdef DEBUG_BUILD_TYPE
    new DebugLogger( this );
#endif // DEBUG_BUILD_TYPE

    new Amarok::DefaultApplicationController( this );
    Amarok::Components::applicationController()->start();

    // Instantiate statistics synchronization controller. Needs to be before creating
    // MainWindow as MainWindow connects a signal to StatSyncing::Controller.
    Amarok::Components::setStatSyncingController( new StatSyncing::Controller( this ) );

    PERF_LOG( "Creating MainWindow" )
    m_mainWindow = new MainWindow();
    PERF_LOG( "Done creating MainWindow" )

    if( AmarokConfig::showTrayIcon() )
        m_tray = new Amarok::TrayIcon( m_mainWindow );

    PERF_LOG( "Creating DBus handlers" )
    QDBusConnection::sessionBus().registerService("org.mpris.amarok");
    new CollectionDBusHandler( this );
    new Amarok::Mpris2( this );
    PERF_LOG( "Done creating DBus handlers" )

    //DON'T DELETE THIS NEXT LINE or the app crashes when you click the X (unless we reimplement closeEvent)
    //Reason: in ~App we have to call the deleteBrowsers method or else we run afoul of refcount foobar in KHTMLPart
    //But if you click the X (not Action->Quit) it automatically kills MainWindow because KMainWindow sets this
    //for us as default (bad KMainWindow)
    m_mainWindow->setAttribute( Qt::WA_DeleteOnClose, false );
    //init playlist window as soon as the database is guaranteed to be usable

    // Create engine, show TrayIcon etc.
    applySettingsFirstTime();
    // Must be created _after_ MainWindow.
    PERF_LOG( "Starting ScriptManager" )
    ScriptManager::instance();
    PERF_LOG( "ScriptManager started" )

    The::engineController()->setVolume( AmarokConfig::masterVolume() );
    The::engineController()->setMuted( AmarokConfig::muteState() );

    Amarok::KNotificationBackend::instance()->setEnabled( AmarokConfig::kNotifyEnabled() );
    Amarok::OSD::instance()->applySettings(); // Create after setting volume (don't show OSD for that)

    // Restore keyboard shortcuts etc from config
    Amarok::actionCollection()->readSettings();
    //on startup we need to show the window, but only if it wasn't hidden on exit
    //and always if the trayicon isn't showing
    if( !Amarok::config().readEntry( "HiddenOnExit", false ) || !AmarokConfig::showTrayIcon() )
    {
        PERF_LOG( "showing main window again" )
        m_mainWindow->show();
        PERF_LOG( "after showing mainWindow" )
    }

    //Instantiate the Transcoding::Controller, this fires up an asynchronous KProcess with
    //FFmpeg which should not take more than ~200msec.
    Amarok::Components::setTranscodingController( new Transcoding::Controller( this ) );

    PERF_LOG( "App init done" )

    // check that the amarok sql configuration is valid.
    if( !StorageManager::instance()->getLastErrors().isEmpty() )
    {
        QMessageBox::critical( The::mainWindow(), i18n( "Database Error" ),
                 i18n( "The Amarok database reported the following errors:"
                 "\n%1\nIn most cases you will need to resolve these errors "
                 "before Amarok will run properly.",
                 StorageManager::instance()->getLastErrors().join( "\n" ) ) );
        StorageManager::instance()->clearLastErrors();
        slotConfigAmarok( "DatabaseConfig" );
    }
    else
    {
        handleFirstRun();
    }

    if( AmarokConfig::resumePlayback() && restoreSession && !m_args->isSet( "stop" ) ) {
        //restore session as long as the user didn't specify media to play etc.
        //do this after applySettings() so OSD displays correctly
        The::engineController()->restoreSession();
    }
    //and now we can run any amarokurls provided on startup, as all components should be initialized by now!
    for( const QString& urlString : s_delayedAmarokUrls )
    {
        AmarokUrl aUrl( urlString );
        aUrl.run();
    }
    
    s_delayedAmarokUrls.clear();
}

//SLOT
void App::activateRequested(const QStringList &arguments, const QString & cwd)
{
        qDebug() << "activateRequested";
    if (!arguments.isEmpty()) {
        m_args->parse(arguments);
        handleCliArgs(cwd);
    } else {
        newInstance( );
    }
}

void App::slotConfigAmarok( const QString& page )
{
    KConfigDialog *dialog = KConfigDialog::exists( "settings" );
    if( !dialog )
    {
        //KConfigDialog didn't find an instance of this dialog, so lets create it :
        dialog = new Amarok2ConfigDialog( mainWindow(), "settings", AmarokConfig::self() );

        connect( dialog, &KConfigDialog::settingsChanged,
                 this, &App::applySettings );
    }
    static_cast<Amarok2ConfigDialog*>( dialog )->show( page );
}

void App::slotConfigAmarokWithEmptyPage()
{
    slotConfigAmarok( QString() );
}

void App::slotConfigShortcuts()
{
    KShortcutsDialog::configure( Amarok::actionCollection(), KShortcutsEditor::LetterShortcutsAllowed, mainWindow() );
    AmarokConfig::self()->save();
}

KIO::Job *App::trashFiles( const QList<QUrl> &files )
{
    KIO::Job *job = KIO::trash( files );
    Amarok::Logger::newProgressOperation( job, i18n("Moving files to trash") );
    connect( job, &KIO::Job::result, this, &App::slotTrashResult );
    return job;
}

void App::slotTrashResult( KJob *job )
{
    if( job->error() )
        job->uiDelegate()->showErrorMessage();
}

void App::quit()
{
    DEBUG_BLOCK
    The::playlistManager()->completePodcastDownloads();

    // Following signal is relayed to scripts, which may block quitting for a while
    Q_EMIT prepareToQuit();
    QApplication::quit();
}

bool App::event( QEvent *event )
{
    switch( event->type() )
    {
        //allows Amarok to open files from the finder on OS X
        case QEvent::FileOpen:
        {
            QString file = static_cast<QFileOpenEvent*>( event )->file();
            The::playlistController()->insertOptioned( QUrl( file ), Playlist::OnPlayMediaAction );
            return true;
        }
        default:
            return QApplication::event( event );
    }
}

int App::newInstance()
{
    DEBUG_BLOCK

    static bool first = true;
    if ( isSessionRestored() && first )
    {
        first = false;
        return 0;
    }

    first = false;

    handleCliArgs(QDir::currentPath());
    return 0;
}

void App::handleFirstRun()
{
    KConfigGroup config = Amarok::config( "General" );
    if( !config.readEntry( "First Run", true ) )
        return;

    const QString musicDir = QStandardPaths::writableLocation( QStandardPaths::MusicLocation );
    const QDir dir( musicDir );

    int result = KMessageBox::No;
    if( dir.exists() && dir.isReadable() )
    {
        result = KMessageBox::questionYesNoCancel( m_mainWindow, i18n( "A music path, "
                "%1, is set in System Settings.\nWould you like to use that as a "
                "collection folder?", musicDir ) );
    }

    switch( result )
    {
        case KMessageBox::Yes:
        {
            Collections::Collection *coll = CollectionManager::instance()->primaryCollection();
            if( coll )
            {
                coll->setProperty( "collectionFolders", QStringList() << musicDir );
                CollectionManager::instance()->startFullScan();
            }
            break;
        }
        case KMessageBox::No:
            slotConfigAmarok( "CollectionConfig" );
            break;
        default:
            break;
    }

    config.writeEntry( "First Run", false );
}
