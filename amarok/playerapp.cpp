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
#include "amarokslider.h"
#include "analyzers/analyzerbase.h"
#include "browserwin.h"
#include "effectwidget.h"
#include "engine/enginebase.h"
#include "fht/fht.cpp"
#include "metabundle.h" //play( const KURL& )
#include "osd.h"
#include "playerapp.h"
#include "playerwidget.h"

#include "amarokconfig.h"
#include "Options1.h"
#include "Options2.h"
#include "Options3.h"
#include "Options4.h"

#include <vector>
#include <string>

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

//FIXME remove these dependencies, we can implement saveConfig across objects and use a save() signal
//      a little less neat, but boy would that help with compile times
#include "playlistwidget.h"

#include <qdir.h>
#include <qpoint.h>
#include <qsize.h>
#include <qstring.h>
#include <qtimer.h>
#include <qvaluelist.h>
#include <qpushbutton.h> //initPlayerWidget()

PlayerApp::PlayerApp()
        : KUniqueApplication( true, true, false )
        , m_pGlobalAccel( new KGlobalAccel( this ) )
        , m_bgColor( Qt::black )
        , m_fgColor( QColor( 0x80, 0xa0, 0xff ) )
        , m_DelayTime( 0 )
        , m_playingURL( KURL() )
        , m_pConfig( kapp->config() )
        , m_pMainTimer( new QTimer( this ) )
        , m_pAnimTimer( new QTimer( this ) )
        , m_length( 0 )
        , m_playRetryCounter( 0 )
        , m_pEffectWidget( NULL )
        , m_bChangingSlider( false )
        , m_pFht( new FHT( SCOPE_SIZE ) )
{
    setName( "amaroK" );
    pApp = this; //global
    
    initOSD();
    initPlayerWidget();
    initBrowserWin();

    readConfig();
    
    m_pEngine = EngineBase::createEngine( config()->soundSystem(), m_artsNeedsRestart, SCOPE_SIZE );
    m_pEngine ->initMixer( config()->softwareMixerOnly() );

    connect( m_pMainTimer, SIGNAL( timeout() ), this, SLOT( slotMainTimer() ) );
    connect( m_pAnimTimer, SIGNAL( timeout() ), this, SLOT( slotAnimTimer() ) );
    m_pMainTimer->start( MAIN_TIMER );

    m_pPlayerWidget->show(); //browserwin will be shown automatically if the playlistButton is setOn( true )

    kapp->processEvents();

    //restore last playlist
    //<mxcl> At some point it'd be nice to start loading of the playlist before we initialise arts so
    //       the playlist seems to be loaded when the browserWindow appears
    //TODO   now it's threaded, it's more feasable to load it early
    m_pBrowserWin->m_pPlaylistWidget->insertMedia( kapp->dirs()->saveLocation
                                                 ( "data", kapp->instanceName() + "/" ) + "current.m3u" );

    restoreSession();

    m_pAnimTimer->start( ANIM_TIMER ); //do it after restoreSession() avoid some visual nastys

    KTipDialog::showTip( "amarok/data/startupTip.txt", false );
}


PlayerApp::~PlayerApp()
{
    kdDebug() << "PlayerApp:~PlayerApp()" << endl;

    //Save current item info in dtor rather than saveConfig() as it is only relevant on exit
    //and we may in the future start to use read and saveConfig() in other situations
//    m_pConfig->setGroup( "Session" );
    KURL url( m_pEngine->loaded() ?  m_playingURL : m_pBrowserWin->m_pPlaylistWidget->currentTrackURL() );

    if ( !url.isEmpty() )
    {
       config()->setResumeTrack( url.url() );
       
       if ( m_pEngine->state() != EngineBase::Empty )
           config()->setResumeTime( m_pEngine->position() / 1000 );
       else
           config()->setResumeTime( -1 );
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
        m_pBrowserWin->m_pPlaylistWidget->insertMedia( KURL::List( KCmdLineArgs::makeURL( playlistUrl ) ), true );
    }

    if ( args->count() > 0 )
    {
       KURL::List list;

       for ( int i = 0; i < args->count(); i++ )
       {
          list << args->url( i );
       }

       bool b = !args->isSet( "e" ); //b = (not enqueue?)

       m_pBrowserWin->m_pPlaylistWidget->insertMedia( list, b );

       if ( b )
       {
          //FIXME why specify the play flag if we aren't going to be strict?
          slotPlay();
       }
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


// INIT -------------------------------------------------------------------------

void PlayerApp::initOSD()
{
    kdDebug() << "begin PlayerApp::initOSD()" << endl;

    // set font    
    QFont font( "Impact" );
    font.setBold( FALSE );
    font.setPixelSize( 28 );

    // create osd widget    
    m_pOSD = new OSDWidget();
    m_pOSD->setEnabled( TRUE );
    m_pOSD->setFont( font );
    m_pOSD->setColor( QColor( "yellow" )  );
    
    kdDebug() << "end PlayerApp::initOSD()" << endl;
}


void PlayerApp::initPlayerWidget()
{
    kdDebug() << "begin PlayerApp::initPlayerWidget()" << endl;

    m_pPlayerWidget = new PlayerWidget( 0, "PlayerWidget" );
    //    setCentralWidget(m_pPlayerWidget);

    m_bSliderIsPressed = false;

    m_pPlayerWidget->m_pSlider->setMinValue( 0 );
    m_pPlayerWidget->m_pSlider->setMaxValue( 0 );
    m_pPlayerWidget->m_pSlider->setValue( 0 );

    m_pPlayerWidget->m_pSliderVol->setMinValue( 0 );
    m_pPlayerWidget->m_pSliderVol->setMaxValue( VOLUME_MAX );

    connect( m_pPlayerWidget->m_pSlider, SIGNAL( sliderPressed() ),
             this, SLOT( slotSliderPressed() ) );

    connect( m_pPlayerWidget->m_pSlider, SIGNAL( sliderReleased() ),
             this, SLOT( slotSliderReleased() ) );

    connect( m_pPlayerWidget->m_pSlider, SIGNAL( valueChanged( int ) ),
             this, SLOT( slotSliderChanged( int ) ) );

    connect( m_pPlayerWidget->m_pSliderVol, SIGNAL( valueChanged( int ) ),
             this, SLOT( slotVolumeChanged( int ) ) );

    connect( m_pPlayerWidget->m_pButtonPrev, SIGNAL( clicked() ),
             this, SLOT( slotPrev() ) );

    connect( m_pPlayerWidget->m_pButtonPlay, SIGNAL( clicked() ),
             this, SLOT( slotPlay() ) );

    connect( m_pPlayerWidget->m_pButtonPause, SIGNAL( clicked() ),
             this, SLOT( slotPause() ) );

    connect( m_pPlayerWidget->m_pButtonStop, SIGNAL( clicked() ),
             this, SLOT( slotStop() ) );

    connect( m_pPlayerWidget->m_pButtonNext, SIGNAL( clicked() ),
             this, SLOT( slotNext() ) );

    connect( m_pPlayerWidget->m_pButtonPl, SIGNAL( toggled( bool ) ),
             this, SLOT( slotPlaylistToggle( bool ) ) );

    connect( m_pPlayerWidget->m_pButtonEq, SIGNAL( clicked() ),
             this, SLOT( slotConfigEffects() ) );

    connect( m_pPlayerWidget, SIGNAL( sigAboutToHide() ), this, SLOT( slotHide() ) );
    connect( m_pPlayerWidget, SIGNAL( sigAboutToShow() ), this, SLOT( slotShow() ) );

    kdDebug() << "end PlayerApp::initPlayerWidget()" << endl;
}


void PlayerApp::initBrowserWin()
{
    kdDebug() << "begin PlayerApp::initBrowserWin()" << endl;

    m_pBrowserWin = new BrowserWin( m_pPlayerWidget, "BrowserWin" );


    connect( (QPushButton *)m_pBrowserWin->m_pButtonPlay, SIGNAL( clicked() ),
             this, SLOT( slotPlay() ) );

    connect( (QPushButton *)m_pBrowserWin->m_pButtonPause, SIGNAL( clicked() ),
             this, SLOT( slotPause() ) );

    connect( (QPushButton *)m_pBrowserWin->m_pButtonStop, SIGNAL( clicked() ),
             this, SLOT( slotStop() ) );

    connect( (QPushButton *)m_pBrowserWin->m_pButtonNext, SIGNAL( clicked() ),
             this, SLOT( slotNext() ) );

    connect( (QPushButton *)m_pBrowserWin->m_pButtonPrev, SIGNAL( clicked() ),
             this, SLOT( slotPrev() ) );

    connect( m_pBrowserWin, SIGNAL( signalHide() ),
             this, SLOT( slotPlaylistIsHidden() ) );

    //make sure playlist is linked to playback
    connect( m_pBrowserWin->m_pPlaylistWidget, SIGNAL( activated( const KURL&, const MetaBundle* ) ),
             this, SLOT( play( const KURL&, const MetaBundle* ) ) );

    kdDebug() << "end PlayerApp::initBrowserWin()" << endl;
}


void PlayerApp::restoreSession()
{
   //here we restore the session
   //however, do note, this is always done, KDE session management is not involved

   if ( config()->resumePlayback() )
   {
      //see if we also saved the time
      int seconds = config()->resumeTime();

      if ( seconds >= 0 )
      {
         play( config()->resumeTrack() );

         if ( seconds > 0 )
            m_pEngine->seek( seconds * 1000 );
      }
   }
}


// METHODS --------------------------------------------------------------------------

#include <klineedit.h>     //browserWin
#include <kcombobox.h>     //browserWin::KComboHistory (file chooser lineEdit)
#include "browserwidget.h" //anoyingly necessary //FIXME KConfig XT!
#include <kdirlister.h>    //for browserwin component also


AmarokConfig *PlayerApp::config()
{
    return AmarokConfig::self();
}


void PlayerApp::saveConfig()
{
    config()->setMasterVolume( m_Volume );
    config()->setCurrentDirectory( m_pBrowserWin->m_pBrowserWidget->m_pDirLister->url().path() );
    config()->setPathHistory( m_pBrowserWin->m_pBrowserLineEdit->historyItems() );
    config()->setPlayerPos( m_pPlayerWidget->pos() );
    config()->setBrowserWinPos( m_pBrowserWin->pos() );
    config()->setBrowserWinSize( m_pBrowserWin->size() );
    config()->setBrowserWinSplitter( m_pBrowserWin->m_pSplitter->sizes() );
    config()->setBrowserWinEnabled( m_pPlayerWidget->m_pButtonPl->isOn() );
    
    config()->writeConfig();
    
    if (config()->savePlaylist())
       m_pBrowserWin->m_pPlaylistWidget->saveM3u( kapp->dirs()->saveLocation(
          "data", kapp->instanceName() + "/" ) + "current.m3u" );
}


void PlayerApp::readConfig()
{
    kdDebug() << "begin PlayerApp::readConfig()" << endl;

    //we restart artsd after each version change, so that it picks up any plugin changes
    m_artsNeedsRestart = config()->version() != APP_VERSION;
    
    m_pBrowserWin->m_pBrowserWidget->readDir( config()->currentDirectory() );
    m_pBrowserWin->m_pBrowserLineEdit->setHistoryItems( config()->pathHistory() );
    
    m_pPlayerWidget->move( config()->playerPos() );
    m_pBrowserWin->move( config()->browserWinPos() );
    m_pBrowserWin->resize( config()->browserWinSize() );

    m_pPlayerWidget->m_pButtonPl->setOn( config()->browserWinEnabled() );

    m_pBrowserWin->slotUpdateFonts();
    
    m_Volume = config()->masterVolume();

    setupColors();

    m_pPlayerWidget->createVis();

    QValueList<int> splitterList;
    splitterList = config()->browserWinSplitter();
     if ( splitterList.count() != 4 )
    {
        splitterList.clear();
        splitterList.append( 70 );
        splitterList.append( 140 );
        splitterList.append( 140 );
        splitterList.append( 340 );
    }
    m_pBrowserWin->m_pSplitter->setSizes( splitterList );

    m_pPlayerWidget->slotUpdateTrayIcon( config()->showTrayIcon() );

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
    m_pGlobalAccel->setConfigGroup( "Shortcuts" );
    m_pGlobalAccel->readSettings( m_pConfig );
    m_pGlobalAccel->updateConnections();

    //FIXME use a global actionCollection (perhaps even at global scope)
    m_pPlayerWidget->m_pActionCollection->readShortcutSettings( QString::null, m_pConfig );
    m_pBrowserWin->m_pActionCollection->readShortcutSettings( QString::null, m_pConfig );

    kdDebug() << "end PlayerApp::readConfig()" << endl;
}


bool PlayerApp::queryClose()
{
    if ( config()->confirmExit() )
        if ( KMessageBox::questionYesNo( 0, i18n( "Really exit the program?" ) ) == KMessageBox::No )
            return false;

    return true;
}


void PlayerApp::receiveStreamMeta( QString title, QString url, QString kbps )
{
    //FIXME this could all be compressed into a single setScroll() if the bitrate is used in the else case
    if ( url.isEmpty() )
    {
        QString text = m_pBrowserWin->m_pPlaylistWidget->currentTrackName();
        if ( text.isEmpty() ) text = i18n( "stream" );

        m_pPlayerWidget->setScroll( QString( "%1 (%2)" ).arg( title ).arg( text ), kbps + "kbps", "--" );
    }
    else
        //FIXME show bitrate? this was how it was before so I am leaving it as is.. <mxcl>
        m_pPlayerWidget->setScroll( QString( "%1 (%2)" ).arg( title ).arg( url ), "--", "--" );
}


void PlayerApp::setupColors()
{
    // we try to be smart: this code figures out contrasting colors for selection and alternate background rows
    int h, s, v;

    config()->browserBgColor().hsv( &h, &s, &v );
    if ( v < 128 )
        v += 50;
    else
        v -= 50;
    m_optBrowserBgAltColor.setHsv( h, s, v );

    config()->browserFgColor().hsv( &h, &s, &v );
    if ( v < 128 )
        v += 150;
    else
        v -= 150;
    if ( v < 0 )
        v = 0;
    if ( v > 255 )
        v = 255;
    m_optBrowserSelColor.setHsv( h, s, v );

    m_pBrowserWin->setPalettes( config()->browserFgColor(), config()->browserBgColor(), m_optBrowserBgAltColor );
}


// SLOTS -----------------------------------------------------------------

void PlayerApp::slotPrev() const { m_pBrowserWin->m_pPlaylistWidget->request( PlaylistWidget::Prev, m_pEngine->loaded() ); }
void PlayerApp::slotNext() const { m_pBrowserWin->m_pPlaylistWidget->request( PlaylistWidget::Next, m_pEngine->loaded() ); }
void PlayerApp::slotPlay() const { m_pBrowserWin->m_pPlaylistWidget->request( PlaylistWidget::Current ); }


#include <math.h> //FIXME: I put it here so we remember it's only used by this function and the one somewhere below

void PlayerApp::play( const KURL &url, const MetaBundle *tags )
{
    m_pEngine->open( url );
    
    connect( m_pEngine, SIGNAL( metaData         ( QString, QString, QString ) ),
             this,      SLOT  ( receiveStreamMeta( QString, QString, QString ) ) );
    
    if ( tags )
    {
        m_length = tags->m_length * 1000;      // sec -> ms
        QString text, bps, Hz, length;
        
        if( tags->m_title.isEmpty() )
        {
            text = url.fileName();
        }
        else
        {
            //TODO <berkus> user tunable title format!
            text = tags->m_artist;
            if( text != "" ) text += " - ";
            text += tags->m_title;
        }
        
        //FIXME add this back
        //text.append( " (" + tags->length() + ")" );
        
        bps  = QString::number( tags->m_bitrate );
        bps += "kbps";
        Hz   = QString::number( tags->m_sampleRate );
        Hz  += "Hz";
        
        // length to string
        if ( floor( tags->m_length / 60 ) < 10 ) length += "0";
        length += QString::number( floor( tags->m_length / 60 ) );
        length += ":";
        if ( tags->m_length % 60 < 10 ) length += "0";
        length += QString::number( tags->m_length % 60 );
        
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

    m_pPlayerWidget->m_pSlider->setValue   ( 0 );
    m_pPlayerWidget->m_pSlider->setMinValue( 0 );
    m_pPlayerWidget->m_pSlider->setMaxValue( m_length );

    //interface consistency
    m_pPlayerWidget->m_pButtonPlay ->setOn  ( true );
    m_pPlayerWidget->m_pButtonPause->setDown( false );
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

    m_pPlayerWidget->m_pButtonPlay->setOn( false );
    m_pPlayerWidget->m_pButtonPause->setDown( false );
    m_pPlayerWidget->setScroll();
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
    m_bSliderIsPressed = true;
}


void PlayerApp::slotSliderReleased()
{
    if ( m_pEngine->state() == EngineBase::Playing )
    {
        m_pEngine->seek( m_pPlayerWidget->m_pSlider->value() );
    }
        
    m_bSliderIsPressed = false;
}


void PlayerApp::slotSliderChanged( int value )
{
    if ( m_bSliderIsPressed )
    {
        value /= 1000;    // ms -> sec
        
        if ( config()->timeDisplayRemaining() )
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
    m_Volume = value;
    value = VOLUME_MAX - value;

    m_pEngine->setVolume( value );
   
}


void PlayerApp::slotMainTimer()
{
    if ( m_bSliderIsPressed || ( m_pEngine->state() == EngineBase::Empty ) )
        return;

    m_pPlayerWidget->m_pSlider->setValue( m_pEngine->position() );
    
    // <Draw TimeDisplay>
    if ( m_pPlayerWidget->isVisible() )
    {
        int seconds;
        if ( config()->timeDisplayRemaining() && !m_pEngine->isStream() )
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

    // check if track has ended
    if ( m_pEngine->state() == EngineBase::Idle )
    {
        if( !m_pBrowserWin->m_pPlaylistWidget->request( PlaylistWidget::Next, true ) )
        {
            slotStop();
        }
    }
}


void PlayerApp::slotAnimTimer()
{
    if ( m_pPlayerWidget->isVisible() && !m_pPlayerWidget->m_pButtonPause->isDown() )
    {
        m_pPlayerWidget->drawScroll();
    }
}


void PlayerApp::slotVisTimer()
{
    static int t = 1;

    if ( m_pPlayerWidget->isVisible() && !m_pPlayerWidget->m_pButtonPause->isDown() )
    {
        if ( true )    // FIXME
//         if ( m_scopeId )
        {
            std::vector<float> *pScopeVector = m_pEngine->scope();
            float *front = static_cast<float*>( &pScopeVector->front() );
            m_pFht->power( front );
            m_pFht->scale( front, 1.0 / 64 );
            pScopeVector->resize( pScopeVector->size() / 2 );
                        
            m_pPlayerWidget->m_pVis->drawAnalyzer( pScopeVector );

/*
            // Muesli's Beat Detection - A Night's Oddysee
            // shift old elements
            for ( uint x = 0; x < 18; ++x )
                for ( uint y = 42; y > 0; --y ) m_beatEnergy[x][y] = m_beatEnergy[x][y - 1];

            // get current energy values
            for ( uint x = 0; x < pScopeVector->size(); ++x ) m_beatEnergy[x][0] = pScopeVector->at(x);

            // compare to old elements and get averages
            double beatAvg[18];
//            double beatVariance[18];
//            double beatMood[18];
            
            for ( uint x = 0; x < 18; ++x )
            {
                beatAvg[x] = 0;
                for ( uint y = 1; y < 44; ++y )  beatAvg[x] += m_beatEnergy[x][y];
                
                beatAvg[x] = beatAvg[x] / 43;
            }
*/                
/*            for ( uint x = 0; x < 18; ++x )
            {
                beatVariance[x] = 0;
                for ( uint y = 0; y < 42; ++y )  beatVariance[x] += (pow((m_beatEnergy[x][y] - beatAvg[x]), 2) / 43);
            }
                
            for ( uint x = 0; x < 18; ++x )
                beatMood[x] = (-0.0025714 * beatVariance[x]) + 1.5142857;
*/

            // do we have a beat? let's dance!
/*            int total_hits = 0;
            for ( uint x = 0; x < 18; ++x )
            {
                double factor = cos( x * 4 ) * 18;
                factor = beatAvg[x] * factor;
                
                if ( m_beatEnergy[x][0] > factor )
                {
                    total_hits++;
                    kdDebug() << "*CLAP* factor: " << factor << " - x: " << x << " - average energy: " << beatAvg[x] << " - current peak: " << m_beatEnergy[x][0] << endl;
                }
            }

            if ( total_hits > 3 ) kdDebug() << "***CLAPCLAPCLAP***" << endl;
*/
            delete pScopeVector;
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
}

// FIXME <berkus> unify this and the one below
void PlayerApp::slotPlaylistShowHide()
{
    if ( m_pBrowserWin->isHidden() )
    {
        m_pPlayerWidget->m_pButtonPl->setOn(true);
        m_pBrowserWin->show();
    }
    else
    {
        m_pPlayerWidget->m_pButtonPl->setOn(false);
        m_pBrowserWin->hide();
    }
}


void PlayerApp::slotPlaylistToggle( bool b )
{
    if ( b )
    {
        m_pBrowserWin->show();
    }
    else
    {
        m_pBrowserWin->hide();
    }
}


void PlayerApp::slotPlaylistIsHidden()
{
    //only called via playlist::closeEvent() - NOT hideEvent()

    m_pPlayerWidget->m_pButtonPl->setOn( false );
}


void PlayerApp::slotEq( bool b )
{
    if ( b )
    {
        KMessageBox::sorry( 0, i18n( "Equalizer is not yet implemented." ) );
        m_pPlayerWidget->m_pButtonEq->setOn( false );
    }
}


void PlayerApp::slotShowOptions()
{
    if( KConfigDialog::showDialog("settings") )
        return;
    
    KConfigDialog *dialog = new KConfigDialog( m_pPlayerWidget, "settings", AmarokConfig::self() );
    
    dialog->addPage( new Options1(0,"General"),  i18n("General"),  "misc",   i18n("Configure general options") );
    dialog->addPage( new Options2(0,"Fonts"),    i18n("Fonts"),    "fonts",  i18n("Configure fonts") );
    dialog->addPage( new Options3(0,"Colors"),   i18n("Colors"),   "colors", i18n("Configure Colors") );
    dialog->addPage( new Options4(0,"Playback"), i18n("Playback"), "kmix",   i18n("Configure playback") );
    
    connect( dialog, SIGNAL( settingsChanged() ), this, SLOT( readConfig() ) );
    
    dialog->setInitialSize( QSize( 480, 430 ) );        
    dialog->show();
}


void PlayerApp::slotConfigEffects()
{
    // we never destroy the EffectWidget, just hide it, since destroying would delete the EffectListItems
    if ( m_pEffectWidget == NULL )
        m_pEffectWidget = new EffectWidget();

    m_pEffectWidget->show();
}


void PlayerApp::slotHide()
{
//FIXME: as browserWin is now a child widget of playerWidget, it should, technically hide browserWin
//       for us when we hide playerWidget, find out why it doesn't! We shouldn't have to map out this
//       functionality!

//But conveniently this allows us to keep the hidePlaylistWindowWithMainWidget option
//But I think this option should be removed and amaroK's behavior should be to hide everything

// <berkus> imo the reason it doesn't hide is that we pass some wrong flags upong creation of browserwin
// (maybe toplevel is redundant or something)

    if ( config()->hidePlaylistWindow() )
       m_pBrowserWin->hide();
}


void PlayerApp::slotShow()
{
    if ( m_pPlayerWidget->m_pButtonPl->isOn() )
        m_pBrowserWin->show();
}


#include "playerapp.moc"
