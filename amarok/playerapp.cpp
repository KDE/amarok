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

#include "amarokarts/amarokarts.h"
#include "amarokbutton.h"
#include "amarokconfig.h"
#include "amarokconfigdialog.h"
#include "amarokslider.h"
#include "analyzers/analyzerbase.h"
#include "browserwin.h"
#include "effectwidget.h"
#include "engine/enginebase.h"
#include "fht.h"
#include "metabundle.h" //play( const KURL& )
#include "osd.h"
#include "playerapp.h"
#include "playerwidget.h"
#include "threadweaver.h" //restoreSession()
#include "playlisttooltip.h"
#include "titleproxy/titleproxy.h"

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
        , m_pFht( new FHT( SCOPE_SIZE ) )
        , m_pOSD( new OSDWidget() )
        , m_proxyError( false )
{
    setName( "amarok" );
    pApp = this; //global

    initPlayerWidget();
    initBrowserWin();
    
    //we monitor for close, hide and show events
    m_pBrowserWin  ->installEventFilter( this );
    m_pPlayerWidget->installEventFilter( this );

    readConfig();

    { //<AmarokConfigDialog>
        //TODO can we create this on demand only? it would save a whole bunch of memory
        KConfigDialog *dialog = new AmarokConfigDialog( m_pPlayerWidget, "settings", AmarokConfig::self() );
        
        connect( dialog, SIGNAL( settingsChanged() ), this, SLOT( applySettings() ) );
    } //</AmarokConfigDialog>
    
    { //<EffectConfigWidget>
        EffectWidget *pEffectWidget = new EffectWidget(); //gets destroyed with PlayerWidget
        
        connect( m_pPlayerWidget->m_pButtonEq, SIGNAL( toggled  ( bool ) ),
                 pEffectWidget,                SLOT  ( setShown ( bool ) ) );
        connect( pEffectWidget,                SIGNAL( sigHide  ( bool ) ),
                 m_pPlayerWidget->m_pButtonEq, SLOT  ( setOn    ( bool ) ) );
        connect( m_pPlayerWidget,              SIGNAL( destroyed() ),
                 pEffectWidget,                SLOT  ( deleteLater() ) );
    } //</EffectConfigWidget>
                                             
    applySettings();
    m_pPlayerWidget->show(); //browserWin gets shown automatically if buttonPl isOn()

    connect( m_pMainTimer, SIGNAL( timeout() ), this, SLOT( slotMainTimer() ) );
    connect( m_pAnimTimer, SIGNAL( timeout() ), this, SLOT( slotAnimTimer() ) );
    m_pMainTimer->start( MAIN_TIMER );

    kapp->processEvents();

    restoreSession(); //do after processEvents() - sounds better
    m_pAnimTimer->start( ANIM_TIMER ); //do after restoreSession() - looks better

    KTipDialog::showTip( "amarok/data/startupTip.txt", false );
}


PlayerApp::~PlayerApp()
{
    kdDebug() << "PlayerApp:~PlayerApp()" << endl;

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

    killTimers();

    saveConfig();

    delete m_pFht;
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

void PlayerApp::initPlayerWidget()
{
    kdDebug() << "begin PlayerApp::initPlayerWidget()" << endl;

    m_pPlayerWidget = new PlayerWidget( 0, "PlayerWidget" );

    m_pPlayerWidget->m_pSliderVol->setMaxValue( VOLUME_MAX );

    //could fancy formatting be the true purpose of life?
    connect( m_pPlayerWidget->m_pSlider,         SIGNAL( sliderPressed() ),
             this,                               SLOT  ( slotSliderPressed() ) );
    connect( m_pPlayerWidget->m_pSlider,         SIGNAL( sliderReleased() ),
             this,                               SLOT  ( slotSliderReleased() ) );
    connect( m_pPlayerWidget->m_pSlider,         SIGNAL( valueChanged( int ) ),
             this,                               SLOT  ( slotSliderChanged( int ) ) );
    connect( m_pPlayerWidget->m_pSliderVol,      SIGNAL( valueChanged( int ) ),
             this,                               SLOT  ( slotVolumeChanged( int ) ) );
    connect( m_pPlayerWidget->m_pButtonPrev,     SIGNAL( clicked() ),
             this,                               SLOT  ( slotPrev() ) );
    connect( m_pPlayerWidget->m_pButtonPlay,     SIGNAL( clicked() ),
             this,                               SLOT  ( slotPlay() ) );
    connect( m_pPlayerWidget->m_pButtonPause,    SIGNAL( clicked() ),
             this,                               SLOT  ( slotPause() ) );
    connect( m_pPlayerWidget->m_pButtonStop,     SIGNAL( clicked() ),
             this,                               SLOT  ( slotStop() ) );
    connect( m_pPlayerWidget->m_pButtonNext,     SIGNAL( clicked() ),
             this,                               SLOT  ( slotNext() ) );

    kdDebug() << "end PlayerApp::initPlayerWidget()" << endl;
}


void PlayerApp::initBrowserWin()
{
    kdDebug() << "begin PlayerApp::initBrowserWin()" << endl;

    m_pBrowserWin = new BrowserWin( m_pPlayerWidget, "BrowserWin" );

    connect( m_pPlayerWidget->m_pButtonPl,       SIGNAL( toggled( bool ) ),
             m_pBrowserWin,                      SLOT  ( setShown( bool ) ) );
    
    kdDebug() << "end PlayerApp::initBrowserWin()" << endl;
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
        m_pEngine = EngineBase::createEngine( AmarokConfig::soundSystem(),
                                              m_artsNeedsRestart,
                                              SCOPE_SIZE,
                                              AmarokConfig::rememberEffects() );

        m_pEngine->setVolume( AmarokConfig::masterVolume() );

        kdDebug() << "[PlayerApp::applySettings()] AmarokConfig::soundSystem() == " << AmarokConfig::soundSystem() << endl;
    }

    if ( AmarokConfig::hardwareMixer() != m_pEngine->isMixerHardware() )
        AmarokConfig::setHardwareMixer( m_pEngine->initMixer( AmarokConfig::hardwareMixer() ) );

    m_pEngine->setRestoreEffects( AmarokConfig::rememberEffects() );
    m_pEngine->setXfadeLength( AmarokConfig::crossfade() ? AmarokConfig::crossfadeLength() : 0 );

    m_pOSD->setEnabled( AmarokConfig::osdEnabled() );
    m_pOSD->setFont   ( AmarokConfig::osdFont() );
    m_pOSD->setColor  ( AmarokConfig::osdColor() );

    m_pPlayerWidget->createVis();
    m_pBrowserWin->setFont( AmarokConfig::useCustomFonts() ?
                            AmarokConfig::browserWindowFont() : QApplication::font() );

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

    m_pEngine = EngineBase::createEngine( AmarokConfig::soundSystem(),
                                          m_artsNeedsRestart,
                                          SCOPE_SIZE,
                                          AmarokConfig::rememberEffects() );

    AmarokConfig::setHardwareMixer( m_pEngine->initMixer( AmarokConfig::hardwareMixer() ) );
    m_pEngine->setVolume( AmarokConfig::masterVolume() );
    m_pPlayerWidget->m_pSliderVol->setValue( m_pEngine->volume() );

    m_pPlayerWidget->move  ( AmarokConfig::playerPos() );
    m_pBrowserWin  ->move  ( AmarokConfig::browserWinPos() );
    m_pBrowserWin  ->resize( AmarokConfig::browserWinSize() );

    m_pPlayerWidget->m_pButtonPl->setOn( AmarokConfig::browserWinEnabled() );

    m_pPlayerWidget->slotUpdateTrayIcon( AmarokConfig::showTrayIcon() );

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
    else if( e->type() == QEvent::Hide && o == m_pPlayerWidget && !e->spontaneous() )
    {
        m_pBrowserWin->hide();
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
    bool success;

    if ( AmarokConfig::titleStreaming() && !m_proxyError && !url.isLocalFile() )
    {
        TitleProxy::Proxy *pProxy = new TitleProxy::Proxy( url );
        success = m_pEngine->open( pProxy->proxyUrl() );

        connect( m_pEngine, SIGNAL( endOfTrack  () ),
                 pProxy,    SLOT  ( deleteLater () ) );
        connect( pProxy,    SIGNAL( error       () ),
                 this,      SLOT  ( proxyError  () ) );
        connect( pProxy,    SIGNAL( metaData    ( const TitleProxy::metaPacket& ) ),
                 this,      SIGNAL( metaData    ( const TitleProxy::metaPacket& ) ) );
    }
    else
        success = m_pEngine->open( url );

    if ( !success ) {
        slotNext();
        return;
    }

    m_proxyError = false;

    m_pPlayerWidget->setScroll( bundle );
    m_pOSD->showOSD( bundle );

    m_length = bundle.length() * 1000;

    // update image tooltip
    PlaylistToolTip::add( m_pPlayerWidget->m_pFrame, bundle );

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


void PlayerApp::slotAnimTimer()
{
    //FIXME move animation timer to playerWidget

    if ( m_pPlayerWidget->isVisible() && !m_pPlayerWidget->m_pButtonPause->isDown() )
    {
        m_pPlayerWidget->drawScroll();
    }
}


void PlayerApp::slotVisTimer()
{
    //FIXME move to playerWidget

    if ( !m_pPlayerWidget->isVisible() )
        return;

    static int t = 1;

    if ( m_pEngine->state() == EngineBase::Playing )
    {
        std::vector<float> *pScopeVector = m_pEngine->scope();
        float *front = static_cast<float*>( &pScopeVector->front() );
        if (!front)
            return;

        if ( AmarokConfig::currentAnalyzer() == 2)
        { // sonogram
            m_pFht->power( front );
            m_pFht->scale( front, 1.0 / 64 );
        }
        else
        {
            float *f = new float[ m_pFht->size() ];
            m_pFht->copy( f, front );
            m_pFht->logSpectrum( front, f );
            m_pFht->scale( front, 1.0 / 20 );
            delete[] f;
        }
        pScopeVector->resize( pScopeVector->size() / 2 );

        m_pPlayerWidget->m_pVis->drawAnalyzer( pScopeVector );

        delete pScopeVector;
        //FIXME <markey> beat detection code temporarily moved to VIS_PLAN, since it was disabled anyway
    }
    else
    {
        if ( t > 999 ) t = 1; //0 = wasted calculations
        if ( t < 201 )
        {
            double dt = double(t) / 200 ;
            std::vector<float> v( 31 );
            for( uint i = 0; i < v.size(); ++i )
                v[i] = dt * (sin( M_PI + (i * M_PI) / v.size() ) + 1.0);
            m_pPlayerWidget->m_pVis->drawAnalyzer( &v );
        }
        else
            m_pPlayerWidget->m_pVis->drawAnalyzer( NULL );

        ++t;
    }
}


void PlayerApp::slotShowOptions()
{
    KConfigDialog::showDialog( "settings" );
}


#include "playerapp.moc"
