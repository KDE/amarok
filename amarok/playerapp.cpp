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
#include "amarokslider.h" //FIXME
#include "browserwin.h"
#include "effectwidget.h"
#include "enginebase.h"
#include "metabundle.h"
#include "osd.h"
#include "playerapp.h"
#include "playerwidget.h"
#include "threadweaver.h" //restoreSession()
#include "playlisttooltip.h"
#include "titleproxy.h"

#include <kaction.h>
#include <kapp.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobalaccel.h>
#include <klocale.h>
#include <kmessagebox.h>    //applySettings()
#include <kshortcut.h>
#include <kstandarddirs.h>
#include <ktip.h>
#include <kurl.h>
#include <kconfigdialog.h>
#include <kwin.h>    //eventFilter()

#include <qcstring.h>     //initIpc()
#include <qpixmap.h> //QPixmap::setDefaultOptimization()
#include <qsize.h>
#include <qserversocket.h>   //initIpc()
#include <qsocketnotifier.h> //initIpc()
#include <qstring.h>
#include <qtimer.h>
#include <qregexp.h>

#include <unistd.h>       //initIpc()
#include <sys/socket.h>   //initIpc()
#include <sys/un.h>       //initIpc()


//statics
EngineBase* PlayerApp::m_pEngine = 0;

PlayerApp::PlayerApp()
        : KApplication()
        , m_pGlobalAccel( new KGlobalAccel( this ) )
        , m_sliderIsPressed( false )
        , m_pMainTimer( new QTimer( this ) )
        , m_pAnimTimer( new QTimer( this ) )
        , m_length( 0 )
        , m_playRetryCounter( 0 )
        , m_delayTime( 0 )
        , m_pOSD( new OSDWidget( "amaroK" ) )
        , m_proxyError( false )
        , m_sockfd( -1 )
{
    //TODO readConfig and applySettings first
    //     reason-> create Engine earlier, so we can restore session asap to get playlist loaded by
    //     the time amaroK is visible

    setName( "amarok" );
    pApp = this; //global

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

    applySettings();  //will create the engine

    //restore session as long as the user isn't asking for stuff to be inserted into the playlist etc.
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    if( args->count() == 0 || args->isSet( "enqueue" ) ) restoreSession(); //resume playback + load prev PLS

    //TODO remember if we were in tray last exit, if so don't show!

    m_pPlayerWidget->show(); //BrowserWin will sponaneously show if appropriate

    connect( m_pMainTimer, SIGNAL( timeout() ), this, SLOT( slotMainTimer() ) );
    connect( m_pAnimTimer, SIGNAL( timeout() ), m_pPlayerWidget, SLOT( drawScroll() ) );
    //process some events so that the UI appears and things feel more responsive
    kapp->processEvents();
    //start timers
    m_pMainTimer->start( MAIN_TIMER );

    connect( this, SIGNAL( metaData( const MetaBundle& ) ), this, SLOT( slotShowOSD( const MetaBundle& ) ) );
    KTipDialog::showTip( "amarok/data/startupTip.txt", false );

    handleCliArgs( KCmdLineArgs::parsedArgs() );
}


PlayerApp::~PlayerApp()
{
    kdDebug() << "PlayerApp:~PlayerApp()" << endl;

    m_pMainTimer->stop();

    //close loader IPC server socket
    if ( m_sockfd != -1 )
        ::close( m_sockfd );

    //hiding these widgets stops visual oddness
    //I know they won't dissapear because the event Loop isn't updated, but it stops
    //some syncronous updates etc.
    m_pPlayerWidget->hide();
    m_pBrowserWin->hide();

    //Save current item info in dtor rather than saveConfig() as it is only relevant on exit
    //and we may in the future start to use read and saveConfig() in other situations
    //    kapp->config()->setGroup( "Session" );

    if( AmarokConfig::resumePlayback() && !m_playingURL.isEmpty() )
    {
        AmarokConfig::setResumeTrack( m_playingURL.url() );

        if ( m_pEngine->state() != EngineBase::Empty )
            AmarokConfig::setResumeTime( m_pEngine->position() / 1000 );
        else
            AmarokConfig::setResumeTime( -1 );
    }
    else AmarokConfig::setResumeTrack( QString::null ); //otherwise it'll play previous resume next time!

    slotStop();

    //killTimers(); doesn't kill QTimers only QObject::startTimer() timers

    saveConfig();

    delete m_pPlayerWidget; //is parent of browserWin (and thus deletes it)
    delete m_pOSD;
    delete m_pEngine;
}


void PlayerApp::handleCliArgs( KCmdLineArgs* args )
{
    if ( args->count() > 0 )
    {
        KURL::List list;
        bool notEnqueue = !args->isSet( "enqueue" );

        for ( int i = 0; i < args->count(); i++ )
            list << args->url( i );

        //add to the playlist with the correct arguments ( bool clear, bool play )
        m_pBrowserWin->insertMedia( list, notEnqueue, notEnqueue || args->isSet( "play" ) );
    }

    //we shouldn't let the user specify two of these since it is pointless!
    //so we prioritise, pause > stop > play > next > prev
    //thus pause is the least destructive, followed by stop as brakes are the most important bit of a car(!)
    //then the others seemed sensible. Feel free to modify this order, but please leave justification in the cvs log
    //I considered doing some sanity checks (eg only stop if paused or playing), but decided it wasn't worth it

    else if ( args->isSet( "pause" ) )
        pApp->slotPause();
    else if ( args->isSet( "stop" ) )
        pApp->slotStop();
    else if ( args->isSet( "play" ) ) //will restart if we are playing
        pApp->slotPlay();
    else if ( args->isSet( "next" ) )
        pApp->slotNext();
    else if ( args->isSet( "previous" ) )
        pApp->slotPrev();

    args->clear();    //free up memory
}

//SLOT
void PlayerApp::handleLoaderArgs( const QCString& args )
{
    //Unfortunately, we must do some ugly string parsing here, since there is (apparently) no way
    //to re-initialize KCmdLineArgs --> FIXME

    QString data = args;
    KURL::List list;
    bool notEnqueue = !data.contains( "-e" );
    QString str;

    for ( int i = 0;; ++i ) {
        str = data.section( " ", i, i );
        if ( str.isEmpty() )
            break;
        if ( !str.startsWith( "-" ) )
            list << str;
    }
    //add to the playlist with the correct arguments ( bool clear, bool play )
    m_pBrowserWin->insertMedia( list, notEnqueue, notEnqueue || data.contains( "-p" ) );

    if ( data.contains( "-s" ) )
        pApp->slotStop();
    if ( data.contains( "-p" ) ) //will restart if we are playing
        pApp->slotPlay();
    if ( data.contains( "-f" ) )
        pApp->slotNext();
    if ( data.contains( "-r" ) )
        pApp->slotPrev();
}

/////////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////////

void PlayerApp::initEngine()
{
    m_pEngine = EngineBase::createEngine( AmarokConfig::soundSystem(),
                                          m_artsNeedsRestart,
                                          SCOPE_SIZE,
                                          AmarokConfig::rememberEffects() );
}


void PlayerApp::initIpc()
{
    int m_sockfd = ::socket( AF_UNIX, SOCK_STREAM, 0 );
    if ( m_sockfd == -1 ) {
        kdDebug() << "[PlayerApp::initIpc()] socket() error\n";
        return;
    }

    sockaddr_un local;
    local.sun_family = AF_UNIX;
    QCString path( ::getenv( "HOME" ) );
    path += "/.kde/share/apps/amarok/.loader_socket";
    ::strcpy( &local.sun_path[0], path );
    ::unlink( path );

    int len = ::strlen( local.sun_path ) + sizeof( local.sun_family );

    if ( ::bind( m_sockfd, (struct sockaddr*) &local, len ) == -1 ) {
        kdDebug() << "[PlayerApp::initIpc()] bind() error\n";
        return;
    }
    if ( ::listen( m_sockfd, 1 ) == -1 ) {
        kdDebug() << "[PlayerApp::initIpc()] listen() error\n";
        return;
    }

    LoaderServer* server = new LoaderServer( this );
    server->setSocket( m_sockfd );

    connect( server, SIGNAL( loaderArgs( const QCString& ) ),
             this,   SLOT( handleLoaderArgs( const QCString& ) ) );
}


void PlayerApp::initBrowserWin()
{
    kdDebug() << "begin PlayerApp::initBrowserWin()" << endl;

    m_pBrowserWin = new BrowserWin( m_pPlayerWidget, "BrowserWin" );

    connect( m_pPlayerWidget->m_pButtonPl, SIGNAL( toggled( bool ) ),
             //m_pBrowserWin,                SLOT  ( setShown( bool ) ) );
             this,                SLOT  ( slotPlaylistShowHide() ) );

    kdDebug() << "end PlayerApp::initBrowserWin()" << endl;
}


void PlayerApp::initPlayerWidget()
{
    kdDebug() << "begin PlayerApp::initPlayerWidget()" << endl;

    m_pPlayerWidget = new PlayerWidget( 0, "PlayerWidget" );

    connect( this,                         SIGNAL( metaData        ( const MetaBundle& ) ),
             m_pPlayerWidget,              SLOT  ( setScroll       ( const MetaBundle& ) ) );
    connect( m_pPlayerWidget->m_pButtonEq, SIGNAL( released        () ),
             this,                         SLOT  ( showEffectWidget() ) );

    kdDebug() << "end PlayerApp::initPlayerWidget()" << endl;
}


void PlayerApp::restoreSession()
{
    //here we restore the session
    //however, do note, this is always done, KDE session management is not involved

    //load previous playlist
    if ( AmarokConfig::savePlaylist() )
        m_pBrowserWin->insertMedia( m_pBrowserWin->defaultPlaylistPath() );

    if ( AmarokConfig::resumePlayback() && !AmarokConfig::resumeTrack().isEmpty() )
    {
        MetaBundle *bundle = TagReader::readTags( AmarokConfig::resumeTrack(), true );
        play( *bundle );
        delete bundle;

        //see if we also saved the time
        int seconds = AmarokConfig::resumeTime();
        if ( seconds > 0 )
            m_pEngine->seek( seconds * 1000 );
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// METHODS
/////////////////////////////////////////////////////////////////////////////////////

//SLOT
void PlayerApp::applySettings()
{
    if ( AmarokConfig::soundSystem() != m_pEngine->name() )
    {
        if ( AmarokConfig::soundSystem() == "gstreamer" )
            KMessageBox::information( 0, i18n( "GStreamer support is still experimental. Some features "
                                               "(like effects and visualizations) might not work properly." ) );

        delete m_pEngine;
        initEngine();

        kdDebug() << "[PlayerApp::applySettings()] AmarokConfig::soundSystem() == " << AmarokConfig::soundSystem() << endl;
    }

    if ( AmarokConfig::hardwareMixer() != m_pEngine->isMixerHardware() )
        AmarokConfig::setHardwareMixer( m_pEngine->initMixer( AmarokConfig::hardwareMixer() ) );

    m_pEngine->setVolume( AmarokConfig::masterVolume() );
    m_pEngine->setRestoreEffects( AmarokConfig::rememberEffects() );
    m_pEngine->setXfadeLength( AmarokConfig::crossfade() ? AmarokConfig::crossfadeLength() : 0 );

    m_pOSD->setEnabled ( AmarokConfig::osdEnabled() );
    m_pOSD->setFont    ( AmarokConfig::osdFont() );
    m_pOSD->setTextColor   ( AmarokConfig::osdColor() );
    m_pOSD->setDuration( AmarokConfig::osdDuration() );

    m_pPlayerWidget->createAnalyzer( false );
    m_pBrowserWin->setFont( AmarokConfig::useCustomFonts() ?
                            AmarokConfig::browserWindowFont() : QApplication::font() );

    reinterpret_cast<QWidget*>(m_pPlayerWidget->m_pTray)->setShown( AmarokConfig::showTrayIcon() );

    setupColors();
}

//SLOT
void PlayerApp::loaderMessage()
{
    kdDebug() << "[PlayerApp::loaderMessage()]\n";
}


bool PlayerApp::isPlaying() const
{
    //this method can get called by PlaylistWidget::restoreCurrentTrack() before engine is initialised
    if ( m_pEngine )
        return m_pEngine->loaded();
    else
        return false;
}


void PlayerApp::saveConfig()
{
    AmarokConfig::setBrowserWinPos     ( m_pBrowserWin->pos() );
    AmarokConfig::setBrowserWinSize    ( m_pBrowserWin->size() );
    AmarokConfig::setBrowserWinEnabled ( m_pPlayerWidget->m_pButtonPl->isOn() );
    AmarokConfig::setMasterVolume      ( m_pEngine->volume() );
    AmarokConfig::setPlayerPos         ( m_pPlayerWidget->pos() );
    AmarokConfig::setVersion           ( APP_VERSION );

    m_pBrowserWin->saveConfig();

    AmarokConfig::writeConfig();
}


void PlayerApp::readConfig()
{
    kdDebug() << "begin PlayerApp::readConfig()" << endl;

    //we must restart artsd after each version change, so that it picks up any plugin changes
    m_artsNeedsRestart = AmarokConfig::version() != APP_VERSION;

    initEngine();

    AmarokConfig::setHardwareMixer( m_pEngine->initMixer( AmarokConfig::hardwareMixer() ) );
    m_pEngine->setVolume( AmarokConfig::masterVolume() );
    m_pPlayerWidget->m_pVolSlider->setValue( m_pEngine->volume() );

    m_pPlayerWidget->move  ( AmarokConfig::playerPos() );
    m_pBrowserWin  ->move  ( AmarokConfig::browserWinPos() );
    m_pBrowserWin  ->resize( AmarokConfig::browserWinSize() );

    m_pPlayerWidget->m_pButtonPl->setOn( AmarokConfig::browserWinEnabled() );

    // Actions ==========
    m_pGlobalAccel->insert( "add", i18n( "Add Location" ), 0, KKey("WIN+a"), 0,
                            this, SLOT( slotAddLocation() ), true, true );
    m_pGlobalAccel->insert( "show", i18n( "Show/Hide the Playlist" ), 0, KKey("WIN+p"), 0,
                            this, SLOT( slotPlaylistShowHide() ), true, true );
    m_pGlobalAccel->insert( "play", i18n( "Play" ), 0, KKey("WIN+x"), 0,
                            this, SLOT( slotPlay() ), true, true );
    m_pGlobalAccel->insert( "pause", i18n( "Pause" ), 0, KKey("WIN+c"), 0,
                            this, SLOT( slotPause() ), true, true );
    m_pGlobalAccel->insert( "stop", i18n( "Stop" ), 0, KKey("WIN+v"), 0,
                            this, SLOT( slotStop() ), true, true );
    m_pGlobalAccel->insert( "next", i18n( "Next Track" ), 0, KKey("WIN+b"), 0,
                            this, SLOT( slotNext() ), true, true );
    m_pGlobalAccel->insert( "prev", i18n( "Previous Track" ), 0, KKey("WIN+z"), 0,
                            this, SLOT( slotPrev() ), true, true );
    m_pGlobalAccel->insert( "osd", i18n( "Show OSD" ), 0, KKey("WIN+o"), 0,
                            this, SLOT( slotShowOSD() ), true, true );

    m_pGlobalAccel->setConfigGroup( "Shortcuts" );
    m_pGlobalAccel->readSettings( kapp->config() );
    m_pGlobalAccel->updateConnections();

    //FIXME use a global actionCollection (perhaps even at global scope)
    m_pPlayerWidget->m_pActionCollection->readShortcutSettings( QString::null, kapp->config() );
    m_pBrowserWin->m_pActionCollection->readShortcutSettings( QString::null, kapp->config() );

    kdDebug() << "end PlayerApp::readConfig()" << endl;
}


#include <qpalette.h>
#include <kglobalsettings.h>

void PlayerApp::setupColors()
{
    //FIXME you have to fix the XT stuff for this, we need an enum (and preferably, hard-coded amarok-defaults.. or maybe not)

    if( AmarokConfig::schemeKDE() )
    {
        //TODO this sucks a bit, perhaps just iterate over all children calling "unsetPalette"?
        m_pBrowserWin->setColors( QApplication::palette(), KGlobalSettings::alternateBackgroundColor() );

    } else if( AmarokConfig::schemeAmarok() ) {

        QColorGroup group = QApplication::palette().active();
        const QColor bg( 32, 32, 80 );
        //const QColor bgAlt( 77, 80, 107 );
        const QColor bgAlt( 57, 64, 98 );
        //bgAlt.setRgb( 69, 68, 102 );
        //bgAlt.setRgb( 85, 84, 117 );
        //bgAlt.setRgb( 74, 81, 107 );
        //bgAlt.setRgb( 83, 86, 112 );

//         QColor highlight( (bg.red() + bgAlt.red())/2, (bg.green() + bgAlt.green())/2, (bg.blue() + bgAlt.blue())/2 );

        group.setColor( QColorGroup::Text, Qt::white );
        group.setColor( QColorGroup::Base, bg );
        group.setColor( QColorGroup::Background, bg.dark( 115 ) );
        //group.setColor( QColorGroup::Button, QColor( 0, 112, 255 ) );

        group.setColor( QColorGroup::Highlight, Qt::white );
        group.setColor( QColorGroup::HighlightedText, bg );

        group.setColor( QColorGroup::Light,    Qt::white );
        group.setColor( QColorGroup::Midlight, group.background() );
        group.setColor( QColorGroup::Dark,     Qt::darkGray );
        group.setColor( QColorGroup::Mid,      Qt::blue );

        //FIXME QColorGroup member "disabled" looks very bad (eg for buttons)
        m_pBrowserWin->setColors( QPalette( group, group, group ), bgAlt );

    } else {
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
        m_pPlayerWidget->m_pButtonPl->setOn( false );
        return TRUE; //so we don't end up in infinite loop!
    }
    else if( e->type() == QEvent::Hide && o == m_pPlayerWidget )
    {
        m_pAnimTimer->stop();

        //if the event is not spontaneous then amaroK was responsible for the event
        //we should therefore hide the playlist as well
        //the only spontaneous hide event we care about is minimization
        if( AmarokConfig::hidePlaylistWindow() && !e->spontaneous() ) m_pBrowserWin->hide();
        else if( AmarokConfig::hidePlaylistWindow() )
        {
            //check to see if we've been minimized
            KWin::WindowInfo info = KWin::windowInfo( m_pPlayerWidget->winId() );

            // minimize not hide when playerwidget is minimized
            if( info.valid() && info.isMinimized() )
                KWin::iconifyWindow( m_pBrowserWin->winId(), false );
        }
    }
    else if( e->type() == QEvent::Show && o == m_pPlayerWidget )
    {
        m_pAnimTimer->start( ANIM_TIMER );

        //TODO this is broke again if playlist is minimized
        //when fixing you have to make sure that changing desktop doesn't un minimise the playlist

        if( AmarokConfig::hidePlaylistWindow() && m_pPlayerWidget->m_pButtonPl->isOn() && e->spontaneous())
        {
            //this is to battle a kwin bug that affects xinerama users
            //FIXME I commented this out for now because spontaneous show events are sent to widgets
            //when you switch desktops, so this would cause the playlist to deiconify when switching desktop!
            //KWin::deIconifyWindow( m_pBrowserWin->winId(), false );
            m_pBrowserWin->show();
        }
        else if( m_pPlayerWidget->m_pButtonPl->isOn() )
        {
            //if minimized the taskbar entry for browserwin is shown
            m_pBrowserWin->show();
        }

        if( m_pBrowserWin->isShown() )
        {
            //slotPlaylistHideShow() can make it so the PL is shown but the button is off.
            //this is intentional behavior BTW
            //FIXME it would be nice not to have to set this as it us unclean(TM)
            IconButton *w = m_pPlayerWidget->m_pButtonPl;
            w->blockSignals( true );
            w->setOn( true );
            w->blockSignals( false );
        }
    }

    return FALSE;
}


// SLOTS -----------------------------------------------------------------

//these functions ask the playlist to change the track, if it can change track it notifies us again via a SIGNAL
//the SIGNAL is connected to ::play() below

void PlayerApp::slotPlay()
{
    kdDebug() << "[PlayerApp::slotPlay()] me got started" << endl;
    if ( m_pEngine->state() == EngineBase::Paused )
    {
        slotPause();
        m_pPlayerWidget->m_pButtonPlay->setDown( TRUE );
        m_pPlayerWidget->m_pButtonPlay->setOn( TRUE );
    } else
        emit orderCurrentTrack();
}

void PlayerApp::slotPrev() { emit orderPreviousTrack(); }
void PlayerApp::slotNext() { emit orderNextTrack(); }


void PlayerApp::play( const MetaBundle &bundle )
{
    const KURL &url = bundle.m_url;
    m_playingURL = url;
    emit currentTrack( url );

    if ( AmarokConfig::titleStreaming() && !m_proxyError && !url.isLocalFile() )
    {
        TitleProxy::Proxy *pProxy = new TitleProxy::Proxy( url );
        m_pEngine->open( pProxy->proxyUrl() );

        connect( m_pEngine, SIGNAL( endOfTrack  () ),
                 pProxy,    SLOT  ( deleteLater () ) );
        connect( pProxy,    SIGNAL( error       () ),
                 this,      SLOT  ( proxyError  () ) );
        connect( pProxy,    SIGNAL( metaData    ( const MetaBundle& ) ),
                 this,      SIGNAL( metaData    ( const MetaBundle& ) ) );
    }
    else
        m_pEngine->open( url );

    m_proxyError = false;

    //TODO replace currentTrack with this, and in PlaylistWidget do a compare type function to see if there is any new data
    emit metaData( bundle );

    m_length = bundle.length() * 1000;

    kdDebug() << "[play()] " << url.prettyURL() << endl;
    m_pEngine->play();

    m_pPlayerWidget->m_pSlider->setValue    ( 0 );
    m_pPlayerWidget->m_pSlider->setMaxValue ( m_length );

    //interface consistency
    m_pPlayerWidget->m_pButtonPlay ->setOn  ( true );
    m_pPlayerWidget->m_pButtonPause->setDown( false );
}


void PlayerApp::proxyError()
{
    kdWarning() << "[PlayerApp::proxyError()] TitleProxy error! Switching to normal playback.." << endl;

    m_proxyError = true;
    slotPlay();
}


void PlayerApp::slotPause()
{
    if ( m_pEngine->loaded() )
    {
        if ( m_pEngine->state() == EngineBase::Paused )
        {
            m_pEngine->play();
            m_pPlayerWidget->m_pButtonPause->setDown( false );
        }
        else
        {
            m_pEngine->pause();
            m_pPlayerWidget->m_pButtonPause->setDown( true );
        }
    }
}


void PlayerApp::slotStop()
{
    m_pEngine->stop();

    m_length = 0;
    m_playingURL = KURL();
    m_pPlayerWidget->defaultScroll          ();
    m_pPlayerWidget->timeDisplay            ( 0 );
    m_pPlayerWidget->m_pSlider->setValue    ( 0 );
    m_pPlayerWidget->m_pSlider->setMaxValue ( 0 );
    m_pPlayerWidget->m_pButtonPlay->setOn   ( false );
    m_pPlayerWidget->m_pButtonPause->setDown( false );
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
        m_pBrowserWin->setShown( true );
    }
    else if( isMinimized ) KWin::deIconifyWindow( info.win() );
    else
    {
        KWin::WindowInfo info2 = KWin::windowInfo( m_pPlayerWidget->winId() );
        if( info2.valid() && info2.isMinimized() ) KWin::iconifyWindow( info.win() );
        else m_pBrowserWin->setShown( false );
    }

    //only do if shown so that it doesn't affect expected layout after restore from systray
    if( m_pPlayerWidget->isShown() )
    {
        IconButton *w = m_pPlayerWidget->m_pButtonPl;
        w->blockSignals( true );
        w->setOn( m_pBrowserWin->isShown() );
        w->blockSignals( false );

    }




}


bool PlayerApp::playObjectConfigurable()
{
    //     if ( m_pPlayObject && !m_pPlayObject->object().isNull() && !m_pPlayerWidget->m_pPlayObjConfigWidget )
    //     {
    //         Arts::TraderQuery query;
    //         query.supports( "Interface", "Arts::GuiFactory" );
    //         query.supports( "CanCreate", m_pPlayObject->object()._interfaceName() );
    //
    //         std::vector<Arts::TraderOffer> *queryResults = query.query();
    //         bool yes = queryResults->size();
    //         delete queryResults;
    //
    //         return yes;
    //     }

    return false;
}


void PlayerApp::slotSliderPressed()
{
    m_sliderIsPressed = true;
}


void PlayerApp::slotSliderReleased()
{
    if ( m_pEngine->state() == EngineBase::Playing )
    {
        m_pEngine->seek( m_pPlayerWidget->m_pSlider->value() );
    }

    m_sliderIsPressed = false;
}


void PlayerApp::slotSliderChanged( int value )
{
    if ( m_sliderIsPressed )
    {
        value /= 1000;    // ms -> sec

        m_pPlayerWidget->timeDisplay( value );
    }
}


void PlayerApp::slotVolumeChanged( int value )
{
    AmarokConfig::setMasterVolume( value );
    m_pEngine->setVolume( value );
}


void PlayerApp::slotMainTimer()
{
    if ( m_sliderIsPressed || m_playingURL.isEmpty() )
        return;

    m_pPlayerWidget->m_pSlider->setValue( m_pEngine->position() );

    // <Draw TimeDisplay>
    if ( m_pPlayerWidget->isVisible() )
    {
        m_pPlayerWidget->timeDisplay( m_pEngine->position() / 1000 );
    }
    // </Draw TimeDisplay>

    // <Crossfading>
    if ( ( AmarokConfig::crossfade() ) &&
         ( !m_pEngine->isStream() ) &&
         ( m_length ) &&
         ( m_length - m_pEngine->position() < AmarokConfig::crossfadeLength() )  )
    {
        slotNext();
        return;
    }

    // check if track has ended or is broken
    if ( m_pEngine->state() == EngineBase::Empty ||
         m_pEngine->state() == EngineBase::Idle )
    {
        if ( AmarokConfig::trackDelayLength() > 0 ) //this can occur syncronously to XFade and not be fatal
        {
            //delay before start of next track, without freezing the app
            m_delayTime += MAIN_TIMER;
            if ( m_delayTime >= AmarokConfig::trackDelayLength() )
            {
                m_delayTime = 0;
                slotNext();
            }
        }
        else if( m_pBrowserWin->isAnotherTrack() )
            slotNext();
        else
            slotStop();
    }
}


void PlayerApp::showEffectWidget()
{
    if ( !EffectWidget::self )
    {
        EffectWidget::self = new EffectWidget();

        connect( EffectWidget::self,           SIGNAL( destroyed() ),
                 m_pPlayerWidget->m_pButtonEq, SLOT  ( setOff   () ) );
        connect( m_pPlayerWidget,              SIGNAL( destroyed() ),
                 EffectWidget::self,           SLOT  ( deleteLater() ) );

        EffectWidget::self->show();

        if ( EffectWidget::save_geometry.isValid() )
            EffectWidget::self->setGeometry( EffectWidget::save_geometry );
    }
    else
        delete EffectWidget::self;
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

// going to remove OSDWidget::showOSD(const MetaBundle&)
void PlayerApp::slotShowOSD( const MetaBundle& bundle )
{
   // Strip HTML tags, expand basic HTML entities
   QString text = QString( "%1 - %2" ).arg( bundle.prettyTitle(), bundle.prettyLength() );

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


////////////////////////////////////////////////////////////////////////////////
// CLASS LoaderServer
////////////////////////////////////////////////////////////////////////////////

LoaderServer::LoaderServer( QObject* parent )
    : QServerSocket( parent )
{}


void LoaderServer::newConnection( int sockfd )
{
    kdDebug() << "[LoaderServer::newConnection()]\n";

    char buf[2000];
    int nbytes = recv( sockfd, buf, sizeof(buf) - 1, 0 );

    if ( nbytes < 0 )
        kdDebug() << "[LoaderServer::newConnection()] recv error" << endl;
    else
    {
        buf[nbytes] = '\000';
        QCString result( buf );
        kdDebug() << result << endl;

        emit loaderArgs( result );
    }

    ::close( sockfd );
}


#include "playerapp.moc"
