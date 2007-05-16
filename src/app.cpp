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

#include "amarok.h"
#include "amarokconfig.h"
#include "amarokdbushandler.h"
#include "app.h"
#include "atomicstring.h"
#include "configdialognew.h"
#include "contextbrowser.h"
#include "collectionbrowser.h"
//#include "dbsetup.h"             //firstRunWizard()
#include "debug.h"
#include "devicemanager.h"
#include "mediadevicemanager.h"
#include "enginebase.h"
#include "enginecontroller.h"
#include "equalizersetup.h"
//#include "firstrunwizard.h"
#include "mediabrowser.h"
#include "meta.h"
#include "metabundle.h"
#include "mountpointmanager.h"
#include "osd.h"
#include "playlist.h"
#include "playlistbrowser.h"
#include "playlistwindow.h"
#include "pluginmanager.h"
#include "refreshimages.h"
#include "scriptmanager.h"
#include "scrobbler.h"
#include "sidebar.h"
#include "statusbar.h"
#include "systray.h"
#include "threadmanager.h"
#include "tracktooltip.h"        //engineNewMetaData()

#include <iostream>

#include <kaboutdata.h>
#include <kaction.h>
#include <kcmdlineargs.h>        //initCliArgs()
#include <kcombobox.h>           //firstRunWizard()
#include <kconfigdialogmanager.h>
#include <kcursor.h>             //Amarok::OverrideCursor
#include <kedittoolbar.h>        //slotConfigToolbars()
#include <kglobalaccel.h>        //initGlobalShortcuts()
#include <kglobalsettings.h>     //applyColorScheme()
#include <kiconloader.h>         //amarok Icon
#include <kio/copyjob.h>
#include <kjob.h>
#include <kjobuidelegate.h>
#include <kshortcutsdialog.h>          //slotConfigShortcuts()
#include <klocale.h>
#include <kmessagebox.h>         //applySettings(), genericEventHandler()
#include <krun.h>                //Amarok::invokeBrowser()
#include <kstandarddirs.h>

#include <QCloseEvent>
#include <QDBusInterface>
#include <QDBusReply>
#include <QEvent>              //genericEventHandler()
#include <QEventLoop>          //applySettings()
#include <QFile>
#include <Q3PopupMenu>          //genericEventHandler
#include <QTimer>              //showHyperThreadingWarning()
#include <QToolTip>            //default tooltip for trayicon


QMutex Debug::mutex;
QMutex Amarok::globalDirsMutex;

int App::mainThreadId = 0;

#ifdef Q_WS_MAC
#include <Carbon/Carbon.h>

static AEEventHandlerUPP appleEventProcessorUPP = 0;

OSStatus
appleEventProcessor(const AppleEvent *ae, AppleEvent *, long /*handlerRefCon*/)
{
    OSType aeID = typeWildCard;
    OSType aeClass = typeWildCard;
    AEGetAttributePtr(ae, keyEventClassAttr, typeType, 0, &aeClass, sizeof(aeClass), 0);
    AEGetAttributePtr(ae, keyEventIDAttr, typeType, 0, &aeID, sizeof(aeID), 0);

    if(aeClass == kCoreEventClass)
    {
        if(aeID == kAEReopenApplication)
        {
            if( PlaylistWindow::self() )
                PlaylistWindow::self()->show();
        }
        return noErr;
    }
    return eventNotHandledErr;
}
#endif

AMAROK_EXPORT KAboutData aboutData( "amarok",
    I18N_NOOP( "Amarok" ), APP_VERSION,
    I18N_NOOP( "The audio player for KDE" ), KAboutData::License_GPL,
    I18N_NOOP( "(C) 2002-2003, Mark Kretschmann\n(C) 2003-2007, The Amarok Development Squad" ),
    I18N_NOOP( "IRC:\nirc.freenode.net - #amarok, #amarok.de, #amarok.es\n\nFeedback:\namarok@kde.org\n\n(Build Date: " __DATE__ ")" ),
             ( "http://amarok.kde.org" ) );

App::App()
        : KApplication()
{
    DEBUG_BLOCK

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
                    debug() << "setting PATH=" << path << endl;
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
     new Amarok::DbusContextBrowserHandler();
     new Amarok::DbusCollectionHandler();
     new Amarok::DbusMediaBrowserHandler();
     new Amarok::DbusScriptHandler();
     new Amarok::DbusDevicesHandler();

    // tell AtomicString that this is the GUI thread
    if ( !AtomicString::isMainThread() )
        qWarning("AtomicString was initialized from a thread other than the GUI "
                 "thread. This could lead to memory leaks.");

#ifdef Q_WS_MAC
    appleEventProcessorUPP = AEEventHandlerUPP(appleEventProcessor);
    AEInstallEventHandler(kCoreEventClass, kAEReopenApplication, appleEventProcessorUPP, (long)this, true);
#endif
    QDBusConnection::sessionBus().registerService("org.kde.amarok");
    QTimer::singleShot( 0, this, SLOT( continueInit() ) );
}

App::~App()
{
    DEBUG_BLOCK

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

    // this must be deleted before the connection to the Xserver is
    // severed, or we risk a crash when the QApplication is exited,
    // I asked Trolltech! *smug*
    delete Amarok::OSD::instance();

    AmarokConfig::setVersion( APP_VERSION );
    AmarokConfig::writeConfig();

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
            debug() << "WARNING: urlToDevice needs to be reimplemented with KDE4 technology, it's just a stub at the moment" << endl;
           QDBusInterface mediamanager( "org.kde.kded", "/modules/mediamanager", "org.kde.MediaManager" );
           QDBusReply<QStringList> reply = mediamanager.call( "properties",deviceUrl.fileName() );
           if (!reply.isValid()) {
        debug() << "Invalid reply from mediamanager" << endl;
               return QString();
           }
       QStringList properties = reply;
       if( properties.count()< 6 )
        return QString();
      debug() << "Reply from mediamanager " << properties[5] << endl;
          return properties[5];
        }

        return device;
    }

}


void App::handleCliArgs() //static
{
    static char cwd[PATH_MAX];
    KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();

    if ( args->isSet( "cwd" ) )
    {
        strncpy(cwd, args->getOption( "cwd" ), sizeof(cwd) );
        cwd[sizeof(cwd)-1] = '\0';
        KCmdLineArgs::setCwd( cwd );
    }

    bool haveArgs = false;
    if ( args->count() > 0 )
    {
        haveArgs = true;

        KUrl::List list;
        for( int i = 0; i < args->count(); i++ )
        {
            KUrl url = args->url( i );
            if( url.protocol() == "itpc" || url.protocol() == "pcast" )
                PlaylistBrowser::instance()->addPodcast( url );
            else
                list << url;
        }

        int options = Playlist::DefaultOptions;
        if( args->isSet( "queue" ) )
           options = Playlist::Queue;
        else if( args->isSet( "append" ) || args->isSet( "enqueue" ) )
           options = Playlist::Append;
        else if( args->isSet( "load" ) )
            options = Playlist::Replace;

        if( args->isSet( "play" ) )
            options |= Playlist::DirectPlay;

        Playlist::instance()->insertMedia( list, options );
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
        device = DeviceManager::instance()->convertMediaUrlToDevice( device );
        KUrl::List urls;
        if (EngineController::engine()->getAudioCDContents(device, urls)) {
            Playlist::instance()->insertMedia(
                urls, Playlist::Replace|Playlist::DirectPlay);
        } else { // Default behaviour
            debug() <<
                "Sorry, the engine doesn't support direct play from AudioCD..."
                    << endl;
        }
    }

    if ( args->isSet( "toggle-playlist-window" ) )
    {
        haveArgs = true;
        pApp->m_playlistWindow->showHide();
    }

    static bool firstTime = true;
    if( !firstTime && !haveArgs )
        pApp->m_playlistWindow->activate();
    firstTime = false;

    args->clear();    //free up memory
}


/////////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////////

void App::initCliArgs( int argc, char *argv[] ) //static
{
    static KCmdLineOptions options[] =
        {
            { "+[URL(s)]", I18N_NOOP( "Files/URLs to open" ), 0 },
            { "r", 0, 0 },
            { "previous", I18N_NOOP( "Skip backwards in playlist" ), 0 },
            { "p", 0, 0 },
            { "play", I18N_NOOP( "Start playing current playlist" ), 0 },
            { "t", 0, 0 },
            { "play-pause", I18N_NOOP( "Play if stopped, pause if playing" ), 0 },
            { "pause", I18N_NOOP( "Pause playback" ), 0 },
            { "s", 0, 0 },
            { "stop", I18N_NOOP( "Stop playback" ), 0 },
            { "f", 0, 0 },
            { "next", I18N_NOOP( "Skip forwards in playlist" ), 0 },
            { ":", I18N_NOOP("Additional options:"), 0 },
            { "a", 0, 0 },
            { "append", I18N_NOOP( "Append files/URLs to playlist" ), 0 },
            { "e", 0, 0 },
            { "enqueue", I18N_NOOP("See append, available for backwards compatability"), 0 },
            { "queue", I18N_NOOP("Queue URLs after the currently playing track"), 0 },
            { "l", 0, 0 },
            { "load", I18N_NOOP("Load URLs, replacing current playlist"), 0 },
            { "m", 0, 0 },
            { "toggle-playlist-window", I18N_NOOP("Toggle the Playlist-window"), 0 },
            { "wizard", I18N_NOOP( "Run first-run wizard" ), 0 },
            { "engine <name>", I18N_NOOP( "Use the <name> engine" ), 0 },
            { "cwd <directory>", I18N_NOOP( "Base for relative filenames/URLs" ), 0 },
            { "cdplay <device>", I18N_NOOP("Play an AudioCD from <device> or system:/media/<device>"), 0 },
            { 0, 0, 0 }
        };

    KCmdLineArgs::reset();
    KCmdLineArgs::init( argc, argv, &::aboutData ); //calls KCmdLineArgs::addStdCmdLineOptions()
    KCmdLineArgs::addCmdLineOptions( options );   //add our own options
}


void App::initGlobalShortcuts()
{
    EngineController* const ec = EngineController::instance();
    KAction* action;

//    m_pGlobalAccel->insert( "play", i18n( "Play" ), 0, KKey("WIN+x"), 0, ec, SLOT( play() ), true, true );
    action = new KAction( i18n( "Play" ), m_playlistWindow );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_X ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( play() ) );

//    m_pGlobalAccel->insert( "pause", i18n( "Pause" ), 0, 0, 0, ec, SLOT( pause() ), true, true );
    action = new KAction( i18n( "Pause" ), m_playlistWindow );
    connect( action, SIGNAL( triggered() ), ec, SLOT( pause() ) );

//    m_pGlobalAccel->insert( "play_pause", i18n( "Play/Pause" ), 0, KKey("WIN+c"), 0, ec, SLOT( playPause() ), true, true );
    action = new KAction( i18n( "Play/Pause" ), m_playlistWindow );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_C ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( playPause() ) );

//    m_pGlobalAccel->insert( "stop", i18n( "Stop" ), 0, KKey("WIN+v"), 0, ec, SLOT( stop() ), true, true );
    action = new KAction( i18n( "Stop" ), m_playlistWindow );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_V ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( stop() ) );

//    m_pGlobalAccel->insert( "stop_after_global", i18n( "Stop Playing After Current Track" ), 0, KKey("WIN+CTRL+v"), 0, Playlist::instance()->qscrollview(), SLOT( toggleStopAfterCurrentTrack() ), true, true );
    action = new KAction( i18n( "Stop Playing After Current Track" ), m_playlistWindow );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::CTRL + Qt::Key_V ) );
    connect( action, SIGNAL( triggered() ), Playlist::instance()->qscrollview(), SLOT( toggleStopAfterCurrentTrack() ) );

//    m_pGlobalAccel->insert( "next", i18n( "Next Track" ), 0, KKey("WIN+b"), 0, ec, SLOT( next() ), true, true );
    action = new KAction( i18n( "Next Track" ), m_playlistWindow );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_B ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( next() ) );

//    m_pGlobalAccel->insert( "prev", i18n( "Previous Track" ), 0, KKey("WIN+z"), 0, ec, SLOT( previous() ), true, true );
    action = new KAction( i18n( "Previous Track" ), m_playlistWindow );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_Z ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( previous() ) );

//    m_pGlobalAccel->insert( "volup", i18n( "Increase Volume" ), 0, KKey("WIN+KP_Add"), 0, ec, SLOT( increaseVolume() ), true, true );
    action = new KAction( i18n( "Increase Volume" ), m_playlistWindow );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_Plus ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( increaseVolume() ) );

//    m_pGlobalAccel->insert( "voldn", i18n( "Decrease Volume" ), 0, KKey("WIN+KP_Subtract"), 0, ec, SLOT( decreaseVolume() ), true, true );
    action = new KAction( i18n( "Decrease Volume" ), m_playlistWindow );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_Minus ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( decreaseVolume() ) );


//    m_pGlobalAccel->insert( "seekforward", i18n( "Seek Forward" ), 0, KKey("WIN+Shift+KP_Add"), 0, ec, SLOT( seekForward() ), true, true );
    action = new KAction( i18n( "Seek Forward" ), m_playlistWindow );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::SHIFT + Qt::Key_Plus ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( seekForward() ) );


//    m_pGlobalAccel->insert( "seekbackward", i18n( "Seek Backward" ), 0, KKey("WIN+Shift+KP_Subtract"), 0, ec, SLOT( seekBackward() ), true, true );
    action = new KAction( i18n( "Seek Backward" ), m_playlistWindow );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::SHIFT + Qt::Key_Minus ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( seekBackward() ) );

//    m_pGlobalAccel->insert( "playlist_add", i18n( "Add Media..." ), 0, KKey("WIN+a"), 0, m_playlistWindow, SLOT( slotAddLocation() ), true, true );
    action = new KAction( i18n( "Add Media..." ), m_playlistWindow );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_A ) );
    connect( action, SIGNAL( triggered() ), m_playlistWindow, SLOT( slotAddLocation() ) );

//    m_pGlobalAccel->insert( "show", i18n( "Toggle Playlist Window" ), 0, KKey("WIN+p"), 0, m_playlistWindow, SLOT( showHide() ), true, true );
    action = new KAction( i18n( "Toggle Playlist Window" ), m_playlistWindow );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_P ) );
    connect( action, SIGNAL( triggered() ), m_playlistWindow, SLOT( showHide() ) );


//    m_pGlobalAccel->insert( "osd", i18n( "Show OSD" ), 0, KKey("WIN+o"), 0, Amarok::OSD::instance(), SLOT( forceToggleOSD() ), true, true );
    action = new KAction( i18n( "Show OSD" ), m_playlistWindow );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_O ) );
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
#include <kglobal.h>

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
            : m_codec( QTextCodec::codecForIndex( codecIndex ) )
    {
        debug() << "codec: " << m_codec << endl;
        debug() << "codec-name: " << m_codec->name() << endl;
    }

    ID3v1StringHandler( QTextCodec *codec )
            : m_codec( codec )
    {
        debug() << "codec: " << m_codec << endl;
        debug() << "codec-name: " << m_codec->name() << endl;
    }
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
    playlistWindow()->applySettings();
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
        CollectionView::instance()->renderView(true);
    } //</Collection>
    { //<Context>
        ContextBrowser::instance()->renderView();
    } //</Context>

    {   // delete unneeded cover images from cache
        const QString size = QString::number( AmarokConfig::coverPreviewSize() ) + '@';
        const QDir cacheDir = Amarok::saveLocation( "albumcovers/cache/" );
        const QStringList obsoleteCovers = cacheDir.entryList( QStringList("*") );
        oldForeach( obsoleteCovers )
            if ( !(*it).startsWith( size  ) && !(*it).startsWith( "50@" ) )
                QFile( cacheDir.filePath( *it ) ).remove();
    }

    //if ( !firstTime )
        // Bizarrely and ironically calling this causes crashes for
        // some people! FIXME
        //AmarokConfig::writeConfig();

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


    if ( Amarok::config().readEntry( "First Run", true ) || args->isSet( "wizard" ) ) {
        std::cout << "STARTUP\n" << std::flush; //hide the splashscreen
        firstRunWizard();
        Amarok::config().writeEntry( "First Run", false );
        Amarok::config().sync();
    }

    CollectionDB::instance()->checkDatabase();

    m_mediaDeviceManager = MediaDeviceManager::instance();
    m_playlistWindow = new PlaylistWindow();
#ifndef Q_WS_MAC
    m_tray           = new Amarok::TrayIcon( m_playlistWindow );
#endif
    m_playlistWindow->init(); //creates the playlist, browsers, etc.
    //init playlist window as soon as the database is guaranteed to be usable
    //connect( CollectionDB::instance(), SIGNAL( databaseUpdateDone() ), m_playlistWindow, SLOT( init() ) );
    //initGlobalShortcuts();
    //load previous playlist in separate thread
    if ( restoreSession && AmarokConfig::savePlaylist() )
    {
        Playlist::instance()->restoreSession();
        //Debug::stamp();
        //p->restoreSession();
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
    // Start ScriptManager. Must be created _after_ PlaylistWindow.
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
    //Collection scan is triggered in firstRunWizard if the colelction folder setup was changed in the wizard

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
                Playlist::instance()->insertMedia( list, id );
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
    const MetaBundle &bundle = EngineController::instance()->bundle();
    switch( state )
    {
    case Engine::Empty:
        m_playlistWindow->setCaption( "Amarok" );
        TrackToolTip::instance()->clear();
        Amarok::OSD::instance()->setImage( QImage( KIconLoader().iconPath( "amarok", -K3Icon::SizeHuge ) ) );
        break;

    case Engine::Playing:
        if ( oldState == Engine::Paused )
            Amarok::OSD::instance()->OSDWidget::show( i18nc( "state, as in playing", "Play" ) );
        if ( !bundle.prettyTitle().isEmpty() )
            m_playlistWindow->setCaption( i18n("Amarok - %1", bundle.veryNiceTitle() ) );
        break;

    case Engine::Paused:
        Amarok::OSD::instance()->OSDWidget::show( i18n("Paused") );
        break;

    case Engine::Idle:
        m_playlistWindow->setCaption( "Amarok" );
        break;

    default:
        ;
    }
}

void App::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    Amarok::OSD::instance()->show( bundle );
    if ( !bundle.prettyTitle().isEmpty() )
        m_playlistWindow->setCaption( i18n("Amarok - %1", bundle.veryNiceTitle() ) );

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
        dialog = new Amarok2ConfigDialog( m_playlistWindow, "settings", AmarokConfig::self() );

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
    KShortcutsDialog::configure( Amarok::actionCollection(), KShortcutsEditor::LetterShortcutsAllowed, m_playlistWindow );
}

void App::slotConfigToolBars()
{
    PlaylistWindow* const pw = playlistWindow();
    KEditToolBar dialog( pw->actionCollection(), pw );
    dialog.setResourceFile( pw->xmlFile() );

    dialog.showButton( KEditToolBar::Apply, false );

//     if( dialog.exec() )
//     {
//         playlistWindow()->reloadXML();
//         playlistWindow()->createGUI();
//     }
}

void App::firstRunWizard()
{
#if 0
    ///show firstRunWizard
    DEBUG_BLOCK

    FirstRunWizard wizard;
    setTopWidget( &wizard );
    KConfigDialogManager* config = new KConfigDialogManager(&wizard, AmarokConfig::self(), "wizardconfig");
    config->updateWidgets();
   // connect(config, SIGNAL(settingsChanged()), SLOT(updateSettings()));
    wizard.setCaption( makeStdCaption( i18n( "First-Run Wizard" ) ) );

    if( wizard.exec() != QDialog::Rejected )
    {
        //make sure that the DB config is stored in amarokrc before calling CollectionDB's ctor
        AmarokConfig::setDatabaseEngine(
            QString::number( Amarok::databaseTypeCode( wizard.dbSetup7->databaseEngine->currentText() ) ) );
        config->updateSettings();

        const QStringList oldCollectionFolders = MountPointManager::instance()->collectionFolders();
        wizard.writeCollectionConfig();

        // If wizard is invoked at runtime, rescan collection if folder setup has changed
        if ( !Amarok::config().readEntry( "First Run", true ) &&
             oldCollectionFolders != MountPointManager::instance()->collectionFolders() )
            CollectionDB::instance()->startScan();

    }
#endif
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
        const QString path = EngineController::instance()->playingURL().path();
        CollectionDB::instance()->setSongRating( path, n, true );
        const int rating = CollectionDB::instance()->getSongRating( path );
        EngineController::instance()->updateBundleRating( rating );
        Amarok::OSD::instance()->OSDWidget::ratingChanged( rating );
    }
    else if( PlaylistWindow::self()->isReallyShown() && Playlist::instance()->qscrollview()->hasFocus() )
        Playlist::instance()->setSelectedRatings( n );
}

void App::slotTrashResult( KJob *job )
{
    if( job->error() )
        job->uiDelegate()->showErrorMessage();
}

QWidget *App::mainWindow() const
{
    return static_cast<QWidget*>( m_playlistWindow );
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
        return pApp->playlistWindow();
    }

    KActionCollection *actionCollection()
    {
        return pApp->playlistWindow()->actionCollection();
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
        const QString cmd = "%1 \"%2\"";
        return KRun::runCommand( cmd.arg( AmarokConfig::externalBrowser(), KUrl( url ).url() ) ) > 0;
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

        // accented vowels
        QChar a[] = { 'a', 0xe0,0xe1,0xe2,0xe3,0xe5, 0 };
        QChar A[] = { 'A', 0xc0,0xc1,0xc2,0xc3,0xc5, 0 };
        QChar E[] = { 'e', 0xe8,0xe9,0xea,0xeb, 0 };
        QChar e[] = { 'E', 0xc8,0xc9,0xca,0xcb, 0 };
        QChar i[] = { 'i', 0xec,0xed,0xee,0xef, 0 };
        QChar I[] = { 'I', 0xcc,0xcd,0xce,0xcf, 0 };
        QChar o[] = { 'o', 0xf2,0xf3,0xf4,0xf5,0xf8, 0 };
        QChar O[] = { 'O', 0xd2,0xd3,0xd4,0xd5,0xd8, 0 };
        QChar u[] = { 'u', 0xf9,0xfa,0xfb, 0 };
        QChar U[] = { 'U', 0xd9,0xda,0xdb, 0 };
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

        while( s.startsWith( "." ) )
            s = s.mid(1);

        while( s.endsWith( "." ) )
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
        if( t.endsWith( " " ) || !ref.at( t.length() ).isLetterOrNumber() ) // common part ends with a space or complete word
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
