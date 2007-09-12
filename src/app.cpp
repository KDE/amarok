/***************************************************************************
                      app.cpp  -  description
                         -------------------
begin                : Mit Okt 23 14:35:18 CEST 2002
copyright            : (C) 2002 by Mark Kretschmann
email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "app.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "amarokdbushandler.h"
#include "atomicstring.h"
#include "collectiondb.h"
#include "CollectionManager.h"
#include "ConfigDialog.h"
#include "context/ContextView.h"
#include "debug.h"
#include "enginebase.h"
#include "enginecontroller.h"
#include "equalizersetup.h"
#include "MainWindow.h"
#include "mediabrowser.h"
#include "meta.h"
#include "metabundle.h"
#include "mountpointmanager.h"
#include "osd.h"
#include "playlist/PlaylistModel.h"

#include "pluginmanager.h"
#include "portabledevices/SolidHandler.h"
#include "refreshimages.h"
#include "scriptmanager.h"
#include "scrobbler.h"
#include "statusbar.h"
#include "systray.h"
#include "threadmanager.h"
#include "tracktooltip.h"        //engineNewMetaData()
#include "TheInstances.h"
#include "metadata/tplugins.h"

#include <iostream>

#include <KAboutData>
#include <KAction>
#include <KCmdLineArgs>        //initCliArgs()
#include <KConfigDialogManager>
#include <KCursor>             //Amarok::OverrideCursor
#include <KEditToolBar>        //slotConfigToolbars()
#include <KGlobalAccel>        //initGlobalShortcuts()
#include <KGlobalSettings>     //applyColorScheme()
#include <KIO/CopyJob>
#include <KIconLoader>         //amarok Icon
#include <KJob>
#include <KJobUiDelegate>
#include <KLocale>
#include <KMessageBox>         //applySettings(), genericEventHandler()
#include <KRun>                //Amarok::invokeBrowser()
#include <kshell.h>
#include <KShortcutsDialog>     //slotConfigShortcuts()
#include <KSplashScreen>
#include <KStandardDirs>

#include <QCloseEvent>
#include <QDBusInterface>
#include <QDBusReply>
#include <QEvent>              //genericEventHandler()
#include <QEventLoop>          //applySettings()
#include <QFile>
#include <QPixmapCache>
#include <Q3PopupMenu>          //genericEventHandler
#include <QTimer>              //showHyperThreadingWarning()
#include <QToolTip>            //default tooltip for trayicon

QMutex Debug::mutex;
QMutex Amarok::globalDirsMutex;

int App::mainThreadId = 0;

#ifdef Q_WS_MAC
#include <CoreFoundation/CoreFoundation.h>
extern void setupEventHandler_mac(long);
#endif

AMAROK_EXPORT KAboutData aboutData( "amarok", 0,
    ki18n( "Amarok" ), APP_VERSION,
    ki18n( "The audio player for KDE" ), KAboutData::License_GPL,
    ki18n( "(C) 2002-2003, Mark Kretschmann\n(C) 2003-2007, The Amarok Development Squad" ),
    ki18n( "IRC:\nirc.freenode.net - #amarok, #amarok.de, #amarok.es\n\nFeedback:\namarok@kde.org\n\n(Build Date: " __DATE__ ")" ),
             ( "http://amarok.kde.org" ) );


App::App()
        : KUniqueApplication()
        , m_splash( 0 )
{
    DEBUG_BLOCK

    if( AmarokConfig::showSplashscreen() )
    {
        QPixmap splashpix( KStandardDirs().findResource("data", "amarok/images/splash_screen.jpg") );
        m_splash = new KSplashScreen( splashpix, Qt::WindowStaysOnTopHint );
        m_splash->show();
    }

    registerTaglibPlugins();

    qRegisterMetaType<MetaBundle>();

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
    QPixmapCache::setCacheLimit ( 10 * 1024 ); 

#ifdef Q_WS_MAC
    // this is inspired by OpenSceneGraph: osgDB/FilePath.cpp

    // Start with the the Bundle PlugIns directory.

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
                    QByteArray path = getenv( "PATH" );
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
#endif

    //needs to be created before the wizard
     new Amarok::DbusPlayerHandler(); // Must be created first
     new Amarok::DbusPlaylistHandler();
     new Amarok::DbusPlaylistBrowserHandler();
     new Amarok::DbusContextHandler();
     new Amarok::DbusCollectionHandler();
     new Amarok::DbusMediaBrowserHandler();
     new Amarok::DbusScriptHandler();

    // tell AtomicString that this is the GUI thread
    if ( !AtomicString::isMainThread() )
        qWarning("AtomicString was initialized from a thread other than the GUI "
                 "thread. This could lead to memory leaks.");

#ifdef Q_WS_MAC
    setupEventHandler_mac((long)this);
#endif
    QDBusConnection::sessionBus().registerService("org.kde.amarok");
    QTimer::singleShot( 0, this, SLOT( continueInit() ) );
}

App::~App()
{
    DEBUG_BLOCK

    delete m_splash;
    m_splash = 0;

    // Hiding the OSD before exit prevents crash
    Amarok::OSD::instance()->hide();

    EngineBase* const engine = EngineController::engine();

    if ( AmarokConfig::resumePlayback() ) {
        if ( engine->state() != Engine::Empty ) {
            AmarokConfig::setResumeTrack( EngineController::instance()->playingURL().prettyUrl() );
            AmarokConfig::setResumeTime( engine->position() );
        }
        else AmarokConfig::setResumeTrack( QString() ); //otherwise it'll play previous resume next time!
    }

    EngineController::instance()->endSession(); //records final statistics
    EngineController::instance()->detach( this );

    // do even if trayicon is not shown, it is safe
    Amarok::config().writeEntry( "HiddenOnExit", mainWindow()->isHidden() );

    CollectionDB::instance()->stopScan();

    ThreadManager::deleteInstance(); //waits for jobs to finish

    delete mainWindow();

    // this must be deleted before the connection to the Xserver is
    // severed, or we risk a crash when the QApplication is exited,
    // I asked Trolltech! *smug*
    delete Amarok::OSD::instance();

    AmarokConfig::setVersion( APP_VERSION );
    AmarokConfig::self()->writeConfig();

    //need to unload the engine before the kapplication is destroyed
    PluginManager::unload( engine );
}


#include <QStringList>

namespace
{
    // grabbed from KsCD source, kompatctdisk.cpp
    QString urlToDevice(const QString& device)
    {
        KUrl deviceUrl(device);
        if (deviceUrl.protocol() == "media" || deviceUrl.protocol() == "system")
        {
            debug() << "WARNING: urlToDevice needs to be reimplemented with KDE4 technology, it's just a stub at the moment";
           QDBusInterface mediamanager( "org.kde.kded", "/modules/mediamanager", "org.kde.MediaManager" );
           QDBusReply<QStringList> reply = mediamanager.call( "properties",deviceUrl.fileName() );
           if (!reply.isValid()) {
        debug() << "Invalid reply from mediamanager";
               return QString();
           }
       QStringList properties = reply;
       if( properties.count()< 6 )
        return QString();
      debug() << "Reply from mediamanager " << properties[5];
          return properties[5];
        }

        return device;
    }

}


void App::handleCliArgs() //static
{
    KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();

    if ( args->isSet( "cwd" ) )
    {
        KCmdLineArgs::setCwd( args->getOption( "cwd" ).toLocal8Bit() );
    }

    bool haveArgs = false;
    if ( args->count() > 0 )
    {
        haveArgs = true;

        KUrl::List list;
        for( int i = 0; i < args->count(); i++ )
        {
            KUrl url = args->url( i );
            //TODO:PORTME
//             if( url.protocol() == "itpc" || url.protocol() == "pcast" )
//                 PlaylistBrowserNS::instance()->addPodcast( url );
//             else
                list << url;
        }

        int options = Playlist::AppendAndPlay;
        if( args->isSet( "queue" ) )
           options = Playlist::Queue;
        else if( args->isSet( "append" ) || args->isSet( "enqueue" ) )
           options = Playlist::Append;
        else if( args->isSet( "load" ) )
            options = Playlist::Replace;

        if( args->isSet( "play" ) )
            options |= Playlist::DirectPlay;

        The::playlistModel()->insertMedia( list, options );
    }

    //we shouldn't let the user specify two of these since it is pointless!
    //so we prioritise, pause > stop > play > next > prev
    //thus pause is the least destructive, followed by stop as brakes are the most important bit of a car(!)
    //then the others seemed sensible. Feel free to modify this order, but please leave justification in the cvs log
    //I considered doing some sanity checks (eg only stop if paused or playing), but decided it wasn't worth it
    else if ( args->isSet( "pause" ) )
    {
        haveArgs = true;
        EngineController::instance()->pause();
    }
    else if ( args->isSet( "stop" ) )
    {
        haveArgs = true;
        EngineController::instance()->stop();
    }
    else if ( args->isSet( "play-pause" ) )
    {
        haveArgs = true;
        EngineController::instance()->playPause();
    }
    else if ( args->isSet( "play" ) ) //will restart if we are playing
    {
        haveArgs = true;
        EngineController::instance()->play();
    }
    else if ( args->isSet( "next" ) )
    {
        haveArgs = true;
        EngineController::instance()->next();
    }
    else if ( args->isSet( "previous" ) )
    {
        haveArgs = true;
        EngineController::instance()->previous();
    }
    else if (args->isSet("cdplay"))
    {
        haveArgs = true;
        QString device = args->getOption("cdplay");
        KUrl::List urls;
        if (EngineController::engine()->getAudioCDContents(device, urls)) {
            Meta::TrackList tracks = CollectionManager::instance()->tracksForUrls( urls );
            The::playlistModel()->insertOptioned(
                tracks, Playlist::Replace|Playlist::DirectPlay);
        } else { // Default behaviour
            debug() <<
                "Sorry, the engine doesn't support direct play from AudioCD..."
                   ;
        }
    }

    if ( args->isSet( "toggle-playlist-window" ) )
    {
        haveArgs = true;
        pApp->mainWindow()->showHide();
    }

    Amarok::config().writeEntry( "Debug Enabled", args->isSet( "debug" ) ); 
      
    static bool firstTime = true;
    if( !firstTime && !haveArgs )
        pApp->mainWindow()->activate();
    firstTime = false;

    args->clear();    //free up memory
}


/////////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////////

void App::initCliArgs( int argc, char *argv[] ) //static
{
    KCmdLineArgs::reset();
    KCmdLineArgs::init( argc, argv, &::aboutData ); //calls KCmdLineArgs::addStdCmdLineOptions()

    KCmdLineOptions options;
    options.add("+[URL(s)]", ki18n( "Files/URLs to open" ));
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
    options.add("e");
    options.add("enqueue", ki18n("See append, available for backwards compatability"));
    options.add("queue", ki18n("Queue URLs after the currently playing track"));
    options.add("l");
    options.add("load", ki18n("Load URLs, replacing current playlist"));
    options.add("d");
    options.add("debug", ki18n("Print verbose debugging information"));
    options.add("m");
    options.add("toggle-playlist-window", ki18n("Toggle the Playlist-window"));
    options.add("wizard", ki18n( "Run first-run wizard" ));
    options.add("engine <name>", ki18n( "Use the <name> engine" ));
    options.add("cwd <directory>", ki18n( "Base for relative filenames/URLs" ));
    options.add("cdplay <device>", ki18n("Play an AudioCD from <device> or system:/media/<device>"));
    KCmdLineArgs::addCmdLineOptions( options );   //add our own options
}


void App::initGlobalShortcuts()
{
    EngineController* const ec = EngineController::instance();
    KAction* action;

//    m_pGlobalAccel->insert( "play", i18n( "Play" ), 0, KKey("WIN+x"), 0, ec, SLOT( play() ), true, true );
    action = new KAction( i18n( "Play" ), mainWindow() );
//    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_X ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( play() ) );

//    m_pGlobalAccel->insert( "pause", i18n( "Pause" ), 0, 0, 0, ec, SLOT( pause() ), true, true );
    action = new KAction( i18n( "Pause" ), mainWindow() );
    connect( action, SIGNAL( triggered() ), ec, SLOT( pause() ) );

//    m_pGlobalAccel->insert( "play_pause", i18n( "Play/Pause" ), 0, KKey("WIN+c"), 0, ec, SLOT( playPause() ), true, true );
    action = new KAction( i18n( "Play/Pause" ), mainWindow() );
//    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_C ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( playPause() ) );

//    m_pGlobalAccel->insert( "stop", i18n( "Stop" ), 0, KKey("WIN+v"), 0, ec, SLOT( stop() ), true, true );
    action = new KAction( i18n( "Stop" ), mainWindow() );
//    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_V ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( stop() ) );

//    m_pGlobalAccel->insert( "stop_after_global", i18n( "Stop Playing After Current Track" ), 0, KKey("WIN+CTRL+v"), 0, Playlist::instance()->qscrollview(), SLOT( toggleStopAfterCurrentTrack() ), true, true );
    action = new KAction( i18n( "Stop Playing After Current Track" ), mainWindow() );
//    action->setGlobalShortcut( KShortcut( Qt::META + Qt::CTRL + Qt::Key_V ) );
    //Port 2.0
//     connect( action, SIGNAL( triggered() ), Playlist::instance()->qscrollview(), SLOT( toggleStopAfterCurrentTrack() ) );

//    m_pGlobalAccel->insert( "next", i18n( "Next Track" ), 0, KKey("WIN+b"), 0, ec, SLOT( next() ), true, true );
    action = new KAction( i18n( "Next Track" ), mainWindow() );
//    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_B ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( next() ) );

//    m_pGlobalAccel->insert( "prev", i18n( "Previous Track" ), 0, KKey("WIN+z"), 0, ec, SLOT( previous() ), true, true );
    action = new KAction( i18n( "Previous Track" ), mainWindow() );
//    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_Z ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( previous() ) );

//    m_pGlobalAccel->insert( "volup", i18n( "Increase Volume" ), 0, KKey("WIN+KP_Add"), 0, ec, SLOT( increaseVolume() ), true, true );
    action = new KAction( i18n( "Increase Volume" ), mainWindow() );
//    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_Plus ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( increaseVolume() ) );

//    m_pGlobalAccel->insert( "voldn", i18n( "Decrease Volume" ), 0, KKey("WIN+KP_Subtract"), 0, ec, SLOT( decreaseVolume() ), true, true );
    action = new KAction( i18n( "Decrease Volume" ), mainWindow() );
//    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_Minus ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( decreaseVolume() ) );


//    m_pGlobalAccel->insert( "seekforward", i18n( "Seek Forward" ), 0, KKey("WIN+Shift+KP_Add"), 0, ec, SLOT( seekForward() ), true, true );
    action = new KAction( i18n( "Seek Forward" ), mainWindow() );
//    action->setGlobalShortcut( KShortcut( Qt::META + Qt::SHIFT + Qt::Key_Plus ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( seekForward() ) );


//    m_pGlobalAccel->insert( "seekbackward", i18n( "Seek Backward" ), 0, KKey("WIN+Shift+KP_Subtract"), 0, ec, SLOT( seekBackward() ), true, true );
    action = new KAction( i18n( "Seek Backward" ), mainWindow() );
//    action->setGlobalShortcut( KShortcut( Qt::META + Qt::SHIFT + Qt::Key_Minus ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( seekBackward() ) );

//    m_pGlobalAccel->insert( "playlist_add", i18n( "Add Media..." ), 0, KKey("WIN+a"), 0, mainWindow(), SLOT( slotAddLocation() ), true, true );
    action = new KAction( i18n( "Add Media..." ), mainWindow() );
//    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_A ) );
    connect( action, SIGNAL( triggered() ), mainWindow(), SLOT( slotAddLocation() ) );

//    m_pGlobalAccel->insert( "show", i18n( "Toggle Playlist Window" ), 0, KKey("WIN+p"), 0, mainWindow(), SLOT( showHide() ), true, true );
    action = new KAction( i18n( "Toggle Playlist Window" ), mainWindow() );
//    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_P ) );
    connect( action, SIGNAL( triggered() ), mainWindow(), SLOT( showHide() ) );


//    m_pGlobalAccel->insert( "osd", i18n( "Show OSD" ), 0, KKey("WIN+o"), 0, Amarok::OSD::instance(), SLOT( forceToggleOSD() ), true, true );
    action = new KAction( i18n( "Show OSD" ), mainWindow() );
//    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_O ) );
    connect( action, SIGNAL( triggered() ), Amarok::OSD::instance(), SLOT( forceToggleOSD() ) );
#if 0
    m_pGlobalAccel->insert( "mute", i18n( "Mute Volume" ), 0, KKey("WIN+m"), 0,
                            ec, SLOT( mute() ), true, true );
    m_pGlobalAccel->insert( "rating1", i18n( "Rate Current Track: 1" ), 0, KKey("WIN+1"), 0,
                            this, SLOT( setRating1() ), true, true );
    m_pGlobalAccel->insert( "rating2", i18n( "Rate Current Track: 2" ), 0, KKey("WIN+2"), 0,
                            this, SLOT( setRating2() ), true, true );
    m_pGlobalAccel->insert( "rating3", i18n( "Rate Current Track: 3" ), 0, KKey("WIN+3"), 0,
                            this, SLOT( setRating3() ), true, true );
    m_pGlobalAccel->insert( "rating4", i18n( "Rate Current Track: 4" ), 0, KKey("WIN+4"), 0,
                            this, SLOT( setRating4() ), true, true );
    m_pGlobalAccel->insert( "rating5", i18n( "Rate Current Track: 5" ), 0, KKey("WIN+5"), 0,
                            this, SLOT( setRating5() ), true, true );
#endif

//    KGlobalAccel::self()->setConfigGroup( "Shortcuts" );
//    KGlobalAccel::self()->readSettings( KGlobal::config().data() );


// FIXME Is this still needed with KDE4?
#if 0
    //TODO fix kde accel system so that kactions find appropriate global shortcuts
    //     and there is only one configure shortcuts dialog

    KActionCollection* const ac = Amarok::actionCollection();
    KAccelShortcutList list( m_pGlobalAccel );

    for( uint i = 0; i < list.count(); ++i )
    {
        KAction *action = ac->action( list.name( i ).toLatin1() );

        if( action )
        {
            //this is a hack really, also it means there may be two calls to the slot for the shortcut
            action->setShortcutConfigurable( false );
            action->setShortcut( list.shortcut( i ) );
        }
    }
#endif
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

class ID3v1StringHandler : public TagLib::ID3v1::StringHandler
{
    QTextCodec *m_codec;

    virtual TagLib::String parse( const TagLib::ByteVector &data ) const
    {
        return QStringToTString( m_codec->toUnicode( data.data(), data.size() ) );
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

//SLOT
void App::applySettings( bool firstTime )
{
    ///Called when the configDialog is closed with OK or Apply

    DEBUG_BLOCK

#ifndef Q_WS_MAC
    //probably needs to be done in TrayIcon when it receives a QEvent::ToolTip (see QSystemtrayIcon documentation)
    //TrackToolTip::instance()->removeFromWidget( m_tray );
#endif
    Scrobbler::instance()->applySettings();
    Amarok::OSD::instance()->applySettings();
    CollectionDB::instance()->applySettings();
#ifndef Q_WS_MAC
    m_tray->setVisible( AmarokConfig::showTrayIcon() );
    //TrackToolTip::instance()->addToWidget( m_tray );
#endif


    //on startup we need to show the window, but only if it wasn't hidden on exit
    //and always if the trayicon isn't showing
    QWidget* main_window = mainWindow();
#ifndef Q_WS_MAC
    if( ( main_window && firstTime && !Amarok::config().readEntry( "HiddenOnExit", false ) ) || ( main_window && !AmarokConfig::showTrayIcon() ) )
#endif
    {
        main_window->show();

        //takes longer but feels shorter. Crazy eh? :)
        kapp->processEvents( QEventLoop::ExcludeUserInputEvents );
    }


    { //<Engine>
        EngineBase *engine = EngineController::engine();

        if( firstTime || AmarokConfig::soundSystem() !=
                         PluginManager::getService( engine )->property( "X-KDE-Amarok-name" ).toString() )
        {
            //will unload engine for us first if necessary
            engine = EngineController::instance()->loadEngine();
        }

        engine->setXfadeLength( AmarokConfig::crossfade() ? AmarokConfig::crossfadeLength() : 0 );
        engine->setVolume( AmarokConfig::masterVolume() );

        engine->setEqualizerEnabled( AmarokConfig::equalizerEnabled() );
        if ( AmarokConfig::equalizerEnabled() )
            engine->setEqualizerParameters( AmarokConfig::equalizerPreamp(), AmarokConfig::equalizerGains() );

        Amarok::actionCollection()->action("play_audiocd")->setEnabled( EngineController::hasEngineProperty( "HasKIO" ) || EngineController::hasEngineProperty("HasCDDA"));
    } //</Engine>

    { //<Collection>
//PORT 2.0        CollectionView::instance()->renderView(true);
    } //</Collection>
    { //<Context>
 //PORT 2.0       ContextBrowser::instance()->renderView();
    } //</Context>

    {   // delete unneeded cover images from cache
        const QString size = QString::number( AmarokConfig::coverPreviewSize() ) + '@';
        const QDir cacheDir = Amarok::saveLocation( "albumcovers/cache/" );
        const QStringList obsoleteCovers = cacheDir.entryList( QStringList("*") );
        foreach( QString it, obsoleteCovers )
            if ( !it.startsWith( size  ) && !it.startsWith( "50@" ) )
                QFile( cacheDir.filePath( it ) ).remove();
    }

    //if ( !firstTime )
        // Bizarrely and ironically calling this causes crashes for
        // some people! FIXME
        //AmarokConfig::self()->writeConfig();

}

//SLOT
void
App::continueInit()
{
    DEBUG_BLOCK
    const KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();
    bool restoreSession = args->count() == 0 || args->isSet( "append" ) || args->isSet( "enqueue" )
                                || Amarok::config().readEntry( "AppendAsDefault", false );

    // Make this instance so it can start receiving signals
    MoodServer::instance();

    // Remember old folder setup, so we can detect changes after the wizard was used
    //const QStringList oldCollectionFolders = MountPointManager::instance()->collectionFolders();


    // Is this needed in Amarok 2?
    if( Amarok::config().readEntry( "First Run", true ) || args->isSet( "wizard" ) )
    {
        std::cout << "STARTUP\n" << std::flush; //hide the splashscreen
        Amarok::config().writeEntry( "First Run", false );
        Amarok::config().sync();
    }

    CollectionDB::instance()->checkDatabase();

    The::SolidHandler()->Initialize();
    m_mainWindow = new MainWindow();
#ifndef Q_WS_MAC
    m_tray           = new Amarok::TrayIcon( mainWindow() );
#endif
    mainWindow()->init(); //creates the playlist, browsers, etc.
    //init playlist window as soon as the database is guaranteed to be usable
    //connect( CollectionDB::instance(), SIGNAL( databaseUpdateDone() ), mainWindow(), SLOT( init() ) );
    //initGlobalShortcuts();
    //load previous playlist in separate thread
    if ( restoreSession && AmarokConfig::savePlaylist() )
    {
        The::playlistModel()->restoreSession();
    }
    if( args->isSet( "engine" ) ) {
        // we correct some common errors (case issues, missing -engine off the end)
        QString engine = args->getOption( "engine" ).toLower();
        if( engine.startsWith( "gstreamer" ) ) engine = "gst-engine";
        if( !engine.endsWith( "engine" ) ) engine += "-engine";

        AmarokConfig::setSoundSystem( engine );
    }
    Debug::stamp();
    //create engine, show TrayIcon etc.
    applySettings( true );
    Debug::stamp();
    // Start ScriptManager. Must be created _after_ MainWindow.
    ScriptManager::instance();
    Debug::stamp();
    //notify loader application that we have started
    std::cout << "STARTUP\n" << std::flush;

    //do after applySettings(), or the OSD will flicker and other wierdness!
    //do before restoreSession()!
    EngineController::instance()->attach( this );

    //set a default interface
    engineStateChanged( Engine::Empty );

    if ( AmarokConfig::resumePlayback() && restoreSession && !args->isSet( "stop" ) ) {
        //restore session as long as the user didn't specify media to play etc.
        //do this after applySettings() so OSD displays correctly
        EngineController::instance()->restoreSession();
    }

    // Refetch covers every 80 days to comply with Amazon license
    new RefreshImages();

    CollectionDB *collDB = CollectionDB::instance();

    // If database version is updated, the collection needs to be rescanned.
    // Works also if the collection is empty for some other reason
    // (e.g. deleted collection.db)
    if ( CollectionDB::instance()->isEmpty() )
    {
         //connect( collDB, SIGNAL( databaseUpdateDone() ), collDB, SLOT( startScan() ) );
        collDB->startScan();
    }
    else if ( AmarokConfig::monitorChanges() )
        //connect( collDB, SIGNAL( databaseUpdateDone() ), collDB, SLOT( scanModifiedDirs() ) );
        collDB->scanModifiedDirs();


    handleCliArgs();

    delete m_splash;
    m_splash = 0;
}

bool Amarok::genericEventHandler( QWidget *recipient, QEvent *e )
{
    //this is used as a generic event handler for widgets that want to handle
    //typical events in an Amarok fashion

    //to use it just pass the event eg:
    //
    // void Foo::barEvent( QBarEvent *e )
    // {
    //     Amarok::genericEventHandler( this, e );
    // }

    switch( e->type() )
    {
    case QEvent::DragEnter:
        #define e static_cast<QDropEvent*>(e)
        e->setAccepted( KUrl::List::canDecode( e->mimeData() ) );
        break;

    case QEvent::Drop:
    {
        KUrl::List list = KUrl::List::fromMimeData( e->mimeData() );
        if( !list.isEmpty() )
        {
            Q3PopupMenu popup;
            //FIXME this isn't a good way to determine if there is a currentTrack, need playlist() function
            const bool b = EngineController::engine()->loaded();

            popup.insertItem( KIcon( Amarok::icon( "add_playlist" ) ), i18n( "&Append to Playlist" ),
                              Playlist::Append );
            popup.insertItem( KIcon( Amarok::icon( "add_playlist" ) ), i18n( "Append && &Play" ),
                              Playlist::DirectPlay | Playlist::Append );
            if( b )
                popup.insertItem( KIcon( Amarok::icon( "fast_forward" ) ), i18n( "&Queue Track" ),
                              Playlist::Queue );
            popup.addSeparator();
            popup.insertItem( i18n( "&Cancel" ), 0 );

            const int id = popup.exec( recipient->mapToGlobal( e->pos() ) );

            if ( id > 0 )
                The::playlistModel()->insertMedia( list, id );
        }
        else return false;
        #undef e

        break;
    }

    //this like every entry in the generic event handler is used by more than one widget
    //please don't remove!
    case QEvent::Wheel:
    {
        #define e static_cast<QWheelEvent*>(e)

        //this behaviour happens for the systray
        //to override one, override it in that class

        switch( e->state() )
        {
        case Qt::ControlModifier:
        {
            const bool up = e->delta() > 0;

            //if this seems strange to you, please bring it up on #amarok
            //for discussion as we can't decide which way is best!
            if( up ) EngineController::instance()->previous();
            else     EngineController::instance()->next();
            break;
        }
        case Qt::ShiftModifier:
        {
            EngineController::instance()->seekRelative( ( e->delta() / 120 ) * 5000 ); //5 seconds for keyboard seeking.
            break;
        }
        default:
            EngineController::instance()->increaseVolume( e->delta() / Amarok::VOLUME_SENSITIVITY );
        }

        e->accept();
        #undef e

        break;
    }

    case QEvent::Close:

        //KDE policy states we should hide to tray and not quit() when the
        //close window button is pushed for the main widget

        static_cast<QCloseEvent*>(e)->accept(); //if we don't do this the info box appears on quit()!

        if( AmarokConfig::showTrayIcon() && !e->spontaneous() && !kapp->sessionSaving() )
        {
            KMessageBox::information( recipient,
                i18n( "<qt>Closing the main-window will keep Amarok running in the System Tray. "
                      "Use <B>Quit</B> from the menu, or the Amarok tray-icon to exit the application.</qt>" ),
                i18n( "Docking in System Tray" ), "hideOnCloseInfo" );
        }
        else pApp->quit();

        break;

    default:
        return false;
    }

    return true;
}


void App::engineStateChanged( Engine::State state, Engine::State oldState )
{
    Meta::TrackPtr track = EngineController::instance()->currentTrack();
    //track is 0 if the engien state is Empty. we check that in the switch
    switch( state )
    {
    case Engine::Empty:
        mainWindow()->setCaption( "Amarok" );
        TrackToolTip::instance()->clear();
        Amarok::OSD::instance()->setImage( QImage( KIconLoader().iconPath( "amarok", -K3Icon::SizeHuge ) ) );
        break;

    case Engine::Playing:
        if ( oldState == Engine::Paused )
            Amarok::OSD::instance()->OSDWidget::show( i18nc( "state, as in playing", "Play" ) );
        if ( !track->prettyName().isEmpty() )
            //TODO: write a utility function somewhere
            mainWindow()->setCaption( i18n("Amarok - %1", /*bundle.veryNiceTitle()*/ track->prettyName() ) );
        break;

    case Engine::Paused:
        Amarok::OSD::instance()->OSDWidget::show( i18n("Paused") );
        break;

    case Engine::Idle:
        mainWindow()->setCaption( "Amarok" );
        break;

    default:
        ;
    }
}

void App::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    Amarok::OSD::instance()->show( bundle );
    if ( !bundle.prettyTitle().isEmpty() )
        mainWindow()->setCaption( i18n("Amarok - %1", bundle.veryNiceTitle() ) );

    TrackToolTip::instance()->setTrack( bundle );
}

void App::engineTrackPositionChanged( long position, bool /*userSeek*/ )
{
    TrackToolTip::instance()->setPos( position );
}

void App::engineVolumeChanged( int newVolume )
{
    Amarok::OSD::instance()->OSDWidget::volChanged( newVolume );
}

void App::slotConfigEqualizer() //SLOT
{
    EqualizerSetup::instance()->show();
    EqualizerSetup::instance()->raise();
}


void App::slotConfigAmarok( const QByteArray& page )
{
    DEBUG_THREAD_FUNC_INFO

    Amarok2ConfigDialog* dialog = static_cast<Amarok2ConfigDialog*>( KConfigDialog::exists( "settings" ) );

    if( !dialog )
    {
        //KConfigDialog didn't find an instance of this dialog, so lets create it :
        dialog = new Amarok2ConfigDialog( mainWindow(), "settings", AmarokConfig::self() );

        connect( dialog, SIGNAL(settingsChanged(const QString&)), SLOT(applySettings()) );
    }

    //FIXME it seems that if the dialog is on a different desktop it gets lost
    //      what do to? detect and move it?

//    if ( page.isNull() )
          // FIXME
//        dialog->showPage( AmarokConfigDialog::s_currentPage );
//    else
        dialog->showPageByName( page );

    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void App::slotConfigShortcuts()
{
    KShortcutsDialog::configure( Amarok::actionCollection(), KShortcutsEditor::LetterShortcutsAllowed, mainWindow() );
}

void App::slotConfigToolBars()
{
    KEditToolBar dialog( mainWindow()->actionCollection(), mainWindow() );
    dialog.setResourceFile( mainWindow()->xmlFile() );

    dialog.showButton( KEditToolBar::Apply, false );

//     if( dialog.exec() )
//     {
//         mainWindow()->reloadXML();
//         mainWindow()->createGUI();
//     }
}

void App::setUseScores( bool use )
{
    AmarokConfig::setUseScores( use );
    emit useScores( use );
}

void App::setUseRatings( bool use )
{
    AmarokConfig::setUseRatings( use );
    emit useRatings( use );
}

void App::setMoodbarPrefs( bool show, bool moodier, int alter, bool withMusic )
{
    AmarokConfig::setShowMoodbar( show );
    AmarokConfig::setMakeMoodier( moodier );
    AmarokConfig::setAlterMood( alter );
    AmarokConfig::setMoodsWithMusic( withMusic );
    emit moodbarPrefs( show, moodier, alter, withMusic );
}

KIO::Job *App::trashFiles( const KUrl::List &files )
{
    KIO::Job *job = KIO::trash( files, true /*show progress*/ );
    Amarok::StatusBar::instance()->newProgressOperation( job ).setDescription( i18n("Moving files to trash") );
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotTrashResult( KJob* ) ) );
    return job;
}

void App::setRating( int n )
{
    if( !AmarokConfig::useRatings() ) return;

    n *= 2;

    const Engine::State s = EngineController::instance()->engine()->state();
    if( s == Engine::Playing || s == Engine::Paused || s == Engine::Idle )
    {
        Meta::TrackPtr track = EngineController::instance()->currentTrack();
        track->setRating( n );
        Amarok::OSD::instance()->OSDWidget::ratingChanged( track->rating() );
    }
    //PORT 2.0
//     else if( MainWindow::self()->isReallyShown() && Playlist::instance()->qscrollview()->hasFocus() )
//         Playlist::instance()->setSelectedRatings( n );
}

void App::slotTrashResult( KJob *job )
{
    if( job->error() )
        job->uiDelegate()->showErrorMessage();
}

void App::quit()
{
    emit prepareToQuit();
    if( MediaBrowser::instance() && MediaBrowser::instance()->blockQuit() )
    {
        // don't quit yet, as some media devices still have to finish transferring data
        QTimer::singleShot( 100, this, SLOT( quit() ) );
        return;
    }
    KApplication::quit();
}

namespace Amarok
{
    /// @see amarok.h

    //TODO remove these, they suck, do a generic getImage

    QPixmap getPNG( const QString &filename )
    {
        QString file = !filename.endsWith( ".png", Qt::CaseInsensitive ) ? "amarok/images/%1.png" : "amarok/images/%1";

        return QPixmap( KStandardDirs::locate( "data", file.arg( filename ) ), "PNG" );
    }

    QPixmap getJPG( const QString &filename )
    {
        QString file = !filename.endsWith( ".jpg", Qt::CaseInsensitive ) ? "amarok/images/%1.jpg" : "amarok/images/%1";

        return QPixmap( KStandardDirs::locate( "data", QString( "amarok/images/%1.jpg" ).arg( filename ) ), "JPEG" );
    }

    QWidget *mainWindow()
    {
        return pApp->mainWindow();
    }

    KActionCollection *actionCollection()
    {
        return pApp->mainWindow()->actionCollection();
    }

    KConfigGroup config( const QString &group )
    {
        //Slightly more useful config() that allows setting the group simultaneously
//         KGlobal::config()->setGroup( group );
        KConfigGroup configGroup = KGlobal::config()->group( group );
        return configGroup;
    }

    bool invokeBrowser( const QString& url )
    {
        //URL can be in whatever forms KUrl understands - ie most.
        const QString cmd = KShell::quoteArg(AmarokConfig::externalBrowser()) 
            + ' ' + KShell::quoteArg(KUrl( url ).url());
        return KRun::runCommand( cmd, 0L ) > 0;
    }

    namespace ColorScheme
    {
        QColor Base;
        QColor Text;
        QColor Background;
        QColor Foreground;
        QColor AltBase;
    }

    OverrideCursor::OverrideCursor( Qt::CursorShape cursor )
    {
        QApplication::setOverrideCursor( cursor == Qt::WaitCursor ?
                                        Qt::WaitCursor :
                                        Qt::BusyCursor );
    }

    OverrideCursor::~OverrideCursor()
    {
        QApplication::restoreOverrideCursor();
    }

    QString saveLocation( const QString &directory )
    {
        globalDirsMutex.lock();
        QString result = KGlobal::dirs()->saveLocation( "data", QString("amarok/") + directory, true );
        globalDirsMutex.unlock();
        return result;
    }

    QString cleanPath( const QString &path )
    {
        QString result = path;
        // german umlauts
        result.replace( QChar(0x00e4), "ae" ).replace( QChar(0x00c4), "Ae" );
        result.replace( QChar(0x00f6), "oe" ).replace( QChar(0x00d6), "Oe" );
        result.replace( QChar(0x00fc), "ue" ).replace( QChar(0x00dc), "Ue" );
        result.replace( QChar(0x00df), "ss" );

        // some strange accents
        result.replace( QChar(0x00e7), "c" ).replace( QChar(0x00c7), "C" );
        result.replace( QChar(0x00fd), "y" ).replace( QChar(0x00dd), "Y" );
        result.replace( QChar(0x00f1), "n" ).replace( QChar(0x00d1), "N" );

        // czech letters with carons
        result.replace( QChar(0x0161), "s" ).replace( QChar(0x0160), "S" );
        result.replace( QChar(0x010d), "c" ).replace( QChar(0x010c), "C" );
        result.replace( QChar(0x0159), "r" ).replace( QChar(0x0158), "R" );
        result.replace( QChar(0x017e), "z" ).replace( QChar(0x017d), "Z" );
        result.replace( QChar(0x0165), "t" ).replace( QChar(0x0164), "T" );
        result.replace( QChar(0x0148), "n" ).replace( QChar(0x0147), "N" );
        result.replace( QChar(0x010f), "d" ).replace( QChar(0x010e), "D" );

        // accented vowels
        QChar a[] = { 'a', 0xe0,0xe1,0xe2,0xe3,0xe5, 0 };
        QChar A[] = { 'A', 0xc0,0xc1,0xc2,0xc3,0xc5, 0 };
        QChar E[] = { 'e', 0xe8,0xe9,0xea,0xeb,0x11a, 0 };
        QChar e[] = { 'E', 0xc8,0xc9,0xca,0xcb,0x11b, 0 };
        QChar i[] = { 'i', 0xec,0xed,0xee,0xef, 0 };
        QChar I[] = { 'I', 0xcc,0xcd,0xce,0xcf, 0 };
        QChar o[] = { 'o', 0xf2,0xf3,0xf4,0xf5,0xf8, 0 };
        QChar O[] = { 'O', 0xd2,0xd3,0xd4,0xd5,0xd8, 0 };
        QChar u[] = { 'u', 0xf9,0xfa,0xfb,0x16e, 0 };
        QChar U[] = { 'U', 0xd9,0xda,0xdb,0x16f, 0 };
        QChar nul[] = { 0 };
        QChar *replacements[] = { a, A, e, E, i, I, o, O, u, U, nul };

        for( int i = 0; i < result.length(); i++ )
        {
            QChar c = result[ i ];
            for( uint n = 0; replacements[n][0] != QChar(0); n++ )
            {
                for( uint k=0; replacements[n][k] != QChar(0); k++ )
                {
                    if( replacements[n][k] == c )
                    {
                        c = replacements[n][0];
                    }
                }
            }
            result[ i ] = c;
        }
        return result;
    }

    QString asciiPath( const QString &path )
    {
        QString result = path;
        for( int i = 0; i < result.length(); i++ )
        {
            QChar c = result[ i ];
            if( c > QChar(0x7f) || c == QChar(0) )
            {
                c = '_';
            }
            result[ i ] = c;
        }
        return result;
    }

    QString vfatPath( const QString &path )
    {
        QString s = path;

        for( int i = 0; i < s.length(); i++ )
        {
            QChar c = s[ i ];
            if( c < QChar(0x20)
                    || c=='*' || c=='?' || c=='<' || c=='>'
                    || c=='|' || c=='"' || c==':' || c=='/'
                    || c=='\\' )
                c = '_';
            s[ i ] = c;
        }

        uint len = s.length();
        if( len == 3 || (len > 3 && s[3] == '.') )
        {
            QString l = s.left(3).toLower();
            if( l=="aux" || l=="con" || l=="nul" || l=="prn" )
                s = '_' + s;
        }
        else if( len == 4 || (len > 4 && s[4] == '.') )
        {
            QString l = s.left(3).toLower();
            QString d = s.mid(3,1);
            if( (l=="com" || l=="lpt") &&
                    (d=="0" || d=="1" || d=="2" || d=="3" || d=="4" ||
                     d=="5" || d=="6" || d=="7" || d=="8" || d=="9") )
                s = '_' + s;
        }

        while( s.startsWith( '.' ) )
            s = s.mid(1);

        while( s.endsWith( '.' ) )
            s = s.left( s.length()-1 );

        s = s.left(255);
        len = s.length();
        if( s[len-1] == ' ' )
            s[len-1] = '_';

        return s;
    }

    QString decapitateString( const QString &input, const QString &ref )
    {
        QString t = ref.toUpper();
        int length = t.length();
        int commonLength = 0;
        while( length > 0 )
        {
            if ( input.toUpper().startsWith( t ) )
            {
                commonLength = t.length();
                t = ref.toUpper().left( t.length() + length/2 );
                length = length/2;
            }
            else
            {
                t = ref.toUpper().left( t.length() - length/2 );
                length = length/2;
            }
        }
        QString clean = input;
        if( t.endsWith( ' ' ) || !ref.at( t.length() ).isLetterOrNumber() ) // common part ends with a space or complete word
            clean = input.right( input.length() - commonLength ).trimmed();
        return clean;
    }

    void setUseScores( bool use ) { App::instance()->setUseScores( use ); }
    void setUseRatings( bool use ) { App::instance()->setUseRatings( use ); }
    void setMoodbarPrefs( bool show, bool moodier, int alter, bool withMusic )
    { App::instance()->setMoodbarPrefs( show, moodier, alter, withMusic ); }
    KIO::Job *trashFiles( const KUrl::List &files ) { return App::instance()->trashFiles( files ); }
}

#include "app.moc"
