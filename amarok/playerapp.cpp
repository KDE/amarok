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
#include <kmessagebox.h>
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
        , m_pEffectWidget( NULL )
        , m_pFht( new FHT( SCOPE_SIZE ) )
        , m_pOSD( new OSDWidget() )
{
    setName( "amaroK" );
    pApp = this; //global

    initPlayerWidget();
    initBrowserWin();

    //we monitor for close, hide and show events
    m_pPlayerWidget->installEventFilter( this );
    m_pBrowserWin  ->installEventFilter( this );
    m_pPlayerWidget->show(); //browserWin gets shown automatically if buttonPl isOn()

    readConfig();

    //TODO can we create this on demand only? it would save a whole bunch of memory
    KConfigDialog *dialog = new AmarokConfigDialog( m_pPlayerWidget, "settings", AmarokConfig::self() );
    connect( dialog, SIGNAL( settingsChanged() ), this, SLOT( applySettings() ) );

    applySettings();

    connect( m_pMainTimer, SIGNAL( timeout() ), this, SLOT( slotMainTimer() ) );
    connect( m_pAnimTimer, SIGNAL( timeout() ), this, SLOT( slotAnimTimer() ) );
    m_pMainTimer->start( MAIN_TIMER );

    restoreSession();
    m_pAnimTimer->start( ANIM_TIMER ); //do after restoreSession() - looks better

    kapp->processEvents();

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

    delete m_pEffectWidget;
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
    connect( m_pPlayerWidget->m_pButtonPl,       SIGNAL( toggled( bool ) ),
             this,                               SLOT  ( slotPlaylistToggle( bool ) ) );
    connect( m_pPlayerWidget->m_pButtonEq,       SIGNAL( clicked() ),
             this,                               SLOT  ( slotConfigEffects() ) );
    connect( m_pPlayerWidget,                    SIGNAL( sigAboutToHide() ),
             this,                               SLOT  ( slotHide() ) );
    connect( m_pPlayerWidget,                    SIGNAL( sigAboutToShow() ),
             this,                               SLOT  ( slotShow() ) );

    kdDebug() << "end PlayerApp::initPlayerWidget()" << endl;
}


void PlayerApp::initBrowserWin()
{
    kdDebug() << "begin PlayerApp::initBrowserWin()" << endl;

    m_pBrowserWin = new BrowserWin( m_pPlayerWidget, "BrowserWin" );

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
            //TODO make a static syncronous readTags function
            play( AmarokConfig::resumeTrack(), MetaBundle() );

            if ( seconds > 0 )
                m_pEngine->seek( seconds * 1000 );
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
    return m_pEngine->loaded();
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
    m_pPlayerWidget->m_pSliderVol->setValue( VOLUME_MAX - AmarokConfig::masterVolume() );

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

    // FIXME <berkus> this needs some other way of handling with KConfig XT?!?
    //<mxcl> doesn't need to be XT'd as it's not for configDialogs and doesn't need global access
    m_pGlobalAccel->setConfigGroup( "Shortcuts" );
    m_pGlobalAccel->readSettings( kapp->config() );
    m_pGlobalAccel->updateConnections();

    //FIXME use a global actionCollection (perhaps even at global scope)
    m_pPlayerWidget->m_pActionCollection->readShortcutSettings( QString::null, kapp->config() );
    m_pBrowserWin->m_pActionCollection->readShortcutSettings( QString::null, kapp->config() );

    kdDebug() << "end PlayerApp::readConfig()" << endl;
}


void PlayerApp::receiveStreamMeta( QString title, QString url, QString kbps )
{
    //FIXME this could all be compressed into a single setScroll() if the bitrate is used in the else case
    if ( url.isEmpty() )
    {
        QString text = m_playingURL.prettyURL();
        if ( text.isEmpty() ) text = i18n( "stream" );

        m_pPlayerWidget->setScroll( QString( "%1 (%2)" ).arg( title ).arg( text ), kbps + "kbps", "--" );
    }
    else
        //FIXME show bitrate? this was how it was before so I am leaving it as is.. <mxcl>
        m_pPlayerWidget->setScroll( QString( "%1 (%2)" ).arg( title ).arg( url ), "--", "--" );
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
    if( o == m_pBrowserWin && e->type() == QEvent::Close )
    {
        m_pPlayerWidget->m_pButtonPl->setOn( false );
    }
    else if( o == m_pPlayerWidget && e->type() == QEvent::Hide )
    {
        m_pBrowserWin->hide();
    }
    else if( o == m_pPlayerWidget && e->type() == QEvent::Show )
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


void PlayerApp::play( const KURL &url, const MetaBundle &tags )
{
    m_pEngine->open( url );

    connect( m_pEngine, SIGNAL( metaData         ( QString, QString, QString ) ),
             this,      SLOT  ( receiveStreamMeta( QString, QString, QString ) ) );

    if( !(tags.m_length == 0 && tags.m_bitrate == 0 && tags.m_sampleRate == 0) )
    {
        m_length = tags.m_length * 1000;      // sec -> ms
        QString text, bps, Hz, length;

        if( tags.m_title.isEmpty() )
        {
            text = url.fileName();
        }
        else
        {
            //TODO <berkus> user tunable title format!
            text = tags.m_artist;
            if( text != "" ) text += " - ";
            text += tags.m_title;
        }

        bps  = QString::number( tags.m_bitrate );
        bps += "kbps";
        Hz   = QString::number( tags.m_sampleRate );
        Hz  += "Hz";

        // length to string
        if ( floor( tags.m_length / 60 ) < 10 ) length += "0";
        length += QString::number( floor( tags.m_length / 60 ) );
        length += ":";
        if ( tags.m_length % 60 < 10 ) length += "0";
        length += QString::number( tags.m_length % 60 );

        m_pPlayerWidget->setScroll( text, bps, Hz, length ); //FIXME get end function to add units!

        // OSD message
        m_pOSD->showOSD( text + " - " + length );
    }
    else
    {
        m_length = 0;

        if ( m_pEngine->isStream() )
            m_pPlayerWidget->setScroll( i18n( "Stream from: %1" ).arg( url.prettyURL() ), "?", "--" );
        else
            m_pPlayerWidget->setScroll( url.fileName() );
    }

    kdDebug() << "[play()] Playing " << url.prettyURL() << endl;
    m_pEngine->play();

    m_playingURL = url;

    m_pPlayerWidget->m_pSlider->setValue    ( 0 );
    m_pPlayerWidget->m_pSlider->setMaxValue ( m_length );

    //interface consistency
    m_pPlayerWidget->m_pButtonPlay ->setOn  ( true );
    m_pPlayerWidget->m_pButtonPause->setDown( false );

    emit currentTrack( url );
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
    m_pPlayerWidget->setScroll              ();
    m_pPlayerWidget->timeDisplay            ( false, 0, 0, 0 );
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

        if ( AmarokConfig::timeDisplayRemaining() )
        {
            value = m_length / 1000 - value;
            m_pPlayerWidget->timeDisplay( true, value / 60 / 60 % 60, value / 60 % 60, value % 60 );
        }
        else
        {
            m_pPlayerWidget->timeDisplay( false, value / 60 / 60 % 60, value / 60 % 60, value % 60 );
        }
    }
}


void PlayerApp::slotVolumeChanged( int value )
{
    AmarokConfig::setMasterVolume( value );
    value = VOLUME_MAX - value;

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
        int seconds;
        if ( AmarokConfig::timeDisplayRemaining() && !m_pEngine->isStream() )
        {
            seconds = ( m_length - m_pEngine->position() ) / 1000;
            m_pPlayerWidget->timeDisplay( true, seconds / 60 / 60 % 60, seconds / 60 % 60, seconds % 60 );
        }
        else
        {
            seconds = m_pEngine->position() / 1000;
            m_pPlayerWidget->timeDisplay( false, seconds / 60 / 60 % 60, seconds / 60 % 60, seconds % 60 );
        }
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

    static int t = 1;

    if ( m_pPlayerWidget->isVisible() && !m_pPlayerWidget->m_pButtonPause->isDown() )
    {
        std::vector<float> *pScopeVector = m_pEngine->scope();
        float *front = static_cast<float*>( &pScopeVector->front() );
        if (!front)
            return;

        if ( AmarokConfig::currentAnalyzer() == 6)
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


void PlayerApp::slotPlaylistToggle( bool b )
{
    if( b ) m_pBrowserWin->show();
    else m_pBrowserWin->hide();
}


void PlayerApp::slotEq( bool b )
{
    //FIXME this is no longer needed?

    if ( b )
    {
        KMessageBox::sorry( 0, i18n( "Equalizer is not yet implemented." ) );
        m_pPlayerWidget->m_pButtonEq->setOn( false );
    }
}


void PlayerApp::slotConfigEffects()
{
    // we never destroy the EffectWidget, just hide it, since destroying would delete the EffectListItems
    if ( m_pEffectWidget == NULL )
        m_pEffectWidget = new EffectWidget();

    m_pEffectWidget->show();
}


void PlayerApp::slotShowOptions()
{
    KConfigDialog::showDialog( "settings" );
}


#include "playerapp.moc"
