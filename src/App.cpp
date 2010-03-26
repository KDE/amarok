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

#include <config-amarok.h>

#include "Amarok.h"
#include "amarokconfig.h"
#include "amarokurls/AmarokUrl.h"
#include "CollectionManager.h"
#include "Components.h"
#include "ConfigDialog.h"
#include "covermanager/CoverFetcher.h"
#include "dialogs/EqualizerDialog.h"
#include "dbus/CollectionDBusHandler.h"
#include "Debug.h"
#include "EngineController.h"
#include "firstruntutorial/FirstRunTutorial.h"
#include "KNotificationBackend.h"
#include "core/capabilities/SourceInfoCapability.h"
#include "core/meta/support/MetaConstants.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "Osd.h"
#include "PlaybackConfig.h"
#include "PlayerDBusHandler.h"
#include "core/playlists/Playlist.h"
#include "core/playlists/impl/file/PlaylistFileSupport.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModelStack.h"
#include "playlist/PlaylistController.h"
#include "playlistmanager/PlaylistManager.h"
#include "core/plugins/PluginManager.h"
#include "podcasts/PodcastProvider.h"
#include "RootDBusHandler.h"
#include "ScriptManager.h"
#include "statemanagement/ApplicationController.h"
#include "statemanagement/DefaultApplicationController.h"
#include "statusbar/StatusBar.h"
#include "TracklistDBusHandler.h"
#ifdef HAVE_KSTATUSNOTIFIERITEM
#include "TrayIcon.h"
#else
#include "TrayIconLegacy.h"
#endif

#ifdef NO_MYSQL_EMBEDDED
#include "MySqlServerTester.h"
#endif

#include <iostream>

#include <KAction>
#include <KCalendarSystem>
#include <KCmdLineArgs>                  //initCliArgs()
#include <KDirLister>
#include <KEditToolBar>                  //slotConfigToolbars()
#include <KGlobalSettings>
#include <KIO/CopyJob>
#include <KJob>
#include <KJobUiDelegate>
#include <KLocale>
#include <KMessageBox>
#include <KShortcutsDialog>              //slotConfigShortcuts()
#include <KStandardDirs>

#include <QByteArray>
#include <QDesktopServices>
#include <QFile>
#include <KPixmapCache>
#include <QStringList>
#include <QTextDocument>                // for Qt::escape()
#include <QTimer>                       //showHyperThreadingWarning()
#include <QtDBus/QtDBus>

#include "shared/taglib_filetype_resolvers/asffiletyperesolver.h"
#include "shared/taglib_filetype_resolvers/mimefiletyperesolver.h"
#include "shared/taglib_filetype_resolvers/mp4filetyperesolver.h"
#include "shared/taglib_filetype_resolvers/wavfiletyperesolver.h"
#include <audiblefiletyperesolver.h>
#include <realmediafiletyperesolver.h>

QMutex Debug::mutex;
int App::mainThreadId = 0;

#ifdef Q_WS_MAC
#include <CoreFoundation/CoreFoundation.h>
extern void setupEventHandler_mac(long);
#endif

#ifdef DEBUG
#include "TestDirectoryLoader.h"
#endif // DEBUG

AMAROK_EXPORT KAboutData aboutData( "amarok", 0,
    ki18n( "Amarok" ), AMAROK_VERSION,
    ki18n( "The audio player for KDE" ), KAboutData::License_GPL,
    ki18n( "(C) 2002-2003, Mark Kretschmann\n(C) 2003-2010, The Amarok Development Squad" ),
    ki18n( "IRC:\nirc.freenode.net - #amarok, #amarok.de, #amarok.es, #amarok.fr\n\nFeedback:\namarok@kde.org\n\n(Build Date: %1)" ).subs( __DATE__ ),
             ( "http://amarok.kde.org" ) );

AMAROK_EXPORT OcsData ocsData( "opendesktop" );

App::App()
        : KUniqueApplication()
        , m_tray(0)
{
    DEBUG_BLOCK
    PERF_LOG( "Begin Application ctor" )

    // required for last.fm plugin to grab app version
    setApplicationVersion( AMAROK_VERSION );

    PERF_LOG( "Registering taglib plugins" )
    TagLib::FileRef::addFileTypeResolver(new RealMediaFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new AudibleFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new WAVFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new MP4FileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new ASFFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new MimeFileTypeResolver);
    PERF_LOG( "Done Registering taglib plugins" )

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


    //make sure we have enough cache space for all our crazy svg stuff
    KPixmapCache cache( "Amarok-pixmaps" );
    cache.setCacheLimit ( 20 * 1024 );

#ifdef Q_WS_MAC
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

    setupEventHandler_mac((long)this);
#endif

    PERF_LOG( "Done App ctor" )

    continueInit();
}

App::~App()
{
    DEBUG_BLOCK

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
            AmarokConfig::setResumeTrack( engineTrack->playableUrl().prettyUrl() );
            AmarokConfig::setResumeTime( The::engineController()->trackPositionMs() );
        }
        else
            AmarokConfig::setResumeTrack( QString() ); //otherwise it'll play previous resume next time!
    }

    The::engineController()->endSession(); //records final statistics

#ifndef Q_WS_MAC
    // do even if trayicon is not shown, it is safe
    Amarok::config().writeEntry( "HiddenOnExit", mainWindow()->isHidden() );
    AmarokConfig::self()->writeConfig();
#else
    // for some reason on OS X the main window always reports being hidden
    // this means if you have the tray icon enabled, amarok will always open minimized
    Amarok::config().writeEntry( "HiddenOnExit", false );
    AmarokConfig::self()->writeConfig();
#endif

    ScriptManager::destroy();

    // this must be deleted before the connection to the Xserver is
    // severed, or we risk a crash when the QApplication is exited,
    // I asked Trolltech! *smug*
    Amarok::OSD::destroy();
    Amarok::KNotificationBackend::destroy();

    AmarokConfig::self()->writeConfig();

    //mainWindow()->deleteBrowsers();
    delete mainWindow();

    CollectionManager::destroy();
    Playlist::Actions::destroy();
    Playlist::ModelStack::destroy();
    PlaylistManager::destroy();
    CoverFetcher::destroy();

    //this should be moved to App::quit() I guess
    Amarok::Components::applicationController()->shutdown();


#ifdef Q_WS_WIN
    // work around for KUniqueApplication being not completely implemented on windows
    QDBusConnectionInterface* dbusService;
    if (QDBusConnection::sessionBus().isConnected() && (dbusService = QDBusConnection::sessionBus().interface()))
    {
        dbusService->unregisterService("org.mpris.amarok");
    }
#endif
}

void
App::handleCliArgs() //static
{
    DEBUG_BLOCK

    KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();

    if( args->isSet( "cwd" ) )
        KCmdLineArgs::setCwd( args->getOption( "cwd" ).toLocal8Bit() );

    bool haveArgs = true; // assume having args in first place
    if( args->count() > 0 )
    {
        KUrl::List list;
        for( int i = 0; i < args->count(); i++ )
        {
            KUrl url = args->url( i );
            //TODO:PORTME
            if( PodcastProvider::couldBeFeed( url.url() ) )
            {
                KUrl feedUrl = PodcastProvider::toFeedUrl( url.url() );
                The::playlistManager()->defaultPodcasts()->addPodcast( feedUrl );
            }
            else if( url.protocol() == "amarok" )
            {
                AmarokUrl aUrl( url.url() );
                aUrl.run();
            }
            else
            {
                list << url;
                DEBUG_LINE_INFO
            }
        }

        int options = Playlist::AppendAndPlay;
        if( args->isSet( "queue" ) )
           options = Playlist::Queue;
        else if( args->isSet( "append" ) )
           options = Playlist::Append;
        else if( args->isSet( "load" ) )
            options = Playlist::Replace;

        if( args->isSet( "play" ) )
            options |= Playlist::DirectPlay;

        The::playlistController()->insertOptioned( list, options );
    }
    else if ( args->isSet( "cdplay" ) )
        The::mainWindow()->playAudioCd();

    //we shouldn't let the user specify two of these since it is pointless!
    //so we prioritise, pause > stop > play > next > prev
    //thus pause is the least destructive, followed by stop as brakes are the most important bit of a car(!)
    //then the others seemed sensible. Feel free to modify this order, but please leave justification in the cvs log
    //I considered doing some sanity checks (eg only stop if paused or playing), but decided it wasn't worth it
    else if ( args->isSet( "pause" ) )
        The::engineController()->pause();
    else if ( args->isSet( "stop" ) )
        The::engineController()->stop();
    else if ( args->isSet( "play-pause" ) )
        The::engineController()->playPause();
    else if ( args->isSet( "play" ) ) //will restart if we are playing
        The::engineController()->play();
    else if ( args->isSet( "next" ) )
        The::playlistActions()->next();
    else if ( args->isSet( "previous" ) )
        The::playlistActions()->back();
    else // no args given
        haveArgs = false;

    static bool firstTime = true;
    const bool debugWasJustEnabled = !Amarok::config().readEntry( "Debug Enabled", false ) && args->isSet( "debug" );
    const bool debugIsDisabled = !args->isSet( "debug" );
    //allows debugging on OS X. Bundles have to be started with "open". Therefore it is not possible to pass an argument
    const bool forceDebug = Amarok::config().readEntry( "Force Debug", false );


    Amarok::config().writeEntry( "Debug Enabled", forceDebug ? true : args->isSet( "debug" ) );

    // Debug output will only work from this point on. If Amarok was run without debug output before,
    // then a part of the output (until this point) will be missing. Inform the user about this:
    if( debugWasJustEnabled || forceDebug )
    {
        debug() << "************************************************************************************************************";
        debug() << "** DEBUGGING OUTPUT IS NOW ENABLED. PLEASE NOTE THAT YOU WILL ONLY SEE THE FULL OUTPUT ON THE NEXT START. **";
        debug() << "************************************************************************************************************";
    }
    else if( firstTime && debugIsDisabled )
    {
        Amarok::config().writeEntry( "Debug Enabled", true );
        debug() << "**********************************************************************************************";
        debug() << "** AMAROK WAS STARTED IN NORMAL MODE. IF YOU WANT TO SEE DEBUGGING INFORMATION, PLEASE USE: **";
        debug() << "** amarok --debug                                                                           **";
        debug() << "**********************************************************************************************";
        Amarok::config().writeEntry( "Debug Enabled", false );
    }

    if( !firstTime && !haveArgs )
    {
        // mainWindow() can be 0 if another instance is loading, see https://bugs.kde.org/show_bug.cgi?id=202713
        if( pApp->mainWindow() )
            pApp->mainWindow()->activate();
    }

    firstTime = false;

#ifdef DEBUG
    if( args->isSet( "test" ) )
    {
        bool ok;
        int verboseInt = args->getOption( "verbose" ).toInt( &ok );
        verboseInt     = ok ? verboseInt : 2;

        QStringList testOpt( "amarok" );

        QString verbosity;
        switch( verboseInt )
        {
        case 0:
            verbosity = "-silent";
            break;
        case 1:
            verbosity = "-v1";
            break;
        default:
        case 2:
            verbosity = "-v2";
            break;
        case 3:
            verbosity = "-vs";
            break;
        }
        testOpt << verbosity;

        const QString format = args->getOption( "format" );
        if( format == "xml" || format == "lightxml" )
            testOpt << QString( '-' + format );

        const bool stdout = ( args->getOption( "output" ) == "log" ) ? false : true;
        runUnitTests( testOpt, stdout );
    }
#endif // DEBUG

    args->clear();    //free up memory
}


/////////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////////

void
App::initCliArgs( int argc, char *argv[] )
{
    KCmdLineArgs::reset();
    KCmdLineArgs::init( argc, argv, &::aboutData ); //calls KCmdLineArgs::addStdCmdLineOptions()
    initCliArgs();
}

void
App::initCliArgs() //static
{
    // Update main.cpp (below KUniqueApplication::start() wrt instanceOptions) aswell if needed!
    KCmdLineOptions options;

    options.add("+[URL(s)]", ki18n( "Files/URLs to open" ));
    options.add("cdplay", ki18n("Immediately start playing an audio cd"));
    options.add("r");
    options.add("previous", ki18n( "Skip backwards in playlist" ));
    options.add("p");
    options.add("play", ki18n( "Start playing current playlist" ));
    options.add("t");
    options.add("play-pause", ki18n( "Play if stopped, pause if playing" ));
    options.add("pause", ki18n( "Pause playback" ));
    options.add("s");
    options.add("stop", ki18n( "Stop playback" ));
    options.add("f");
    options.add("next", ki18n( "Skip forwards in playlist" ));
    options.add(":", ki18n("Additional options:"));
    options.add("a");
    options.add("append", ki18n( "Append files/URLs to playlist" ));
    options.add("queue", ki18n("Queue URLs after the currently playing track"));
    options.add("l");
    options.add("load", ki18n("Load URLs, replacing current playlist"));
    options.add("d");
    options.add("debug", ki18n("Print verbose debugging information"));
    options.add("m");
    options.add("multipleinstances", ki18n("Allow running multiple Amarok instances"));
    options.add("cwd <directory>", ki18n( "Base for relative filenames/URLs" ));
#ifdef DEBUG
    options.add(":", ki18n("Unit test options:"));
    options.add("test", ki18n( "Run integrated unit tests" ) );
    options.add("output <dest>", ki18n( "Destination of test output: 'stdout', 'log'" ), "log" );
    options.add("format <type>", ki18n( "Format of test output: 'xml', 'lightxml', 'plaintext'" ), "xml" );
    options.add("verbose <level>", ki18n( "Verbosity from 0-3 (highest)" ), "2" );
#endif // DEBUG

    KCmdLineArgs::addCmdLineOptions( options );   //add our own options
}


/////////////////////////////////////////////////////////////////////////////////////
// METHODS
/////////////////////////////////////////////////////////////////////////////////////

#include <id3v1tag.h>
#include <tbytevector.h>
#include <QTextCodec>
#include <KGlobal>

//this class is only used in this module, so I figured I may as well define it
//here and save creating another header/source file combination

// Local version of taglib's QStringToTString macro. It is here, because taglib's one is
// not Qt3Support clean (uses QString::utf8()). Once taglib will be clean of qt3support
// it is safe to use QStringToTString again
#define Qt4QStringToTString(s) TagLib::String(s.toUtf8().data(), TagLib::String::UTF8)

class ID3v1StringHandler : public TagLib::ID3v1::StringHandler
{
    QTextCodec *m_codec;

    virtual TagLib::String parse( const TagLib::ByteVector &data ) const
    {
        return Qt4QStringToTString( m_codec->toUnicode( data.data(), data.size() ) );
    }

    virtual TagLib::ByteVector render( const TagLib::String &ts ) const
    {
        const QByteArray qcs = m_codec->fromUnicode( TStringToQString(ts) );
        return TagLib::ByteVector( qcs, (uint) qcs.length() );
    }

public:
    ID3v1StringHandler( int codecIndex )
            : m_codec( QTextCodec::codecForName( QTextCodec::availableCodecs().at( codecIndex ) ) )
    {
        debug() << "codec: " << m_codec;
        debug() << "codec-name: " << m_codec->name();
    }

    ID3v1StringHandler( QTextCodec *codec )
            : m_codec( codec )
    {
        debug() << "codec: " << m_codec;
        debug() << "codec-name: " << m_codec->name();
    }

    virtual ~ID3v1StringHandler()
    {}
};

#undef Qt4QStringToTString

//SLOT
void App::applySettings( bool firstTime )
{
    ///Called when the configDialog is closed with OK or Apply

    DEBUG_BLOCK

    if ( AmarokConfig::showTrayIcon() && ! m_tray ) {
        m_tray = new Amarok::TrayIcon( mainWindow() );
    } else if ( !AmarokConfig::showTrayIcon() && m_tray ) {
        delete m_tray;
        m_tray = 0;
    }

    //on startup we need to show the window, but only if it wasn't hidden on exit
    //and always if the trayicon isn't showing
    QWidget* main_window = mainWindow();

    // show or hide CV
    if( mainWindow() )
        mainWindow()->hideContextView( AmarokConfig::hideContextView() );

    if( ( main_window && firstTime && !Amarok::config().readEntry( "HiddenOnExit", false ) ) || ( main_window && !AmarokConfig::showTrayIcon() ) )
    {
        PERF_LOG( "showing main window again" )
        main_window->show();
        PERF_LOG( "after showing mainWindow" )
    }

    if( firstTime )
    {   // delete unneeded cover images from cache
        PERF_LOG( "Begin cover handling" )
        const QString size = QString::number( 100 ) + '@';
        const QDir cacheDir = Amarok::saveLocation( "albumcovers/cache/" );
        const QStringList obsoleteCovers = cacheDir.entryList( QStringList("*") );
        foreach( const QString &it, obsoleteCovers )
            //34@ is for playlist album cover images
            if ( !it.startsWith( size  ) && !it.startsWith( "50@" ) && !it.startsWith( "32@" ) && !it.startsWith( "34@" ) )
                QFile( cacheDir.filePath( it ) ).remove();

        PERF_LOG( "done cover handling" )
    }
}

#ifdef DEBUG
//SLOT
void
App::runUnitTests( const QStringList options, bool stdout )
{
    DEBUG_BLOCK

    QString logPath;
    if( !stdout )
    {
        const QString location = Amarok::saveLocation( "testresults/" );
        const QString stamp    = QDateTime::currentDateTime().toString( "yyyy-MM-dd.HH-mm-ss" );
        logPath                = QDir::toNativeSeparators( location + stamp + "/" );

        // create log folder for this run:
        QDir logDir( logPath );
        logDir.mkpath( logPath );

        QFile::remove( QDir::toNativeSeparators( Amarok::saveLocation( "testresults/" ) + "LATEST" ) );
        QFile::link( logPath, QDir::toNativeSeparators( Amarok::saveLocation( "testresults/" ) + "LATEST" ) );

        QFile logArgs( logPath + "test_options" );
        if( logArgs.open( QIODevice::WriteOnly ) )
        {
            logArgs.write( options.join( " " ).toLatin1() );
            logArgs.close();
        }
    }

    PERF_LOG( "Running Unit Tests" )

    // modifies the playlist asynchronously, so run this last to avoid messing other test results
    TestDirectoryLoader        *test015 = new TestDirectoryLoader( options, logPath );

    PERF_LOG( "Done Running Unit Tests" )
    Q_UNUSED( test015 )
}
#endif // DEBUG

//SLOT
void
App::continueInit()
{
    DEBUG_BLOCK

    PERF_LOG( "Begin App::continueInit" )
    const KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();
    const bool restoreSession = args->count() == 0 || args->isSet( "append" ) || args->isSet( "queue" )
                                || Amarok::config().readEntry( "AppendAsDefault", false );

    QTextCodec* utf8codec = QTextCodec::codecForName( "UTF-8" );
    QTextCodec::setCodecForCStrings( utf8codec ); //We need this to make CollectionViewItem showing the right characters.

    new Amarok::DefaultApplicationController();
    Amarok::Components::applicationController()->start();

    KSplashScreen* splash = 0;
    if( AmarokConfig::showSplashscreen() && !isSessionRestored() )
    {
        QPixmap splashimg( KGlobal::dirs()->findResource( "data", "amarok/images/splash_screen.jpg" ) );
        splash = new KSplashScreen( splashimg, Qt::WindowStaysOnTopHint );
        splash->show();
    }

    PERF_LOG( "Creating MainWindow" )
    m_mainWindow = new MainWindow();
    PERF_LOG( "Done creating MainWindow" )

    if ( AmarokConfig::showTrayIcon() ) {
        m_tray = new Amarok::TrayIcon( mainWindow() );
    }

    PERF_LOG( "Creating DBus handlers" )
    new Amarok::RootDBusHandler();
    new Amarok::PlayerDBusHandler();
    new Amarok::TracklistDBusHandler();
    new CollectionDBusHandler( this );
    QDBusConnection::sessionBus().registerService("org.mpris.amarok");
    PERF_LOG( "Done creating DBus handlers" )

    if( splash ) // close splash correctly
    {
        splash->close();
        delete splash;
    }

    //DON'T DELETE THIS NEXT LINE or the app crashes when you click the X (unless we reimplement closeEvent)
    //Reason: in ~App we have to call the deleteBrowsers method or else we run afoul of refcount foobar in KHTMLPart
    //But if you click the X (not Action->Quit) it automatically kills MainWindow because KMainWindow sets this
    //for us as default (bad KMainWindow)
    mainWindow()->setAttribute( Qt::WA_DeleteOnClose, false );
    //init playlist window as soon as the database is guaranteed to be usable

    // Create engine, show TrayIcon etc.
    applySettings( true );

    // Must be created _after_ MainWindow.
    PERF_LOG( "Starting ScriptManager" )
    ScriptManager::instance();
    PERF_LOG( "ScriptManager started" )

    The::engineController()->setVolume( AmarokConfig::masterVolume() );
    The::engineController()->setMuted( AmarokConfig::muteState() );

    Amarok::KNotificationBackend::instance()->setEnabled( AmarokConfig::kNotifyEnabled() );
    Amarok::OSD::instance()->applySettings(); // Create after setting volume (don't show OSD for that)


    if( AmarokConfig::resumePlayback() && restoreSession && !args->isSet( "stop" ) ) {
        //restore session as long as the user didn't specify media to play etc.
        //do this after applySettings() so OSD displays correctly
        The::engineController()->restoreSession();
    }

    if( AmarokConfig::monitorChanges() )
        CollectionManager::instance()->checkCollectionChanges();

    // Restore keyboard shortcuts etc from config
    Amarok::actionCollection()->readSettings();

    PERF_LOG( "App init done" )
    KConfigGroup config = KGlobal::config()->group( "General" );

    // NOTE: First Run Tutorial disabled for 2.1-beta1 release (too buggy / unfinished)
#if 0
    const bool firstruntut = config.readEntry( "FirstRunTutorial", true );
    debug() << "Checking whether to run first run tutorial..." << firstruntut;
    if( firstruntut )
    {
        config.writeEntry( "FirstRunTutorial", false );
        debug() << "Starting first run tutorial";
        FirstRunTutorial *frt = new FirstRunTutorial( mainWindow() );
        QTimer::singleShot( 1000, frt, SLOT( initOverlay() ) );
    }
#endif

#ifdef NO_MYSQL_EMBEDDED
    bool useServer = true;
    if( !AmarokConfig::useServer() )
    {
        useServer = false;
        AmarokConfig::setUseServer( true );
    }
    if( !MySqlServerTester::testSettings(
             AmarokConfig::host(),
             AmarokConfig::user(),
             AmarokConfig::password(),
             AmarokConfig::port()
         )
    )
    {
        KMessageBox::messageBox( 0, KMessageBox::Information,
                ( !useServer ? i18n( "The embedded database was not found; you must set up a database server connection.\nYou must restart Amarok after doing this." ) :
                              i18n( "The connection details for the database server were invalid.\nYou must enter correct settings and restart Amarok after doing this." ) ),
                i18n( "Database Error" ) );
        slotConfigAmarok( "DatabaseConfig" );
    }
    else
#endif
    {
        if( config.readEntry( "First Run", true ) )
        {
            const KUrl musicUrl = QDesktopServices::storageLocation( QDesktopServices::MusicLocation );
            const QString musicDir = musicUrl.toLocalFile( KUrl::RemoveTrailingSlash );
            const QDir dir( musicDir );

            int result = KMessageBox::No;
            if( dir.exists() && dir.isReadable() )
            {
                result = KMessageBox::questionYesNoCancel(
                    mainWindow(),
                    i18n( "A music path, %1, is set in System Settings.\nWould you like to use that as a collection folder?", musicDir )
                    );
            }

            KConfigGroup folderConf = Amarok::config( "Collection Folders" );
            bool useMusicLocation( false );
            switch( result )
            {
            case KMessageBox::Yes:
                if( CollectionManager::instance()->primaryCollection() )
                {
                    CollectionManager::instance()->primaryCollection()->setProperty( "collectionFolders", QStringList() << musicDir );
                    CollectionManager::instance()->startFullScan();
                    useMusicLocation = true;
                }
                break;

            case KMessageBox::No:
                slotConfigAmarok( "CollectionConfig" );
                break;

            default:
                break;
            }
            folderConf.writeEntry( "Use MusicLocation", useMusicLocation );
            config.writeEntry( "First Run", false );
        }
    }

    // Using QTimer, so that we won't block the GUI
    QTimer::singleShot( 0, this, SLOT( checkCollectionScannerVersion() ) );
}

void App::checkCollectionScannerVersion()  // SLOT
{
    DEBUG_BLOCK

    QProcess scanner;

    scanner.start( collectionScannerLocation(), QStringList( "--version" ) );
    scanner.waitForFinished();

    const QString version = scanner.readAllStandardOutput().trimmed();

    if( version != AMAROK_VERSION  )
    {
        KMessageBox::error( 0, i18n( "<p>The version of the 'amarokcollectionscanner' tool\n"
                                     "does not match your Amarok version.</p>"
                                     "<p>Please note that Collection Scanning may not work correctly.</p>" ) );
    }
}

QString App::collectionScannerLocation()  // static
{
    QString scannerPath = KStandardDirs::locate( "exe", "amarokcollectionscanner" );

    // If the binary is not in $PATH, then search in the application folder too
    if( scannerPath.isEmpty() )
        scannerPath = applicationDirPath() + QDir::separator() + "amarokcollectionscanner";

    return scannerPath;
}

void App::slotConfigAmarok( const QString& page )
{
    Amarok2ConfigDialog* dialog = static_cast<Amarok2ConfigDialog*>( KConfigDialog::exists( "settings" ) );

    if( !dialog )
    {
        //KConfigDialog didn't find an instance of this dialog, so lets create it :
        dialog = new Amarok2ConfigDialog( mainWindow(), "settings", AmarokConfig::self() );

        connect( dialog, SIGNAL( settingsChanged( const QString& ) ), SLOT( applySettings() ) );
    }

    dialog->show( page );
}

void App::slotConfigShortcuts()
{
    KShortcutsDialog::configure( Amarok::actionCollection(), KShortcutsEditor::LetterShortcutsAllowed, mainWindow() );
    AmarokConfig::self()->writeConfig();
}

KIO::Job *App::trashFiles( const KUrl::List &files )
{
    KIO::Job *job = KIO::trash( files );
    The::statusBar()->newProgressOperation( job, i18n("Moving files to trash") );
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotTrashResult( KJob* ) ) );
    return job;
}

void App::slotTrashResult( KJob *job )
{
    if( job->error() )
        job->uiDelegate()->showErrorMessage();
}

void App::quit()
{
    The::playlistManager()->completePodcastDownloads();

    emit prepareToQuit();
    /*
    if( MediaBrowser::instance() && MediaBrowser::instance()->blockQuit() )
    {
        // don't quit yet, as some media devices still have to finish transferring data
        QTimer::singleShot( 100, this, SLOT( quit() ) );
        return;
    }
    */
    KApplication::quit();
}

bool App::event( QEvent *event )
{
    switch( event->type() )
    {
        //allows Amarok to open files from the finder on OS X
        case QEvent::FileOpen:
        {
            QString file = static_cast<QFileOpenEvent*>( event )->file();
            //we are only going to receive local files here
            KUrl url( file );
            if( Playlists::isPlaylist( url ) )
            {
                Playlists::PlaylistPtr playlist =
                        Playlists::PlaylistPtr::dynamicCast( Playlists::loadPlaylistFile( url ) );
                The::playlistController()->insertOptioned( playlist, Playlist::AppendAndPlay );
            }
            else
            {
                Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
                The::playlistController()->insertOptioned( track, Playlist::AppendAndPlay );
            }
            return true;
        }
        default:
            return KUniqueApplication::event( event );
    }
}

bool App::notify( QObject *receiver, QEvent *event )
{
    // Here we try to catch exceptions from LiblastFm, which Qt can't handle, except in this method.
    // @see: https://bugs.kde.org/show_bug.cgi?id=212115

    try
    {
        return QApplication::notify( receiver, event );
    }
    catch(...)
    {
        error() << "Caught an exception, probably from LibLastfm. Ignoring.";
        return false;
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

    handleCliArgs();
    return 0;
}


#include "App.moc"

