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

#include "amarokarts.h"
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

#include <vector>
#include <string>
#include <math.h> //play(), visTimer()

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
#include <kuniqueapplication.h>
#include <kurl.h>
#include <kconfigdialog.h>
#include <kwin.h>    //eventFilter()

#include <qpixmap.h> //QPixmap::setDefaultOptimization()
#include <qdir.h>
#include <qpoint.h>
#include <qsize.h>
#include <qstring.h>
#include <qtimer.h>
#include <qvaluelist.h>
#include <qpushbutton.h> //initPlayerWidget()

//statics
EngineBase* PlayerApp::m_pEngine = 0;

PlayerApp::PlayerApp()
        : KUniqueApplication( true, true, false )
        , m_pGlobalAccel( new KGlobalAccel( this ) )
        , m_sliderIsPressed( false )
        , m_pMainTimer( new QTimer( this ) )
        , m_pAnimTimer( new QTimer( this ) )
        , m_length( 0 )
        , m_playRetryCounter( 0 )
        , m_delayTime( 0 )
        , m_pOSD( new OSDWidget() )
        , m_proxyError( false )
{
    setName( "amarok" );
    pApp = this; //global

    QPixmap::setDefaultOptimization( QPixmap::MemoryOptim );

    initPlayerWidget();
    initBrowserWin();

    //we monitor for close, hide and show events
    m_pBrowserWin  ->installEventFilter( this );
    m_pPlayerWidget->installEventFilter( this );

    readConfig();

    { //<EffectConfigWidget>

        //TODO currently we can't only create on demand
        //     as the class holds the effectslist, solution: make the list a static member
        //     with a static functions for retrieval etc.

        EffectWidget *pEffectWidget = new EffectWidget(); //gets destroyed with PlayerWidget

        connect( m_pPlayerWidget->m_pButtonEq, SIGNAL( toggled  ( bool ) ),
                 pEffectWidget,                SLOT  ( setShown ( bool ) ) );
        connect( pEffectWidget,                SIGNAL( sigHide  ( bool ) ),
                 m_pPlayerWidget->m_pButtonEq, SLOT  ( setOn    ( bool ) ) );
        connect( m_pPlayerWidget,              SIGNAL( destroyed() ),
                 pEffectWidget,                SLOT  ( deleteLater() ) );
    } //</EffectConfigWidget>

    //after this point only analyzer pixmaps will be created
    QPixmap::setDefaultOptimization( QPixmap::BestOptim );

    applySettings();
    m_pPlayerWidget->show(); //browserWin gets shown automatically if buttonPl isOn()

    connect( m_pMainTimer, SIGNAL( timeout() ), this, SLOT( slotMainTimer() ) );
    connect( m_pAnimTimer, SIGNAL( timeout() ), m_pPlayerWidget, SLOT( drawScroll() ) );

    //process some events so that the UI appears and things feel more responsive
    kapp->processEvents();

    //start timers and restore session
    m_pMainTimer->start( MAIN_TIMER );
    restoreSession(); //sounds better done here
    m_pAnimTimer->start( ANIM_TIMER ); //looks better done here

    connect( this, SIGNAL( metaData( const MetaBundle& ) ), m_pOSD, SLOT( showOSD( const MetaBundle& ) ) );

    KTipDialog::showTip( "amarok/data/startupTip.txt", false );
}


PlayerApp::~PlayerApp()
{
    kdDebug() << "PlayerApp:~PlayerApp()" << endl;

    //hiding these widgets stops visual oddness
    //I know they won't dissapear because the event Loop isn't updated, but it stops
    //some syncronous updates etc.
    m_pMainTimer->stop();
    m_pAnimTimer->stop();

    m_pPlayerWidget->hide();
    m_pBrowserWin->hide();

    //Save current item info in dtor rather than saveConfig() as it is only relevant on exit
    //and we may in the future start to use read and saveConfig() in other situations
    //    kapp->config()->setGroup( "Session" );

    if( !m_playingURL.isEmpty() )
    {
        AmarokConfig::setResumeTrack( m_playingURL.url() );

        if ( m_pEngine->state() != EngineBase::Empty )
            AmarokConfig::setResumeTime( m_pEngine->position() / 1000 );
        else
            AmarokConfig::setResumeTime( -1 );
    }

    slotStop();

    //killTimers(); doesn't kill QTimers only QObject::startTimer() timers

    saveConfig();

    delete m_pPlayerWidget; //is parent of browserWin (and thus deletes it)
    delete m_pOSD;
    delete m_pEngine;
}


int PlayerApp::newInstance()
{
    KCmdLineArgs * args = KCmdLineArgs::parsedArgs();

    QCString playlistUrl = args->getOption( "playlist" );

    if ( !playlistUrl.isEmpty() )             //playlist
    {
        //FIXME should we remove the playlist option now we figure that out dynamically?
        m_pBrowserWin->insertMedia( KCmdLineArgs::makeURL( playlistUrl ), true );
    }

    if ( args->count() > 0 )
    {
        KURL::List list;

        for ( int i = 0; i < args->count(); i++ )
        {
            list << args->url( i );
        }

        bool b = !args->isSet( "e" ); //b = not enqueue

        m_pBrowserWin->insertMedia( list, b );

        //TODO play first inserted track automatically?
    }

    if ( args->isSet( "r" ) )                 //rewind
        pApp->slotPrev();
    if ( args->isSet( "f" ) )                 //forward
        pApp->slotNext();
    if ( args->isSet( "p" ) )                 //play
        pApp->slotPlay();
    if ( args->isSet( "s" ) )                 //stop
        pApp->slotStop();

    return KUniqueApplication::newInstance();
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


void PlayerApp::initBrowserWin()
{
    kdDebug() << "begin PlayerApp::initBrowserWin()" << endl;

    m_pBrowserWin = new BrowserWin( m_pPlayerWidget, "BrowserWin" );

    connect( m_pPlayerWidget->m_pButtonPl, SIGNAL( toggled( bool ) ),
             m_pBrowserWin,                SLOT  ( setShown( bool ) ) );

    kdDebug() << "end PlayerApp::initBrowserWin()" << endl;
}


void PlayerApp::initPlayerWidget()
{
    kdDebug() << "begin PlayerApp::initPlayerWidget()" << endl;

    m_pPlayerWidget = new PlayerWidget( 0, "PlayerWidget" );

    connect( this,            SIGNAL( metaData( const MetaBundle& ) ),
             m_pPlayerWidget, SLOT  ( setScroll( const MetaBundle& ) ) );

    kdDebug() << "end PlayerApp::initPlayerWidget()" << endl;
}


void PlayerApp::restoreSession()
{
    //here we restore the session
    //however, do note, this is always done, KDE session management is not involved

    if ( AmarokConfig::resumePlayback() )
    {
        //see if we also saved the time
        int seconds = AmarokConfig::resumeTime();

        if ( seconds >= 0 )
        {
            MetaBundle *bundle = TagReader::readTags( AmarokConfig::resumeTrack(), true );

            play( *bundle );

            if ( seconds > 0 )
                m_pEngine->seek( seconds * 1000 );

            delete bundle;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// METHODS
/////////////////////////////////////////////////////////////////////////////////////

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

    m_pOSD->setEnabled( AmarokConfig::osdEnabled() );
    m_pOSD->setFont   ( AmarokConfig::osdFont() );
    m_pOSD->setColor  ( AmarokConfig::osdColor() );

    m_pPlayerWidget->createAnalyzer( false );
    m_pBrowserWin->setFont( AmarokConfig::useCustomFonts() ?
                            AmarokConfig::browserWindowFont() : QApplication::font() );

    reinterpret_cast<QWidget*>(m_pPlayerWidget->m_pTray)->setShown( AmarokConfig::showTrayIcon() );

    setupColors();
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
    m_pGlobalAccel->insert( "add", i18n( "Add Location" ), 0, CTRL + ALT + Key_A, 0,
                            this, SLOT( slotAddLocation() ), true, true );
    m_pGlobalAccel->insert( "show", i18n( "Show/Hide the Playlist" ), 0, CTRL + ALT + Key_H, 0,
                            this, SLOT( slotPlaylistShowHide() ), true, true );
    m_pGlobalAccel->insert( "play", i18n( "Play" ), 0, CTRL + ALT + Key_P, 0,
                            this, SLOT( slotPlay() ), true, true );
    m_pGlobalAccel->insert( "pause", i18n( "Pause" ), 0, CTRL + ALT + Key_C, 0,
                            this, SLOT( slotPause() ), true, true );
    m_pGlobalAccel->insert( "stop", i18n( "Stop" ), 0, CTRL + ALT + Key_S, 0,
                            this, SLOT( slotStop() ), true, true );
    m_pGlobalAccel->insert( "next", i18n( "Next Track" ), 0, CTRL + ALT + Key_B, 0,
                            this, SLOT( slotNext() ), true, true );
    m_pGlobalAccel->insert( "prev", i18n( "Previous Track" ), 0, CTRL + ALT + Key_Z, 0,
                            this, SLOT( slotPrev() ), true, true );

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

        QColor highlight( (bg.red() + bgAlt.red())/2, (bg.green() + bgAlt.green())/2, (bg.blue() + bgAlt.blue())/2 );

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
    //put the o == test last as these events are fairly rare
    //TODO is using a switch better here?

    if( e->type() == QEvent::Close && o == m_pBrowserWin )
    {
        m_pPlayerWidget->m_pButtonPl->setOn( false );
    }
    else if( e->type() == QEvent::Hide && o == m_pPlayerWidget )
    {
        //if the event is not spontaneous then we did the hide
        //we can therefore hide the playlist window

        if( !e->spontaneous() ) m_pBrowserWin->hide();
        else
        {
            //check to see if we've been minimized
            KWin::WindowInfo info = KWin::windowInfo( m_pPlayerWidget->winId() );

            if( info.valid() && info.isMinimized() ) m_pBrowserWin->hide();
        }
    }
    else if( e->type() == QEvent::Show && o == m_pPlayerWidget && m_pPlayerWidget->m_pButtonPl->isOn() )
    {
        m_pBrowserWin->show();
    }

    return FALSE;
}


// SLOTS -----------------------------------------------------------------

//these functions ask the playlist to change the track, if it can change track it notifies us again via a SIGNAL
//the SIGNAL is connected to ::play() below
void PlayerApp::slotPrev() { emit orderPreviousTrack(); }
void PlayerApp::slotPlay() { emit orderCurrentTrack(); }
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

    emit metaData( bundle );
    m_length = bundle.length() * 1000;

    kdDebug() << "[play()] Playing " << url.prettyURL() << endl;
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
    m_pPlayerWidget->defaultScroll          ();
    m_pPlayerWidget->timeDisplay            ( 0 );
    m_pPlayerWidget->m_pSlider->setValue    ( 0 );
    m_pPlayerWidget->m_pSlider->setMaxValue ( 0 );
    m_pPlayerWidget->m_pButtonPlay->setOn   ( false );
    m_pPlayerWidget->m_pButtonPause->setDown( false );
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
    if ( m_sliderIsPressed || ( m_pEngine->state() == EngineBase::Empty ) )
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

    // check if track has ended
    if ( m_pEngine->state() == EngineBase::Idle )
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

#include "playerapp.moc"
