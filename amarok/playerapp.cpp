/***************************************************************************
                      playerapp.cpp  -  description
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

#include "amarokconfig.h"
#include "amarokconfigdialog.h"
#include "amarokdcophandler.h"
#include "browserwin.h"
#include "effectwidget.h"
#include "enginebase.h"
#include "metabundle.h"
#include "osd.h"
#include "playerapp.h"
#include "playerwidget.h"
#include "plugin.h"
#include "pluginmanager.h"
#include "threadweaver.h"        //restoreSession()
#include "playlisttooltip.h"
#include "enginecontroller.h"
#include "vis/socketserver.h"    //please leave directory prefix
#include "amaroksystray.h"

#include <kaboutdata.h>          //initCliArgs()
#include <kaction.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kconfigdialog.h>
#include <kdebug.h>
#include <kglobalaccel.h>
#include <kkeydialog.h>          //slotConfigShortcuts()
#include <klocale.h>
#include <kmessagebox.h>         //applySettings()
#include <kshortcut.h>
#include <kstandarddirs.h>
#include <kstartupinfo.h>        //handleLoaderArgs()
#include <ktip.h>
#include <kurl.h>
#include <kwin.h>                //eventFilter()

#include <qcstring.h>            //initIpc()
#include <qfile.h>               //initEngine()
#include <qpixmap.h>             //QPixmap::setDefaultOptimization()
#include <qregexp.h>
#include <qsize.h>
#include <qserversocket.h>       //initIpc()
#include <qsocketnotifier.h>     //initIpc()
#include <qstring.h>

#include <unistd.h>              //initIpc()
#include <sys/socket.h>          //initIpc()
#include <sys/un.h>              //initIpc()


PlayerApp::PlayerApp()
        : KApplication()
        , m_pGlobalAccel( new KGlobalAccel( this ) )
        , m_pDcopHandler( new AmarokDcopHandler )
        , m_pTray( 0 )
        , m_pOSD( new OSDWidget( "amaroK" ) )
        , m_sockfd( -1 )
        , m_showBrowserWin( false )
        , m_pActionCollection( new KActionCollection( 0, this ) )
{
    //TODO readConfig and applySettings first
    //     reason-> create Engine earlier, so we can restore session asap to get playlist loaded by
    //     the time amaroK is visible

    setName( "amarok" );
    pApp = this; //global

    KStdAction::keyBindings( this, SLOT( slotConfigShortcuts() ), actionCollection() );
    KStdAction::preferences( this, SLOT( slotShowOptions() ), actionCollection() );
    KStdAction::quit( this, SLOT( quit() ), actionCollection() );
    KStdAction::keyBindings( this, SLOT( slotConfigGlobalShortcuts() ),
                             actionCollection(), "options_configure_global_keybinding" );
    actionCollection()->action( "options_configure_global_keybinding" )->setText( i18n( "Configure Global Shortcuts..." ) );

    new KAction( i18n( "Previous Track" ), "player_start", 0, EngineController::instance(),
                 SLOT( previous() ), actionCollection(), "prev" );
    new KAction( i18n( "Play" ), "player_play", 0, EngineController::instance(),
                 SLOT( play() ), actionCollection(), "play" );
    new KAction( i18n( "Stop" ), "player_stop", 0, EngineController::instance(),
                 SLOT( stop() ), actionCollection(), "stop" );
    new KAction( i18n( "Pause" ), "player_pause", 0, EngineController::instance(),
                 SLOT( pause() ), actionCollection(), "pause" );
    new KAction( i18n( "Next Track" ), "player_end", 0,
                 EngineController::instance(), SLOT( next() ), actionCollection(), "next" );

    QPixmap::setDefaultOptimization( QPixmap::MemoryOptim );

    initPlayerWidget();
    initBrowserWin();

    //we monitor for close, hide and show events
    m_pBrowserWin  ->installEventFilter( this );
    m_pPlayerWidget->installEventFilter( this );

    readConfig();
    initIpc();   //initializes Unix domain socket for loader communication, will also hide the splash

    //after this point only analyzer pixmaps will be created
    QPixmap::setDefaultOptimization( QPixmap::BestOptim );

    EngineController::instance()->attach( m_pPlayerWidget );
    EngineController::instance()->attach( this );
    m_pTray = new AmarokSystray( m_pPlayerWidget, actionCollection() ); //show/hide is handled by KConfig XT


    applySettings();  //will create the engine


    //restore session as long as the user isn't asking for stuff to be inserted into the playlist etc.
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    if( args->count() == 0 || args->isSet( "enqueue" ) ) restoreSession(); //resume playback + load prev PLS

    //TODO remember if we were in tray last exit, if so don't show!
    m_pPlayerWidget->show(); //BrowserWin will sponaneously show if appropriate

    //process some events so that the UI appears and things feel more responsive
    kapp->processEvents();

    KTipDialog::showTip( "amarok/data/startupTip.txt", false );

    handleCliArgs();

    #ifdef AMK_NEW_VIS_SYSTEM
    new Vis::SocketServer( this );
    #endif
}


PlayerApp::~PlayerApp()
{
    kdDebug() << k_funcinfo << endl;

    //close loader IPC server socket
    if ( m_sockfd != -1 )
        ::close( m_sockfd );

    //Save current item info in dtor rather than saveConfig() as it is only relevant on exit
    //and we may in the future start to use read and saveConfig() in other situations
    //    kapp->config()->setGroup( "Session" );

    //TODO why is this configXT'd? hardly need to accesss these globally.
    //     and it means they're a pain to extend
    EngineBase *engine = EngineController::instance()->engine();

    if( AmarokConfig::resumePlayback() && !EngineController::instance()->playingURL().isEmpty() )
    {
        AmarokConfig::setResumeTrack( EngineController::instance()->playingURL().url() );

        if ( engine->state() != EngineBase::Empty )
            AmarokConfig::setResumeTime( engine->position() / 1000 );
        else
            AmarokConfig::setResumeTime( -1 );
    }
    else AmarokConfig::setResumeTrack( QString::null ); //otherwise it'll play previous resume next time!

    engine->stop(); //controller does this plus visual stuff we don't need to do on exit

    saveConfig();

    delete m_pPlayerWidget;
    delete m_pBrowserWin;
    delete m_pOSD;
    // delete EngineController
    PluginManager::unload( engine );
}


void PlayerApp::handleCliArgs()
{
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

    if ( args->count() > 0 )
    {
        KURL::List list;
        bool notEnqueue = !args->isSet( "enqueue" );

        for ( int i = 0; i < args->count(); i++ )
            list << args->url( i );

        kdDebug() << "List size: " << list.count() << endl;

        //add to the playlist with the correct arguments ( bool clear, bool play )
        m_pBrowserWin->insertMedia( list, notEnqueue, notEnqueue || args->isSet( "play" ) );
    }
    //we shouldn't let the user specify two of these since it is pointless!
    //so we prioritise, pause > stop > play > next > prev
    //thus pause is the least destructive, followed by stop as brakes are the most important bit of a car(!)
    //then the others seemed sensible. Feel free to modify this order, but please leave justification in the cvs log
    //I considered doing some sanity checks (eg only stop if paused or playing), but decided it wasn't worth it
    else if ( args->isSet( "pause" ) )
        EngineController::instance()->pause();
    else if ( args->isSet( "stop" ) )
        EngineController::instance()->stop();
    else if ( args->isSet( "play" ) ) //will restart if we are playing
        EngineController::instance()->play();
    else if ( args->isSet( "next" ) )
        EngineController::instance()->next();
    else if ( args->isSet( "previous" ) )
        EngineController::instance()->previous();

    args->clear();    //free up memory
}


//this method processes the cli arguments sent by the loader process
void PlayerApp::handleLoaderArgs( QCString args ) //SLOT
{
    //extract startup_env part
    int index = args.find( "|" );
    QCString startup_env = args.left( index );
    args.remove( 0, index + 1 );
    kdDebug() << k_funcinfo << "DESKTOP_STARTUP_ID: " << startup_env << endl;

    //stop startup cursor animation
    setStartupId( startup_env );
    KStartupInfo::appStarted();

    //divide argument line into single strings
    QStringList strlist = QStringList::split( "|", args );

    int argc = strlist.count();
    char* argv[argc];

    for ( int i = 0; i < argc; i++ )
    {
        argv[i] = const_cast<char*>( strlist[i].latin1() );
        kdDebug() << k_funcinfo << " extracted string: " << argv[i] << endl;
    }

    //re-initialize KCmdLineArgs with the new arguments
    KCmdLineArgs::reset();
    initCliArgs( argc, argv );
    handleCliArgs();
}

/////////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////////

void PlayerApp::initCliArgs( int argc, char *argv[] ) //static
{
    static const char *description = I18N_NOOP( "An audio player for KDE" );

    static KCmdLineOptions options[] =
        {
            { "+[URL(s)]", I18N_NOOP( "Files/URLs to Open" ), 0 },
            { "r", 0, 0 },
            { "previous", I18N_NOOP( "Skip backwards in playlist" ), 0 },
            { "p", 0, 0 },
            { "play", I18N_NOOP( "Start playing current playlist" ), 0 },
            { "s", 0, 0 },
            { "stop", I18N_NOOP( "Stop playback" ), 0 },
            { "pause", I18N_NOOP( "Pause playback" ), 0 },
            { "f", 0, 0 },
            { "next", I18N_NOOP( "Skip forwards in playlist" ), 0 },
            { ":", I18N_NOOP("Additional options:"), 0 },
            { "e", 0, 0 },
            { "enqueue", I18N_NOOP( "Enqueue Files/URLs" ), 0 },
            { 0, 0, 0 }
        };

    static KAboutData aboutData( "amarok", I18N_NOOP( "amaroK" ),
                                 APP_VERSION, description, KAboutData::License_GPL,
                                 I18N_NOOP( "(c) 2002-2003, Mark Kretschmann\n(c) 2003-2004, the amaroK developers" ),
                                 I18N_NOOP( "IRC:\nserver: irc.freenode.net / channel: #amarok\n\n"
                                            "Feedback:\namarok-devel@lists.sourceforge.net" ),
                                 I18N_NOOP( "http://amarok.sourceforge.net" ) );

    aboutData.addAuthor( "Christian Muehlhaeuser", "developer", "chris@chris.de", "http://www.chris.de" );
    aboutData.addAuthor( "Mark Kretschmann", "project founder, developer, maintainer", "markey@web.de" );
    aboutData.addAuthor( "Max Howell", "developer", "max.howell@methylblue.com" );
    aboutData.addAuthor( "Stanislav Karchebny", "patches, improvements, visualizations, cleanups, i18n",
                         "berk@upnet.ru" );

    aboutData.addCredit( "Adam Pigg", "analyzer, patches", "adam@piggz.fsnet.co.uk" );
    aboutData.addCredit( "Alper Ayazoglu", "graphics: buttons", "cubon@cubon.de", "http://cubon.de" );
    aboutData.addCredit( "Enrico Ros", "analyzer", "eros.kde@email.it" );
    aboutData.addCredit( "Frederik Holljen", "OSD improvement, patches", "fh@ez.no" );
    aboutData.addCredit( "Jarkko Lehti", "tester, IRC channel operator, whipping", "grue@iki.fi" );
    aboutData.addCredit( "Josef Spillner", "KDE RadioStation code", "spillner@kde.org" );
    aboutData.addCredit( "Markus A. Rykalski", "graphics", "exxult@exxult.de" );
    aboutData.addCredit( "Melchior Franz", "new FFT routine, bugfixes", "mfranz@kde.org" );
    aboutData.addCredit( "Roman Becker", "graphics: amaroK logo", "roman@formmorf.de", "http://www.formmorf.de" );
    aboutData.addCredit( "Scott Wheeler", "Taglib", "wheeler@kde.org" );
    aboutData.addCredit( "The Noatun Authors", "code inspiration", 0, "http://noatun.kde.org" );
    aboutData.addCredit( "Whitehawk Stormchaser", "tester, patches", "zerokode@gmx.net" );

    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options );   // Add our own options.
    PlayerApp::addCmdLineOptions();
}


void PlayerApp::initEngine()
{
    Plugin* plugin = PluginManager::createFromQuery
                         ( "[X-KDE-amaroK-plugintype] == 'engine' and "
                           "Name                      == '" + AmarokConfig::soundSystem() + "'" );

    if ( !plugin ) {
        kdWarning() << k_funcinfo << "Cannot load the specified engine. Trying with another engine..\n";

        //when the engine specified in our config does not exist/work, try to invoke _any_ engine plugin
        plugin = PluginManager::createFromQuery
                     ( "[X-KDE-amaroK-plugintype] == 'engine'" );

        if ( !plugin )
            //this will call abort(), but this causes amarok to crash anyway!
            kdFatal() << k_funcinfo << "No engine plugin found. Aborting.\n";
        AmarokConfig::setSoundSystem( PluginManager::getService( plugin )->name() );

        kdDebug() << k_funcinfo << "setting soundSystem to: " << AmarokConfig::soundSystem() << endl;
    }

    EngineBase *engine = static_cast<EngineBase*>( plugin );
    // feed engine to controller
    EngineController::instance()->setEngine( engine );
    engine->init( m_artsNeedsRestart, SCOPE_SIZE, AmarokConfig::rememberEffects() );

    //called from AmarokPopup
    connect( this, SIGNAL( configureDecoder() ), EngineController::instance()->engine(), SLOT( configureDecoder() ) );
}


void PlayerApp::initIpc()
{
    int m_sockfd = ::socket( AF_UNIX, SOCK_STREAM, 0 );
    if ( m_sockfd == -1 )
    {
        kdWarning() << k_funcinfo << " socket() error\n";
        return;
    }
    sockaddr_un local;
    local.sun_family = AF_UNIX;
    QCString path( ::getenv( "HOME" ) );
    path += "/.kde/share/apps/amarok/.loader_socket";
    ::strcpy( &local.sun_path[0], path );
    ::unlink( path );

    int len = sizeof( local );

    if ( ::bind( m_sockfd, (struct sockaddr*) &local, len ) == -1 )
    {
        kdWarning() << k_funcinfo << " bind() error\n";
        ::close ( m_sockfd );
        m_sockfd = -1;
        return;
    }
    if ( ::listen( m_sockfd, 1 ) == -1 )
    {
        kdWarning() << k_funcinfo << " listen() error\n";
        ::close ( m_sockfd );
        m_sockfd = -1;
        return;
    }

    LoaderServer* server = new LoaderServer( this );
    server->setSocket( m_sockfd );

    connect( server, SIGNAL( loaderArgs( QCString ) ),
             this,     SLOT( handleLoaderArgs( QCString ) ) );
}


void PlayerApp::initBrowserWin()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    m_pBrowserWin = new BrowserWin( 0, "BrowserWin" );

    connect( m_pPlayerWidget, SIGNAL( playlistToggled( bool ) ),
             this,                SLOT  ( slotPlaylistShowHide() ) );

    kdDebug() << "END " << k_funcinfo << endl;
}


void PlayerApp::initPlayerWidget()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    m_pPlayerWidget = new PlayerWidget( 0, "PlayerWidget" );

    connect( m_pPlayerWidget, SIGNAL( effectsWindowActivated() ),
             this,            SLOT( showEffectWidget() ) );

    kdDebug() << "END " << k_funcinfo << endl;
}


void PlayerApp::restoreSession()
{
    //here we restore the session
    //however, do note, this is always done, KDE session management is not involved

    //load previous playlist
    if ( AmarokConfig::savePlaylist() )
        m_pBrowserWin->restoreSessionPlaylist();

    if ( AmarokConfig::resumePlayback() && !AmarokConfig::resumeTrack().isEmpty() )
    {
        MetaBundle *bundle = TagReader::readTags( AmarokConfig::resumeTrack(), true );

        if( bundle )
        {
            EngineController::instance()->play( *bundle );
            delete bundle;

            //see if we also saved the time
            int seconds = AmarokConfig::resumeTime();
            if ( seconds > 0 ) EngineController::instance()->engine()->seek( seconds * 1000 );
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// METHODS
/////////////////////////////////////////////////////////////////////////////////////

//SLOT
void PlayerApp::applySettings()
{
    EngineBase *engine = EngineController::instance()->engine();
    if ( AmarokConfig::soundSystem() != PluginManager::getService( engine )->name() )
    {
        if ( AmarokConfig::soundSystem() == "GstEngine" )
            KMessageBox::information( 0, i18n( "GStreamer support is still experimental. Some features "
                                               "(like effects and visualizations) might not work properly." ) );

        PluginManager::unload( engine );
        EngineController::instance()->setEngine( NULL );
        initEngine();
        engine = EngineController::instance()->engine();

        kdDebug() << k_funcinfo << " AmarokConfig::soundSystem() == " << AmarokConfig::soundSystem() << endl;
    }


    if ( AmarokConfig::hardwareMixer() != engine->isMixerHardware() )
        AmarokConfig::setHardwareMixer( engine->initMixer( AmarokConfig::hardwareMixer() ) );

    EngineController::instance()->setVolume( AmarokConfig::masterVolume() );
    engine->setRestoreEffects( AmarokConfig::rememberEffects() );
    engine->setXfadeLength( AmarokConfig::crossfade() ? AmarokConfig::crossfadeLength() : 0 );

    m_pOSD->setEnabled ( AmarokConfig::osdEnabled() );
    m_pOSD->setFont    ( AmarokConfig::osdFont() );
    m_pOSD->setTextColor   ( AmarokConfig::osdTextColor() );
    m_pOSD->setBackgroundColor( AmarokConfig::osdBackgroundColor() );
    m_pOSD->setDuration( AmarokConfig::osdDuration() );
    m_pOSD->setPosition( (OSDWidget::Position)AmarokConfig::osdAlignment() );
    m_pOSD->setScreen( AmarokConfig::osdScreen() );
    m_pOSD->setOffset( AmarokConfig::osdXOffset(), AmarokConfig::osdYOffset() );

    m_pPlayerWidget->createAnalyzer( false );
    m_pBrowserWin->setFont( AmarokConfig::useCustomFonts() ?
                            AmarokConfig::browserWindowFont() : QApplication::font() );

    QFont font = m_pPlayerWidget->font();
    font.setFamily( AmarokConfig::useCustomFonts() ?
                    AmarokConfig::playerWidgetFont().family() : QApplication::font().family() );
    m_pPlayerWidget->setFont( font );
    m_pPlayerWidget->update(); //FIXME doesn't update the scroller, we require the metaBundle to do that, wait for my metaBundle modifications..

    //TODO delete when not in use
    m_pTray->setShown( AmarokConfig::showTrayIcon() );

    setupColors();
}


void PlayerApp::setOsdEnabled(bool enable)
{
    AmarokConfig::setOsdEnabled(enable);
    m_pOSD->setEnabled ( AmarokConfig::osdEnabled() );
}


void PlayerApp::saveConfig()
{
    AmarokConfig::setBrowserWinPos     ( m_pBrowserWin->pos() );
    AmarokConfig::setBrowserWinSize    ( m_pBrowserWin->size() );
    AmarokConfig::setBrowserWinEnabled ( m_showBrowserWin );
    AmarokConfig::setMasterVolume      ( EngineController::instance()->engine()->volume() );
    AmarokConfig::setPlayerPos         ( m_pPlayerWidget->pos() );
    AmarokConfig::setVersion           ( APP_VERSION );

    m_pBrowserWin->saveConfig();

    AmarokConfig::writeConfig();
}


void PlayerApp::readConfig()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    //we must restart artsd after each version change, so that it picks up any plugin changes
    m_artsNeedsRestart = AmarokConfig::version() != APP_VERSION;

    initEngine();
    EngineBase *engine = EngineController::instance()->engine();
    AmarokConfig::setHardwareMixer( engine->initMixer( AmarokConfig::hardwareMixer() ) );
    EngineController::instance()->setVolume( AmarokConfig::masterVolume() );

    m_pPlayerWidget->move  ( AmarokConfig::playerPos() );
    m_pBrowserWin  ->move  ( AmarokConfig::browserWinPos() );
    m_pBrowserWin  ->resize( AmarokConfig::browserWinSize() );

    m_showBrowserWin = AmarokConfig::browserWinEnabled();
    m_pPlayerWidget->setPlaylistShown( m_showBrowserWin );

    // Actions ==========
    m_pGlobalAccel->insert( "add", i18n( "Add Location" ), 0, KKey("WIN+a"), 0,
                            this, SLOT( slotAddLocation() ), true, true );
    m_pGlobalAccel->insert( "show", i18n( "Show/Hide the Playlist" ), 0, KKey("WIN+p"), 0,
                            this, SLOT( slotPlaylistShowHide() ), true, true );
    m_pGlobalAccel->insert( "play", i18n( "Play" ), 0, KKey("WIN+x"), 0,
                            EngineController::instance(), SLOT( play() ), true, true );
    m_pGlobalAccel->insert( "pause", i18n( "Pause" ), 0, KKey("WIN+c"), 0,
                            EngineController::instance(), SLOT( pause() ), true, true );
    m_pGlobalAccel->insert( "stop", i18n( "Stop" ), 0, KKey("WIN+v"), 0,
                            EngineController::instance(), SLOT( stop() ), true, true );
    m_pGlobalAccel->insert( "next", i18n( "Next Track" ), 0, KKey("WIN+b"), 0,
                            EngineController::instance(), SLOT( next() ), true, true );
    m_pGlobalAccel->insert( "prev", i18n( "Previous Track" ), 0, KKey("WIN+z"), 0,
                            EngineController::instance(), SLOT( previous() ), true, true );
    m_pGlobalAccel->insert( "osd", i18n( "Show OSD" ), 0, KKey("WIN+o"), 0,
                            this, SLOT( slotShowOSD() ), true, true );
    m_pGlobalAccel->insert( "volup", i18n( "Increase volume" ), 0, KKey("WIN+KP_Add"), 0,
                            this, SLOT( slotIncreaseVolume() ), true, true );
    m_pGlobalAccel->insert( "voldn", i18n( "Decrease volume" ), 0, KKey("WIN+KP_Subtract"), 0,
                            this, SLOT( slotDecreaseVolume() ), true, true );

    m_pGlobalAccel->setConfigGroup( "Shortcuts" );
    m_pGlobalAccel->readSettings( kapp->config() );
    m_pGlobalAccel->updateConnections();

    //FIXME use a global actionCollection (perhaps even at global scope)
    actionCollection()->readShortcutSettings( QString::null, kapp->config() );
    m_pBrowserWin->actionCollection()->readShortcutSettings( QString::null, kapp->config() );

    kdDebug() << "END " << k_funcinfo << endl;
}


#include <qpalette.h>
#include <kglobalsettings.h>

void PlayerApp::setupColors()
{
    //FIXME you have to fix the XT stuff for this, we need an enum (and preferably, hard-coded amarok-defaults.. or maybe not)

    if( AmarokConfig::schemeKDE() )
    {
        //TODO this sucks a bit, perhaps just iterate over all children calling "unsetPalette"?
        QColorGroup group = QApplication::palette().active();
        group.setColor( QColorGroup::BrightText, group.highlight() ); //GlowColor
        group.setColor( QColorGroup::Midlight, group.mid() ); //column separator
        m_pBrowserWin->setColors( QPalette( group, group, group ), KGlobalSettings::alternateBackgroundColor() );

    }
    else if( AmarokConfig::schemeAmarok() )
    {

        QColorGroup group = QApplication::palette().active();
        const QColor bg( 32, 32, 80 );
        //const QColor bgAlt( 77, 80, 107 );
        const QColor bgAlt( 57, 64, 98 );
        //bgAlt.setRgb( 69, 68, 102 );
        //bgAlt.setRgb( 85, 84, 117 );
        //bgAlt.setRgb( 74, 81, 107 );
        //bgAlt.setRgb( 83, 86, 112 );

        group.setColor( QColorGroup::Text, Qt::white );
        group.setColor( QColorGroup::Base, bg );
        group.setColor( QColorGroup::Background, bg.dark( 115 ) );

        group.setColor( QColorGroup::Highlight, Qt::white );
        group.setColor( QColorGroup::HighlightedText, bg );
        group.setColor( QColorGroup::BrightText, QColor( 0xff, 0x40, 0x40 ) ); //GlowColor
/*
        group.setColor( QColorGroup::Light,    Qt::red );
        group.setColor( QColorGroup::Midlight, Qt::red );
        group.setColor( QColorGroup::Mid,      Qt::red );
        group.setColor( QColorGroup::Dark,     Qt::red );
        group.setColor( QColorGroup::Shadow,   Qt::red );
*/
        int h,s,v;
        bgAlt.getHsv( &h, &s, &v );
        group.setColor( QColorGroup::Midlight, QColor( h, s/3, v * 1.2, QColor::Hsv ) );

        //FIXME QColorGroup member "disabled" looks very bad (eg for buttons)
        m_pBrowserWin->setColors( QPalette( group, group, group ), bgAlt );

    }
    else
    {
        // we try to be smart: this code figures out contrasting colors for selection and alternate background rows
        QColorGroup group = QApplication::palette().active();
        const QColor fg( AmarokConfig::browserFgColor() );
        const QColor bg( AmarokConfig::browserBgColor() );
        QColor bgAlt, highlight;
        int h, s, v;

        bg.hsv( &h, &s, &v );
        if ( v < 128 )
            v += 50;
        else
            v -= 50;
        bgAlt.setHsv( h, s, v );

        fg.hsv( &h, &s, &v );
        if ( v < 128 )
            v += 150;
        else
            v -= 150;
        if ( v < 0 )
            v = 0;
        if ( v > 255 )
            v = 255;
        highlight.setHsv( h, s, v );

        group.setColor( QColorGroup::Base, bg );
        group.setColor( QColorGroup::Background, bg.dark( 115 ) );
        group.setColor( QColorGroup::Text, fg );
        group.setColor( QColorGroup::Highlight, highlight );
        group.setColor( QColorGroup::HighlightedText, Qt::white );
        group.setColor( QColorGroup::Dark, Qt::darkGray );
        group.setColor( QColorGroup::BrightText, QColor( 0xff, 0x40, 0x40 ) ); //GlowColor

        //FIXME QColorGroup member "disabled" looks very bad (eg for buttons)
        m_pBrowserWin->setColors( QPalette( group, group, group ), bgAlt );
    }
}


void PlayerApp::insertMedia( const KURL::List &list )
{
    m_pBrowserWin->insertMedia( list );
}


bool PlayerApp::eventFilter( QObject *o, QEvent *e )
{
    //Hi! Welcome to one of amaroK's less clear functions!
    //Please don't change anything in here without talking to mxcl or Larson[H] on amaroK
    //as most of this stuff is cleverly crafted and has purpose! Comments aren't always thorough as
    //it tough explaining what is going on! Thanks.

    if( e->type() == QEvent::Close && o == m_pBrowserWin && m_pPlayerWidget->isShown() )
    {
        m_pPlayerWidget->setPlaylistShown( m_showBrowserWin = false );
    }
    else if( e->type() == QEvent::Hide && o == m_pPlayerWidget )
    {
        //if the event is not spontaneous then amaroK was responsible for the event
        //we should therefore hide the playlist as well
        //the only spontaneous hide events we care about are iconification and shading
        if( AmarokConfig::hidePlaylistWindow() && !e->spontaneous() ) m_pBrowserWin->hide();
        else if( AmarokConfig::hidePlaylistWindow() )
        {
            KWin::WindowInfo info = KWin::windowInfo( m_pPlayerWidget->winId() );

            if( !info.valid() ); //do nothing
            else if( info.isMinimized() )
                KWin::iconifyWindow( m_pBrowserWin->winId(), false );
            else if( info.state() & NET::Shaded )
                m_pBrowserWin->hide();
        }
    }
    else if( e->type() == QEvent::Show && o == m_pPlayerWidget )
    {
        //TODO this is broke again if playlist is minimized
        //when fixing you have to make sure that changing desktop doesn't un minimise the playlist

        if( AmarokConfig::hidePlaylistWindow() && m_showBrowserWin && e->spontaneous()/*)
        {
            //this is to battle a kwin bug that affects xinerama users
            //FIXME I commented this out for now because spontaneous show events are sent to widgets
            //when you switch desktops, so this would cause the playlist to deiconify when switching desktop!
            //KWin::deIconifyWindow( m_pBrowserWin->winId(), false );
            m_pBrowserWin->show();
            eatActivateEvent = true;
        }
        else if( */ || m_showBrowserWin )
        {
            //if minimized the taskbar entry for browserwin is shown
            m_pBrowserWin->show();
        }

        if( m_pBrowserWin->isShown() )
        {
            //slotPlaylistHideShow() can make it so the PL is shown but the button is off.
            //this is intentional behavior BTW
            //FIXME it would be nice not to have to set this as it us unclean(TM)
            m_pPlayerWidget->setPlaylistShown( true );
        }
    }
    /*
    //The idea here is to raise both windows when one raises so that both get shown. Unfortunately
    //there just isn't a simple solution that doesn't cause breakage in other areas. Anything more complex
    //than this would probably be too much effort to maintain. I'll leave it commented though in case
    //a future developer has more wisdom than me. IMO if we can get the systray to do it, that'll be enough
    else if( e->type() == QEvent::WindowActivate )
    {
        (o == m_pPlayerWidget ? (QWidget*)m_pBrowserWin : (QWidget*)m_pPlayerWidget)->raise();
    }
    */
    return FALSE;
}


void PlayerApp::engineStateChanged( EngineBase::EngineState state )
{
    switch( state )
    {
        case EngineBase::Empty:
        case EngineBase::Idle:
            m_pDcopHandler->setNowPlaying( QString::null );
            QToolTip::remove( m_pTray );
            QToolTip::add( m_pTray, i18n( "amaroK - Audio Player" ) );
            break;
    }
}


void PlayerApp::engineNewMetaData( const MetaBundle &bundle, bool trackChanged )
{
    if( trackChanged )
    {
        slotShowOSD( bundle );
    }
    m_pDcopHandler->setNowPlaying( bundle.prettyTitle() );
    QToolTip::remove( m_pTray );
    QToolTip::add( m_pTray, bundle.prettyTitle() );
}


void PlayerApp::slotPlaylistShowHide()
{
    //show/hide the playlist global shortcut slot
    //bahavior depends on state of the PlayerWidget and various minimization states

    KWin::WindowInfo info = KWin::windowInfo( m_pBrowserWin->winId() );
    bool isMinimized = info.valid() && info.isMinimized();

    if( !m_pBrowserWin->isShown() )
    {
        if( isMinimized ) KWin::deIconifyWindow( info.win() );
        m_pBrowserWin->setShown( m_showBrowserWin = true );
    }
    else if( isMinimized ) KWin::deIconifyWindow( info.win() );
    else
    {
        KWin::WindowInfo info2 = KWin::windowInfo( m_pPlayerWidget->winId() );
        if( info2.valid() && info2.isMinimized() ) KWin::iconifyWindow( info.win() );
        else
        {
            m_pBrowserWin->setShown( m_showBrowserWin = false );
        }
    }

    // make sure playerwidget button is in sync
    m_pPlayerWidget->setPlaylistShown( m_showBrowserWin );
}


void PlayerApp::showEffectWidget()
{
    if ( !EffectWidget::self )
    {
        EffectWidget::self = new EffectWidget();

        connect( m_pPlayerWidget,              SIGNAL( destroyed() ),
                 EffectWidget::self,           SLOT  ( deleteLater() ) );
        connect( EffectWidget::self,           SIGNAL( destroyed() ),
                 this,        SLOT( slotEffectWidgetDestroyed() ) );

        EffectWidget::self->show();

        if ( EffectWidget::save_geometry.isValid() )
            EffectWidget::self->setGeometry( EffectWidget::save_geometry );
    }
    else
    {
        m_pPlayerWidget->setEffectsWindowShown( false );
        delete EffectWidget::self;
    }
}


void PlayerApp::slotEffectWidgetDestroyed()
 {
     m_pPlayerWidget->setEffectsWindowShown( false );
 }


void PlayerApp::slotShowOptions()
{
    if( !KConfigDialog::showDialog( "settings" ) )
    {
        //KConfigDialog didn't find an instance of this dialog, so lets create it :
        KConfigDialog* dialog = new AmarokConfigDialog( m_pPlayerWidget, "settings", AmarokConfig::self() );

        connect( dialog, SIGNAL( settingsChanged() ), this, SLOT( applySettings() ) );

        dialog->show();
    }
}


void PlayerApp::slotShowOSD( const MetaBundle& bundle )
{
    // Strip HTML tags, expand basic HTML entities
    QString text = QString( bundle.prettyTitle() );

    if ( bundle.length() )
        text += " - " + bundle.prettyLength();

    QString plaintext = text.copy();
    plaintext.replace( QRegExp( "</?(?:font|a|b|i)\\b[^>]*>" ), QString( "" ) );
    plaintext.replace( QRegExp( "&lt;" ), QString( "<" ) );
    plaintext.replace( QRegExp( "&gt;" ), QString( ">" ) );
    plaintext.replace( QRegExp( "&amp;" ), QString( "&" ) );

    m_textForOSD = plaintext;
    slotShowOSD();
}

void PlayerApp::slotShowOSD()
{
    if (!m_textForOSD.isEmpty())
        m_pOSD->showOSD( m_textForOSD );
}

void PlayerApp::slotShowVolumeOSD()
{
    m_pOSD->showOSD( i18n("Volume %1%").arg( EngineController::instance()->engine()->volume() ), true );
}

void PlayerApp::slotIncreaseVolume()
{
    EngineController *controller = EngineController::instance();
    controller->setVolume( controller->engine()->volume() + 100 / 25 );
    slotShowVolumeOSD();
}

void PlayerApp::slotDecreaseVolume()
{
    EngineController *controller = EngineController::instance();
    controller->setVolume( controller->engine()->volume() - 100 / 25 );
    slotShowVolumeOSD();
}

void PlayerApp::slotConfigShortcuts()
{
    KKeyDialog keyDialog( true );

    keyDialog.insert( actionCollection(), i18n( "General" ) );
    keyDialog.insert( m_pBrowserWin->actionCollection(), i18n( "Playlist Window" ) );

    keyDialog.configure();
}

void PlayerApp::slotConfigGlobalShortcuts()
{
    KKeyDialog::configure( m_pGlobalAccel, true, 0, true );
}


////////////////////////////////////////////////////////////////////////////////
// CLASS LoaderServer
////////////////////////////////////////////////////////////////////////////////

LoaderServer::LoaderServer( QObject* parent )
        : QServerSocket( parent )
{}


void LoaderServer::newConnection( int sockfd )
{
    kdDebug() << k_funcinfo << endl;

    char buf[2000];
    int nbytes = recv( sockfd, buf, sizeof(buf) - 1, 0 );

    if ( nbytes < 0 )
        kdDebug() << k_funcinfo << " recv error" << endl;
    else
    {
        buf[nbytes] = '\000';
        QCString result( buf );
        kdDebug() << k_funcinfo << " received: \n" << result << endl;

        if ( !result.contains( "STARTUP" ) )
            emit loaderArgs( result );
    }

    ::close( sockfd );
}


#include "playerapp.moc"
