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

#include "amarokconfig.h"
#include "configdialog.h"
#include "amarokdcophandler.h"
#include "systray.h"
#include "playlistwindow.h"
#include "effectwidget.h"
#include "enginebase.h"
#include "enginecontroller.h"
#include "metabundle.h"
#include "osd.h"
#include "app.h"
#include "playerwindow.h"
#include "tracktooltip.h"     //engineNewMetaData()
#include "plugin.h"
#include "pluginmanager.h"
#include "threadweaver.h"        //restoreSession()
#include "socketserver.h"

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
#include <qserversocket.h>       //initIpc()
#include <qsocketnotifier.h>     //initIpc()
#include <qtooltip.h>            //adding tooltip to systray

#include <unistd.h>              //initIpc()
#include <sys/socket.h>          //initIpc()
#include <sys/un.h>              //initIpc()



App::App()
        : KApplication()
        , m_pActionCollection( 0 )
        , m_pGlobalAccel( new KGlobalAccel( this ) )
        , m_pPlayerWidget( 0 ) //will be created in applySettings()
        , m_pDcopHandler( new amaroK::DcopHandler )
        , m_pTray( 0 )
        , m_pOSD( new amaroK::OSD() )
        , m_sockfd( -1 )
        , m_showPlaylistWindow( false )
{
    setName( "amarok" );
    pApp = this; //global

    KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();
    const bool bRestoreSession = args->count() == 0 || args->isSet( "enqueue" );

    EngineController::instance()->attach( this ); //must be done before restoreSession()

    QPixmap::setDefaultOptimization( QPixmap::MemoryOptim );

    m_pPlaylistWindow = new PlaylistWindow(); //creates the actionCollection()
    m_pPlaylist       = m_pPlaylistWindow->playlist();
    m_pTray           = new amaroK::TrayIcon( m_pPlaylistWindow, actionCollection() ); //shown state will be adjusted in applySettings()
    (void)              new Vis::SocketServer( this );

    //load previous playlist
    if( bRestoreSession && AmarokConfig::savePlaylist() ) m_pPlaylistWindow->restoreSessionPlaylist();

    readConfig();
    initIpc(); //initializes Unix domain socket for loader communication, will also hide the splash

    //after this point only analyzer pixmaps will be created
    QPixmap::setDefaultOptimization( QPixmap::BestOptim );

    m_pPlaylistWindow->show(); //TODO remember if we were in tray last exit, if so don't show!

    applySettings();  //show PlayerWidget, show TrayIcon etc.

    //restore session as long as the user isn't asking for stuff to be inserted into the playlist etc.
    if( bRestoreSession ) restoreSession();
    else engineStateChanged( EngineBase::Empty ); //otherwise set a default interface

    KTipDialog::showTip( "amarok/data/startupTip.txt", false );

    handleCliArgs();
}

App::~App()
{
    kdDebug() << k_funcinfo << endl;

    //close loader IPC server socket
    if ( m_sockfd != -1 )
        ::close( m_sockfd );

    //Save current item info in dtor rather than saveConfig() as it is only relevant on exit
    //and we may in the future start to use read and saveConfig() in other situations
    //    kapp->config()->setGroup( "Session" );

    EngineBase *engine = EngineController::engine();

    //TODO why are these configXT'd? We hardly need to accesss these globally.
    //     and it means they're a pain to extend
    if( AmarokConfig::resumePlayback() && !EngineController::instance()->playingURL().isEmpty() )
    {
        AmarokConfig::setResumeTrack( EngineController::instance()->playingURL().url() );

        if ( engine->state() != EngineBase::Empty )
            AmarokConfig::setResumeTime( engine->position() / 1000 );
        else
            AmarokConfig::setResumeTime( -1 );
    }
    else AmarokConfig::setResumeTrack( QString::null ); //otherwise it'll play previous resume next time!

    engine->stop(); //don't call slotStop(), it's slow

    delete m_pPlayerWidget;   //sets some XT keys
    delete m_pPlaylistWindow; //sets some XT keys
    delete m_pOSD;

    saveConfig();

    // delete EngineController
    PluginManager::unload( engine );
}


void App::handleCliArgs()
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
        m_pPlaylistWindow->insertMedia( list, notEnqueue, notEnqueue || args->isSet( "play" ) );
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
void App::handleLoaderArgs( QCString args ) //SLOT
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
    if ( argc < 2 ) return;
    char **argv = new char*[argc]; 

    for ( int i = 0; i < argc; i++ ) {
        argv[i] = qstrdup( strlist[i].local8Bit() );
        kdDebug() << k_funcinfo << " extracted string: " << argv[i] << endl;
    }

    //re-initialize KCmdLineArgs with the new arguments
    KCmdLineArgs::reset();
    initCliArgs( argc, argv );
    handleCliArgs();
    
    //clean up your room
    for ( int i = 0; i < argc; i++ )
        delete[] argv[i];
    delete[] argv;
}

/////////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////////

void App::initCliArgs( int argc, char *argv[] ) //static
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

    //TODO should we i18n this stuff?

    aboutData.addAuthor( "Christian \"babe-magnet\" Muehlhaeuser", "developer, stud", "chris@chris.de", "http://www.chris.de" );
    aboutData.addAuthor( "Frederik \"ich bin kein Deustcher!\" Holljen", "developer, 733t code, OSD improvement, patches", "fh@ez.no" );
    aboutData.addAuthor( "Mark \"it's good, but it's not irssi\" Kretschmann", "project founder, developer, maintainer", "markey@web.de" );
    aboutData.addAuthor( "Max \"sleep? there's no time!\" Howell", "developer, knight of the regression round-table",
                         "max.howell@methylblue.com", "http://www.methyblue.com" );
    aboutData.addAuthor( "Stanislav \"did someone say DCOP?\" Karchebny", "developer, DCOP, improvements, cleanups, i18n",
                         "berk@upnet.ru" );

    aboutData.addCredit( "Adam Pigg", "analyzers, patches", "adam@piggz.fsnet.co.uk" );
    aboutData.addCredit( "Alper Ayazoglu", "graphics: buttons", "cubon@cubon.de", "http://cubon.de" );
    aboutData.addCredit( "Enrico Ros", "analyzers, king of openGL", "eros.kde@email.it" );
    aboutData.addCredit( "Jarkko Lehti", "tester, IRC channel operator, whipping", "grue@iki.fi" );
    aboutData.addCredit( "Josef Spillner", "KDE RadioStation code", "spillner@kde.org" );
    aboutData.addCredit( "Markus A. Rykalski", "graphics", "exxult@exxult.de" );
    aboutData.addCredit( "Melchior Franz", "new FFT routine, bugfixes", "mfranz@kde.org" );
    aboutData.addCredit( "Mike Diehl", "handbook", "madpenguin8@yahoo.com" );
    aboutData.addCredit( "Roman Becker", "graphics: amaroK logo", "roman@formmorf.de", "http://www.formmorf.de" );
    aboutData.addCredit( "Scott Wheeler", "Taglib", "wheeler@kde.org" );
    aboutData.addCredit( "The Noatun Authors", "code and inspiration", 0, "http://noatun.kde.org" );
    aboutData.addCredit( "Whitehawk Stormchaser", "tester, patches", "zerokode@gmx.net" );

    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options );   // Add our own options.
    App::addCmdLineOptions();
}


void App::initEngine()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    amaroK::Plugin* plugin = PluginManager::createFromQuery
                             ( "[X-KDE-amaroK-plugintype] == 'engine' and "
                               "Name                      == '" + AmarokConfig::soundSystem() + '\'' );

    if ( !plugin ) {
        kdWarning() << k_funcinfo << "Cannot load the specified engine. Trying with another engine..\n";

        //when the engine specified in our config does not exist/work, try to invoke _any_ engine plugin
        plugin = PluginManager::createFromQuery( "[X-KDE-amaroK-plugintype] == 'engine'" );

        if ( !plugin )
        {
            class DummyEngine : public EngineBase
            {
                virtual void init( bool&, int, bool ) {}
                virtual bool initMixer( bool ) { return false; }
                virtual bool canDecode( const KURL&, mode_t, mode_t ) { return false; }
                virtual long length() const { return 0; }
                virtual long position() const { return 0; }
                virtual EngineState state() const { return EngineBase::Empty; }
                virtual bool isStream() const { return false; }
                virtual const QObject* play( const KURL& ) { return 0; }
                virtual void play() {}
                virtual void stop() {}
                virtual void pause() {}

                virtual void seek( long ) {}
                virtual void setVolume( int ) {}
            };

            //TODO the kdFatal() command crashes amaroK for some reason
            //TODO decide whether or not to keep the dummy engine
            //kdFatal() << k_funcinfo << "No engine plugin found. Aborting.\n";

            EngineController::setEngine( new DummyEngine() );
            AmarokConfig::setSoundSystem( "Dummy Engine" );

            return;
        }

        AmarokConfig::setSoundSystem( PluginManager::getService( plugin )->name() );
        kdDebug() << k_funcinfo << "setting soundSystem to: " << AmarokConfig::soundSystem() << endl;
    }

    // feed engine to controller
    EngineController::setEngine( static_cast<EngineBase*>( plugin ) );
    EngineController::engine()->init( m_artsNeedsRestart, SCOPE_SIZE, AmarokConfig::rememberEffects() );

    kdDebug() << "END " << k_funcinfo << endl;
}


void App::initIpc()
{
    int m_sockfd = ::socket( AF_UNIX, SOCK_STREAM, 0 );
    if ( m_sockfd == -1 )
    {
        kdWarning() << k_funcinfo << " socket() error\n";
        return;
    }
    sockaddr_un local;
    local.sun_family = AF_UNIX;
    QCString path = ::locateLocal( "socket", "amarok.loader_socket" ).local8Bit();
    ::strcpy( &local.sun_path[0], path );
    ::unlink( path );

    kdDebug() << "Opening control socket on " << path << endl;

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


void App::restoreSession()
{
    //here we restore the session
    //however, do note, this is always done, KDE session management is not involved

    if ( AmarokConfig::resumePlayback() && !AmarokConfig::resumeTrack().isEmpty() )
    {
        MetaBundle *bundle = TagReader::readTags( KURL(AmarokConfig::resumeTrack()), true );

        if( bundle )
        {
            EngineController::instance()->play( *bundle );
            delete bundle;

            //see if we also saved the time
            int seconds = AmarokConfig::resumeTime();
            if ( seconds > 0 ) EngineController::engine()->seek( seconds * 1000 );
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// METHODS
/////////////////////////////////////////////////////////////////////////////////////

//SLOT
void App::applySettings()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    if ( AmarokConfig::soundSystem() != PluginManager::getService( EngineController::engine() )->name() ) {
        PluginManager::unload( EngineController::engine() );
        initEngine();
        AmarokConfig::setHardwareMixer( EngineController::engine()->initMixer( AmarokConfig::hardwareMixer() ) );
        kdDebug() << k_funcinfo << " AmarokConfig::soundSystem() == " << AmarokConfig::soundSystem() << endl;
    }

    EngineController *const controller = EngineController::instance();
    EngineBase *const engine = controller->engine();

    if ( AmarokConfig::hardwareMixer() != engine->isMixerHardware() )
        AmarokConfig::setHardwareMixer( engine->initMixer( AmarokConfig::hardwareMixer() ) );

    controller->setVolume( AmarokConfig::masterVolume() ); //FIXME this shouldn't be here!
    engine->setRestoreEffects( AmarokConfig::rememberEffects() );
    engine->setSoundOutput( AmarokConfig::soundOutput() );
    engine->setXfadeLength( AmarokConfig::crossfade() ? AmarokConfig::crossfadeLength() : 0 );
    
    m_pOSD->setEnabled( AmarokConfig::osdEnabled() );
    m_pOSD->setFont( AmarokConfig::osdFont() );
    m_pOSD->setTextColor( AmarokConfig::osdTextColor() );
    m_pOSD->setBackgroundColor( AmarokConfig::osdBackgroundColor() );
    m_pOSD->setDuration( AmarokConfig::osdDuration() );
    m_pOSD->setPosition( (OSDWidget::Position)AmarokConfig::osdAlignment() );
    m_pOSD->setScreen( AmarokConfig::osdScreen() );
    m_pOSD->setOffset( AmarokConfig::osdXOffset(), AmarokConfig::osdYOffset() );

    if( AmarokConfig::showPlayerWindow() )
    {
        if( !m_pPlayerWidget )
        {
            m_pPlayerWidget = new PlayerWidget( m_pPlaylistWindow, "PlayerWidget", Qt::WType_Dialog );

            m_pPlayerWidget->move( AmarokConfig::playerPos() );
            m_pPlayerWidget->setPlaylistShown( m_showPlaylistWindow );

            m_pPlayerWidget->createAnalyzer( false );

            m_pPlaylistWindow->installEventFilter( this );
            m_pPlayerWidget->installEventFilter( this );

            connect( m_pPlayerWidget, SIGNAL(playlistToggled( bool )),  SLOT(slotPlaylistShowHide()) );
            connect( m_pPlayerWidget, SIGNAL(effectsWindowActivated()), SLOT(showEffectWidget()) );

            m_pPlayerWidget->show();
        }

        QFont font = m_pPlayerWidget->font();
        font.setFamily( AmarokConfig::useCustomFonts() ?
                        AmarokConfig::playerWidgetFont().family() : QApplication::font().family() );
        m_pPlayerWidget->setFont( font ); //NOTE dont use unsetFont(), we use custom font sizes (for now)
        m_pPlayerWidget->update(); //FIXME doesn't update the scroller

    } else if( m_pPlayerWidget ) {

        m_pPlaylistWindow->removeEventFilter( this );
        m_pPlayerWidget->removeEventFilter( this );

        delete m_pPlayerWidget;
        m_pPlayerWidget = 0;
    }


    const QFont font = AmarokConfig::useCustomFonts() ? AmarokConfig::playlistWindowFont() : QApplication::font();
    m_pPlaylistWindow->setFont( font );

    m_pTray->setShown( AmarokConfig::showTrayIcon() ); //TODO delete when not in use

    setupColors();

    kdDebug() << "END " << k_funcinfo << endl;
}


void App::saveConfig()
{
    AmarokConfig::setMasterVolume( EngineController::engine()->volume() ); //engineController should set when volume is changed
    AmarokConfig::setVersion( APP_VERSION );
    AmarokConfig::setPlaylistWindowEnabled( m_showPlaylistWindow ); //TODO should be set when toggled no?

    AmarokConfig::writeConfig();
}


void App::readConfig()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    //we must restart artsd after each version change, so that it picks up any plugin changes
    m_artsNeedsRestart = AmarokConfig::version() != APP_VERSION;

    initEngine();

    EngineController* const ec = EngineController::instance();
    EngineBase* const engine = ec->engine();

    AmarokConfig::setHardwareMixer( engine->initMixer( AmarokConfig::hardwareMixer() ) );
    ec->setVolume( AmarokConfig::masterVolume() );

    m_pPlaylistWindow->move( AmarokConfig::playlistWindowPos() );
    m_pPlaylistWindow->resize( AmarokConfig::playlistWindowSize() );
    m_showPlaylistWindow = AmarokConfig::playlistWindowEnabled();

    // Actions ==========
    m_pGlobalAccel->insert( "add", i18n( "Add Location" ), 0, KKey("WIN+a"), 0,
                            this, SLOT( slotAddLocation() ), true, true );
    m_pGlobalAccel->insert( "show", i18n( "Show/Hide the Playlist" ), 0, KKey("WIN+p"), 0,
                            this, SLOT( slotPlaylistShowHide() ), true, true );
    m_pGlobalAccel->insert( "play", i18n( "Play" ), 0, KKey("WIN+x"), 0,
                            ec, SLOT( play() ), true, true );
    m_pGlobalAccel->insert( "pause", i18n( "Pause" ), 0, KKey("WIN+c"), 0,
                            ec, SLOT( pause() ), true, true );
    m_pGlobalAccel->insert( "stop", i18n( "Stop" ), 0, KKey("WIN+v"), 0,
                            ec, SLOT( stop() ), true, true );
    m_pGlobalAccel->insert( "next", i18n( "Next Track" ), 0, KKey("WIN+b"), 0,
                            ec, SLOT( next() ), true, true );
    m_pGlobalAccel->insert( "prev", i18n( "Previous Track" ), 0, KKey("WIN+z"), 0,
                            ec, SLOT( previous() ), true, true );
    m_pGlobalAccel->insert( "osd", i18n( "Show OSD" ), 0, KKey("WIN+o"), 0,
                            m_pOSD, SLOT( showTrack() ), true, true );
    m_pGlobalAccel->insert( "volup", i18n( "Increase Volume" ), 0, KKey("WIN+KP_Add"), 0,
                            this, SLOT( slotIncreaseVolume() ), true, true );
    m_pGlobalAccel->insert( "voldn", i18n( "Decrease Volume" ), 0, KKey("WIN+KP_Subtract"), 0,
                            this, SLOT( slotDecreaseVolume() ), true, true );

    m_pGlobalAccel->setConfigGroup( "Shortcuts" );
    m_pGlobalAccel->readSettings( kapp->config() );
    m_pGlobalAccel->updateConnections();

    actionCollection()->readShortcutSettings( QString::null, kapp->config() );

    kdDebug() << "END " << k_funcinfo << endl;
}


#include <qpalette.h>
#include <kglobalsettings.h>
#include <qobjectlist.h>

void App::setupColors()
{
    //TODO move to PlaylistWindow?

    if( AmarokConfig::schemeKDE() )
    {
        QObject* const browserBar = m_pPlaylistWindow->child( "BrowserBar" );
        QObjectList* const list = browserBar->queryList( "QWidget" );
        list->prepend( browserBar );

        for( QObject *o = list->first(); o; o = list->next() )
        {
            //We have to unset the palette due to BrowserWin::setColors() setting
            //some widgets' palettes, and thus they won't propagate the changes

            static_cast<QWidget*>(o)->unsetPalette();

            if( o->inherits( "KListView" ) )
            {
                //TODO find out how KListView alternate colors are updated when a
                //     control center colour change is made

                static_cast<KListView*>(o)->setAlternateBackground( KGlobalSettings::alternateBackgroundColor() );
            }
        }

        delete list;
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

        /*PLEASE don't do this, it makes lots of widget ugly
         *instead customise PlaylistWindow::setColors();
         */
        //group.setColor( QColorGroup::Foreground, Qt::white );

        group.setColor( QColorGroup::Text, Qt::white );
        group.setColor( QColorGroup::Base, bg );
        group.setColor( QColorGroup::Background, bg.light(120) );

        group.setColor( QColorGroup::Highlight, Qt::white );
        group.setColor( QColorGroup::HighlightedText, bg );
        group.setColor( QColorGroup::BrightText, QColor( 0xff, 0x40, 0x40 ) ); //GlowColor

        int h,s,v;
        bgAlt.getHsv( &h, &s, &v );
        group.setColor( QColorGroup::Midlight, QColor( h, s/3, (int)(v * 1.2), QColor::Hsv ) ); //column separator in playlist

        //FIXME QColorGroup member "disabled" looks very bad (eg for buttons)
        m_pPlaylistWindow->setColors( QPalette( group, group, group ), bgAlt );

    }
    else
    {
        // we try to be smart: this code figures out contrasting colors for selection and alternate background rows
        QColorGroup group = QApplication::palette().active();
        const QColor fg( AmarokConfig::playlistWindowFgColor() );
        const QColor bg( AmarokConfig::playlistWindowBgColor() );
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
        m_pPlaylistWindow->setColors( QPalette( group, group, group ), bgAlt );
    }
}


void App::insertMedia( const KURL::List &list )
{
    m_pPlaylistWindow->insertMedia( list );
}


bool App::eventFilter( QObject *o, QEvent *e )
{
    //Hi! Welcome to one of amaroK's less clear functions!
    //Please don't change anything in here without talking to mxcl or Larson[H] on amaroK
    //as most of this stuff is cleverly crafted and has purpose! Comments aren't always thorough as
    //it tough explaining what is going on! Thanks.

    //NOTE this eventFilter is only processed if the AmarokConfig::showPlayerWindow() is true

    if( e->type() == QEvent::Close && o == m_pPlaylistWindow && m_pPlayerWidget->isShown() )
    {
        m_pPlayerWidget->setPlaylistShown( m_showPlaylistWindow = false );
    }
    else if( e->type() == QEvent::Hide && o == m_pPlayerWidget )
    {
        //if the event is not spontaneous then amaroK was responsible for the event
        //we should therefore hide the playlist as well
        //the only spontaneous hide events we care about are iconification and shading
        if( AmarokConfig::hidePlaylistWindow() && !e->spontaneous() ) m_pPlaylistWindow->hide();
        else if( AmarokConfig::hidePlaylistWindow() )
        {
            KWin::WindowInfo info = KWin::windowInfo( m_pPlayerWidget->winId() );

            if( !info.valid() ); //do nothing
            else if( info.isMinimized() )
                KWin::iconifyWindow( m_pPlaylistWindow->winId(), false );
            else if( info.state() & NET::Shaded )
                m_pPlaylistWindow->hide();
        }
    }
    else if( e->type() == QEvent::Show && o == m_pPlayerWidget )
    {
        //TODO this is broke again if playlist is minimized
        //when fixing you have to make sure that changing desktop doesn't un minimise the playlist

        if( AmarokConfig::hidePlaylistWindow() && m_showPlaylistWindow && e->spontaneous()/*)
        {
            //this is to battle a kwin bug that affects xinerama users
            //FIXME I commented this out for now because spontaneous show events are sent to widgets
            //when you switch desktops, so this would cause the playlist to deiconify when switching desktop!
            //KWin::deIconifyWindow( m_pPlaylistWindow->winId(), false );
            m_pPlaylistWindow->show();
            eatActivateEvent = true;
        }
        else if( */ || m_showPlaylistWindow )
        {
            //if minimized the taskbar entry for browserwin is shown
            m_pPlaylistWindow->show();
        }

        if( m_pPlaylistWindow->isShown() )
        {
            //slotPlaylistHideShow() can make it so the PL is shown but the button is off.
            //this is intentional behavior BTW
            //FIXME it would be nice not to have to set this as it us unclean(TM)
            m_pPlayerWidget->setPlaylistShown( true );
        }
    }

    return FALSE;
}


void App::engineStateChanged( EngineBase::EngineState state )
{
    switch( state )
    {
        case EngineBase::Empty:
        case EngineBase::Idle:
            m_pDcopHandler->setNowPlaying( QString::null );
            QToolTip::add( m_pTray, i18n( "amaroK - Audio Player" ) );
            break;
        case EngineBase::Paused: // shut up GCC
        case EngineBase::Playing:
            break;
    }
}


void App::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    m_pOSD->showTrack( bundle );
    m_pDcopHandler->setNowPlaying( bundle.prettyTitle() );
    PlaylistToolTip::add( m_pTray, bundle );
}


void App::slotPlaylistShowHide()
{
    //show/hide the playlist global shortcut slot
    //bahavior depends on state of the PlayerWidget and various minimization states

    if( !AmarokConfig::showPlayerWindow() ) { m_pPlaylistWindow->setShown( true ); return; }

    KWin::WindowInfo info = KWin::windowInfo( m_pPlaylistWindow->winId() );
    bool isMinimized = info.valid() && info.isMinimized();

    if( !m_pPlaylistWindow->isShown() )
    {
        if( isMinimized ) KWin::deIconifyWindow( info.win() );
        m_pPlaylistWindow->setShown( m_showPlaylistWindow = true );
    }
    else if( isMinimized ) KWin::deIconifyWindow( info.win() );
    else
    {
        KWin::WindowInfo info2 = KWin::windowInfo( m_pPlayerWidget->winId() );
        if( info2.valid() && info2.isMinimized() ) KWin::iconifyWindow( info.win() );
        else
        {
            m_pPlaylistWindow->setShown( m_showPlaylistWindow = false );
        }
    }

    // make sure playerwidget button is in sync
    m_pPlayerWidget->setPlaylistShown( m_showPlaylistWindow );
}


void App::showEffectWidget()
{
    if ( !EffectWidget::self )
    {
        EffectWidget::self = new EffectWidget();

        if( m_pPlayerWidget )
        {
            connect( m_pPlayerWidget,    SIGNAL( destroyed() ),
                     EffectWidget::self,   SLOT( deleteLater() ) );
            connect( EffectWidget::self, SIGNAL( destroyed() ),
                     m_pPlayerWidget,      SLOT( setEffectsWindowShown() ) ); //defaults to false
        }

        EffectWidget::self->show();

        if ( EffectWidget::save_geometry.isValid() )
            EffectWidget::self->setGeometry( EffectWidget::save_geometry );
    }
    else
    {
        if( m_pPlayerWidget ) m_pPlayerWidget->setEffectsWindowShown( false );
        delete EffectWidget::self;
    }
}

void App::slotShowOptions()
{
    if( !KConfigDialog::showDialog( "settings" ) )
    {
        //KConfigDialog didn't find an instance of this dialog, so lets create it :
        KConfigDialog* dialog = new AmarokConfigDialog( m_pPlaylistWindow, "settings", AmarokConfig::self() );

        connect( dialog, SIGNAL( settingsChanged() ), this, SLOT( applySettings() ) );

        dialog->show();
    }
}

void App::setOsdEnabled( bool enabled ) //SLOT //FIXME required due to dcopHandler
{
    m_pOSD->setEnabled( enabled );
}

void App::slotShowVolumeOSD() //SLOT //FIXME required due to dcopHandler
{
    m_pOSD->showVolume();
}

void App::slotIncreaseVolume()
{
    EngineController *controller = EngineController::instance();
    controller->setVolume( controller->engine()->volume() + 100 / 25 );
    m_pOSD->showVolume();
}

void App::slotDecreaseVolume()
{
    //TODO move these two slots to the engineController
    EngineController *controller = EngineController::instance();
    controller->setVolume( controller->engine()->volume() - 100 / 25 );
    m_pOSD->showVolume();
}

void App::slotConfigShortcuts()
{
    KKeyDialog::configure( actionCollection(), m_pPlaylistWindow );
}

void App::slotConfigGlobalShortcuts()
{
    KKeyDialog::configure( m_pGlobalAccel, true, 0, true );
}

#include <kedittoolbar.h>
void App::slotConfigToolBars()
{
    KEditToolbar dialog( m_pPlaylistWindow->actionCollection() );

    if( dialog.exec() )
    {
        m_pPlaylistWindow->reloadXML();
        m_pPlaylistWindow->createGUI();
    }
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


#include "app.moc"
