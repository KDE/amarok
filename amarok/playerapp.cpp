/***************************************************************************
                      playerapp.cpp  -  description
                         -------------------
begin                : Mit Okt 23 14:35:18 CEST 2002
copyright            : (C) 2002 by Mark Kretschmann
email                :
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
#include "configdlg.h"
#include "effectwidget.h"
#include "playerapp.h"
#include "playerwidget.h"
#include "titleproxy/titleproxy.h"
#include "metabundle.h" //play( const KURL& )

#include <vector>
#include <string>

#include <kaction.h>
#include <kapp.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobalaccel.h>
#include <klocale.h>
#include <kmessagebox.h> //initArts()
#include <kshortcut.h>
#include <kstandarddirs.h> //ctor
#include <ktip.h>
#include <kuniqueapplication.h>
#include <kurl.h>


//FIXME remove these dependencies, we can implement saveConfig across objects and use a save() signal
//      a little less neat, but boy would that help with compile times
#include "playlistwidget.h"


#include <arts/artsflow.h>
#include <arts/artskde.h>
#include <arts/artsmodules.h>
#include <arts/connect.h>
#include <arts/dynamicrequest.h>
#include <arts/flowsystem.h>
#include <arts/kartsdispatcher.h>
#include <arts/kmedia2.h>
#include <arts/kplayobjectfactory.h>
#include <arts/soundserver.h>

#include <qdir.h>
#include <qpoint.h>
#include <qsize.h>
#include <qstring.h>
#include <qtimer.h>
#include <qvaluelist.h>
#include <qpushbutton.h> //initPlayerWidget()

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/wait.h>

#define MAIN_TIMER 150
#define ANIM_TIMER 30


PlayerApp::PlayerApp()
        : KUniqueApplication( true, true, false )
        , m_pGlobalAccel( new KGlobalAccel( this ) )
        , m_bgColor( Qt::black )
        , m_fgColor( QColor( 0x80, 0xa0, 0xff ) )
        , m_DelayTime( 0 )
        , m_playingURL( KURL() )
        , m_pPlayObject( NULL )
        , m_pPlayObjectXFade( NULL )
        , m_pArtsDispatcher( NULL )
        , m_pConfig( kapp->config() )
        , m_pMainTimer( new QTimer( this ) )
        , m_pAnimTimer( new QTimer( this ) )
        , m_scopeId( 0 )
        , m_length( 0 )
        , m_playRetryCounter( 0 )
        , m_pEffectWidget( NULL )
        , m_bIsPlaying( false )
        , m_bChangingSlider( false )
        , m_proxyError( false )
        , m_XFadeRunning( false )
        , m_XFadeValue( 1.0 )
        , m_XFadeCurrent( "invalue1" )
{
    setName( "amaroK" );
    pApp = this; //global

    initArts();
    initPlayerWidget();
    initBrowserWin();

    readConfig();
    initMixer();          //initMixer() depends on a config value, so it must be executed after readConfig()

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
    m_pConfig->setGroup( "Session" );
    KURL url( m_bIsPlaying ? m_playingURL : m_pBrowserWin->m_pPlaylistWidget->currentTrackURL() );

    if ( !url.isEmpty() )
    {
       m_pConfig->writeEntry( "Track", url.url() );
       if ( m_bIsPlaying )
       {
          Arts::poTime timeC( m_pPlayObject->currentTime() );
          m_pConfig->writeEntry( "Time", timeC.seconds );
       }
       else m_pConfig->deleteEntry( "Time" );
    }

    slotStop();

    killTimers();

    saveConfig();

    delete m_pEffectWidget;
    delete m_pPlayerWidget; //is parent of browserWin (and thus deletes it)

    m_XFade             = Amarok::Synth_STEREO_XFADE::null();
    m_scope             = Arts::StereoFFTScope::null();
    m_volumeControl     = Arts::StereoVolumeControl::null();
    m_effectStack       = Arts::StereoEffectStack::null();
    m_globalEffectStack = Arts::StereoEffectStack::null();
    m_amanPlay          = Arts::Synth_AMAN_PLAY::null();
    m_Server            = Arts::SoundServerV2::null();

    delete m_pArtsDispatcher;
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

void PlayerApp::initArts()
{
    // We must restart artsd after first installation, because we install new mcoptypes

    m_pConfig->setGroup( "" );

    if ( m_pConfig->readEntry( "Version" ) != APP_VERSION )
    {
        QCString kill_cmdline;
        kill_cmdline = "killall artsd";

        int kill_status = ::system( kill_cmdline );
        if ( kill_status != -1 && WIFEXITED( kill_status ) )
        {
            kdDebug() << "killall artsd succeeded." << endl;
        }
    }
    m_pArtsDispatcher = new KArtsDispatcher();

    // <most of this code was taken from noatun's engine.cpp>
    m_Server = Arts::Reference( "global:Arts_SoundServerV2" );
    if ( m_Server.isNull() || m_Server.error() )
    {
        kdDebug() << "aRtsd not running.. trying to start" << endl;
        // aRts seems not to be running, let's try to run it
        // First, let's read the configuration as in kcmarts
        KConfig config( "kcmartsrc" );
        QCString cmdline;

        config.setGroup( "Arts" );

        bool rt = config.readBoolEntry( "StartRealtime", false );
        bool x11Comm = config.readBoolEntry( "X11GlobalComm", false );

        // put the value of x11Comm into .mcoprc
        {
            KConfig X11CommConfig( QDir::homeDirPath() + "/.mcoprc" );

            if ( x11Comm )
                X11CommConfig.writeEntry( "GlobalComm", "Arts::X11GlobalComm" );
            else
                X11CommConfig.writeEntry( "GlobalComm", "Arts::TmpGlobalComm" );

            X11CommConfig.sync();
        }

        cmdline = QFile::encodeName( KStandardDirs::findExe( QString::fromLatin1( "kdeinit_wrapper" ) ) );
        cmdline += " ";

        if ( rt )
            cmdline += QFile::encodeName( KStandardDirs::findExe(
                                              QString::fromLatin1( "artswrapper" ) ) );
        else
            cmdline += QFile::encodeName( KStandardDirs::findExe(
                                              QString::fromLatin1( "artsd" ) ) );

        cmdline += " ";
        cmdline += config.readEntry( "Arguments", "-F 10 -S 4096 -s 60 -m artsmessage -l 3 -f -n" ).utf8();

        int status = ::system( cmdline );

        if ( status != -1 && WIFEXITED( status ) )
        {
            // We could have a race-condition here.
            // The correct way to do it is to make artsd fork-and-exit
            // after starting to listen to connections (and running artsd
            // directly instead of using kdeinit), but this is better
            // than nothing.
            int time = 0;
            do
            {
                // every time it fails, we should wait a little longer
                // between tries
                ::sleep( 1 + time / 2 );
                m_Server = Arts::Reference( "global:Arts_SoundServerV2" );
            }
            while ( ++time < 5 && ( m_Server.isNull() ) );
        }
    }

    if ( m_Server.isNull() )
    {
        KMessageBox::error( 0, i18n( "Cannot start aRts! Exiting." ), i18n( "Fatal Error" ) );
        ::exit( 1 );
    }
    // </most of this code was taken from noatun's engine.cpp>

    m_amanPlay = Arts::DynamicCast( m_Server.createObject( "Arts::Synth_AMAN_PLAY" ) );
    m_amanPlay.title( "amarok" );
    m_amanPlay.autoRestoreID( "amarok" );
    m_amanPlay.start();

    m_XFade = Arts::DynamicCast( m_Server.createObject( "Amarok::Synth_STEREO_XFADE" ) );

    if ( m_XFade.isNull() )
    {
        KMessageBox::error( 0,
                            i18n( "Cannot find libamarokarts! Probably amaroK was installed with the \
                                   wrong prefix. Please install again using: ./configure \
                                   --prefix=`kde-config --prefix`" ),
                            i18n( "Fatal Error" ) );
        ::exit( 1 );
    }

    m_XFade.percentage( m_XFadeValue );
    m_XFade.start();

    m_scope = Arts::DynamicCast( m_Server.createObject( "Arts::StereoFFTScope" ) );

    m_globalEffectStack = Arts::DynamicCast( m_Server.createObject( "Arts::StereoEffectStack" ) );
    m_globalEffectStack.start();

    m_effectStack = Arts::DynamicCast( m_Server.createObject( "Arts::StereoEffectStack" ) );
    m_effectStack.start();
    m_globalEffectStack.insertBottom( m_effectStack, "Effect Stack" );

    Arts::connect( m_XFade, "outvalue_l", m_globalEffectStack, "inleft" );
    Arts::connect( m_XFade, "outvalue_r", m_globalEffectStack, "inright" );

    Arts::connect( m_globalEffectStack, m_amanPlay );
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
    m_pPlayerWidget->m_pSliderVol->setMaxValue( 100 );

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


void PlayerApp::initMixer()
{
    kdDebug() << "begin PlayerApp::initMixer()" << endl;

    if ( !m_optSoftwareMixerOnly && initMixerHW() )
    {
        m_usingMixerHW = true;
    }
    else
    {
        // Hardware mixer doesn't work --> use arts software-mixing
        kdDebug() << "Cannot initialise Hardware mixer. Switching to software mixing." << endl;

        m_volumeControl = Arts::DynamicCast( m_Server.createObject( "Arts::StereoVolumeControl" ) );

        if ( m_volumeControl.isNull() )
        {
            kdDebug() << "Initialising arts softwaremixing failed!" << endl;
            return ;
        }

        m_usingMixerHW = false;
        m_volumeControl.start();
        m_globalEffectStack.insertBottom( m_volumeControl, "Volume Control" );

        kdDebug() << "end PlayerApp::initMixer()" << endl;
    }
}


bool PlayerApp::initMixerHW()
{
    if ( ( m_Mixer = ::open( "/dev/mixer", O_RDWR ) ) < 0 )
        return false;

    int devmask, recmask, i_recsrc, stereodevs;
    if ( ioctl( m_Mixer, SOUND_MIXER_READ_DEVMASK, &devmask ) == -1 ) return false;
    if ( ioctl( m_Mixer, SOUND_MIXER_READ_RECMASK, &recmask ) == -1 ) return false;
    if ( ioctl( m_Mixer, SOUND_MIXER_READ_RECSRC, &i_recsrc ) == -1 ) return false;
    if ( ioctl( m_Mixer, SOUND_MIXER_READ_STEREODEVS, &stereodevs ) == -1 ) return false;
    if ( !devmask ) return false;

    return true;
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

   m_pConfig->setGroup( "Session" );
   KURL url = m_pConfig->readEntry( "Track" );

   if ( m_optResumePlayback )
   {
      //see if we also saved the time
      int seconds = m_pConfig->readNumEntry( "Time", -1 );

      if ( seconds >= 0 )
      {
         play( url );

         if ( seconds > 0 && m_pPlayObject && !m_pPlayObject->isNull() )
         {
            //FIXME I just copied this code, do I need all these properties?
            Arts::poTime time;
            time.ms = 0;
            time.seconds = seconds;
            time.custom = 0;
            time.customUnit = std::string();

            m_pPlayObject->seek( time );
         }
      }
   }
}



// METHODS --------------------------------------------------------------------------

#include <klineedit.h>     //browserWin
#include <kcombobox.h>     //browserWin::KComboHistory (file chooser lineEdit)
#include "browserwidget.h" //anoyingly necessary //FIXME KConfig XT!
#include <kdirlister.h>    //for browserwin component also

void PlayerApp::saveConfig()
{
    m_pConfig->setGroup( "" );
    m_pConfig->writeEntry ( "Version", APP_VERSION );

    m_pConfig->setGroup( "General Options" );

    m_pConfig->writeEntry( "Master Volume", m_Volume );
    m_pConfig->writeEntry( "CurrentDirectory", m_pBrowserWin->m_pBrowserWidget->m_pDirLister->url().path() );
#if KDE_IS_VERSION(3,1,3)
    m_pConfig->writePathEntry( "PathHistory", m_pBrowserWin->m_pBrowserLineEdit->historyItems() );
#else
    m_pConfig->writeEntry( "PathHistory", m_pBrowserWin->m_pBrowserLineEdit->historyItems() );
#endif
    m_pConfig->writeEntry( "PlayerPos", m_pPlayerWidget->pos() );
    m_pConfig->writeEntry( "BrowserWinPos", m_pBrowserWin->pos() );
    m_pConfig->writeEntry( "BrowserWinSize", m_pBrowserWin->size() );
    m_pConfig->writeEntry( "BrowserWinSplitter", m_pBrowserWin->m_pSplitter->sizes() );
    m_pConfig->writeEntry( "BrowserWin Enabled", m_pPlayerWidget->m_pButtonPl->isOn() );
    m_pConfig->writeEntry( "Save Playlist", m_optSavePlaylist );
    m_pConfig->writeEntry( "Confirm Clear", m_optConfirmClear );
    m_pConfig->writeEntry( "Confirm Exit", m_optConfirmExit );
    m_pConfig->writeEntry( "FollowSymlinks", m_optFollowSymlinks );
    m_pConfig->writeEntry( "Drop Mode", m_optDropMode );
    m_pConfig->writeEntry( "Time Display Remaining", m_optTimeDisplayRemaining );
    m_pConfig->writeEntry( "Repeat Track", m_optRepeatTrack );
    m_pConfig->writeEntry( "Repeat Playlist", m_optRepeatPlaylist );
    m_pConfig->writeEntry( "Random Mode", m_optRandomMode );
    m_pConfig->writeEntry( "Show MetaInfo", m_optReadMetaInfo );
    m_pConfig->writeEntry( "Show Tray Icon", m_optShowTrayIcon );
    m_pConfig->writeEntry( "Crossfade", m_optXFade );
    m_pConfig->writeEntry( "Crossfade Length", m_optXFadeLength );
    m_pConfig->writeEntry( "Track Delay Length", m_optTrackDelay );
    m_pConfig->writeEntry( "Hide Playlist Window", m_optHidePlaylistWindow );
    m_pConfig->writeEntry( "Use Custom Fonts", m_optUseCustomFonts );
    m_pConfig->writeEntry( "Browser Window Font", m_optBrowserWindowFont );
    m_pConfig->writeEntry( "Player Widget Font", m_optPlayerWidgetFont);
    m_pConfig->writeEntry( "Player Widget Scroll Font", m_optPlayerWidgetScrollFont );
    m_pConfig->writeEntry( "BrowserUseCustomColors", m_optBrowserUseCustomColors );
    m_pConfig->writeEntry( "BrowserFgColor", m_optBrowserFgColor );
    m_pConfig->writeEntry( "BrowserBgColor", m_optBrowserBgColor );
    m_pConfig->writeEntry( "Undo Levels", m_optUndoLevels );
    m_pConfig->writeEntry( "Software Mixer Only", m_optSoftwareMixerOnly );
    m_pConfig->writeEntry( "Resume Playback", m_optResumePlayback );
    m_pConfig->writeEntry( "Current Analyzer", m_optVisCurrent );
    m_pConfig->writeEntry( "Browser Sorting Spec", m_optBrowserSortSpec );
    m_pConfig->writeEntry( "Title Streaming", m_optTitleStream );

    m_pBrowserWin->m_pPlaylistWidget->saveM3u( kapp->dirs() ->saveLocation(
        "data", kapp->instanceName() + "/" ) + "current.m3u" );
}


void PlayerApp::readConfig()
{
    kdDebug() << "begin PlayerApp::readConfig()" << endl;

    // FIXME: ok, the compiler warning is gone now. but the result is the same: those variables are
    //        still temporary. so what have we gained? frankly, why does KConfig take a pointer here,
    //        anyway? I've looked at KConfig sources, and it seems, it gets just dereferened once and
    //        that's it. so &( QPoint( 0, 0 ) ); would have done the same. what do you guys think?
    QPoint pointZero = QPoint( 0, 0 );
    QSize arbitrarySize = QSize ( 600, 450 );
    QFont defaultFont( "Helvetica", 9 );
    QColor defaultColor( 0x80, 0xa0, 0xff );
    QColor black( Qt::black );

    m_pConfig->setGroup( "General Options" );

    m_pBrowserWin->m_pBrowserWidget->readDir( m_pConfig->readPathEntry( "CurrentDirectory", QDir::home().path() ) );
#if KDE_IS_VERSION(3,1,3)
    m_pBrowserWin->m_pBrowserLineEdit->setHistoryItems( m_pConfig->readPathListEntry( "PathHistory" ) );
#else
    m_pBrowserWin->m_pBrowserLineEdit->setHistoryItems( m_pConfig->readListEntry( "PathHistory" ) );
#endif
    m_pPlayerWidget->move( m_pConfig->readPointEntry( "PlayerPos", &pointZero ) );
    m_pBrowserWin->move( m_pConfig->readPointEntry( "BrowserWinPos", &pointZero ) );
    m_pBrowserWin->resize( m_pConfig->readSizeEntry( "BrowserWinSize", &arbitrarySize ) );
    m_optSavePlaylist = m_pConfig->readBoolEntry( "Save Playlist", true );
    m_optConfirmClear = m_pConfig->readBoolEntry( "Confirm Clear", false );
    m_optConfirmExit = m_pConfig->readBoolEntry( "Confirm Exit", false );

    m_optFollowSymlinks = m_pConfig->readBoolEntry( "FollowSymlinks", true ); //FIXME at some point add the space again if you like
    m_optDropMode = m_pConfig->readEntry( "Drop Mode", "Recursively" );
    m_optTimeDisplayRemaining = m_pConfig->readBoolEntry( "Time Display Remaining", false );
    m_optRepeatTrack = m_pConfig->readBoolEntry( "Repeat Track", false );
    m_optRepeatPlaylist = m_pConfig->readBoolEntry( "Repeat Playlist", false );
    m_optRandomMode = m_pConfig->readBoolEntry( "Random Mode", false );
    m_optReadMetaInfo = m_pConfig->readBoolEntry( "Show MetaInfo", true );
    m_optShowTrayIcon = m_pConfig->readBoolEntry( "Show Tray Icon", true );
    m_optXFade = m_pConfig->readNumEntry( "Crossfade", true );
    m_optXFadeLength = m_pConfig->readNumEntry( "Crossfade Length", 2500 );
    m_optTrackDelay = m_pConfig->readNumEntry( "Track Delay Length", 0 );
    m_optHidePlaylistWindow = m_pConfig->readBoolEntry( "Hide Playlist Window", true );

    if ( m_pConfig->readBoolEntry( "BrowserWin Enabled", true ) )
       m_pPlayerWidget->m_pButtonPl->setOn( true );

    m_optUseCustomFonts = m_pConfig->readBoolEntry( "Use Custom Fonts", false );
    m_optBrowserWindowFont = m_pConfig->readFontEntry( "Browser Window Font", &defaultFont );
    m_optPlayerWidgetFont = m_pConfig->readFontEntry( "Player Widget Font", &defaultFont );
    m_optPlayerWidgetScrollFont = m_pConfig->readFontEntry( "Player Widget Scroll Font", &defaultFont );

    m_pBrowserWin->slotUpdateFonts();

    m_optBrowserUseCustomColors = m_pConfig->readBoolEntry( "BrowserUseCustomColors", false );
    m_optBrowserFgColor = m_pConfig->readColorEntry( "BrowserFgColor", &defaultColor );
    m_optBrowserBgColor = m_pConfig->readColorEntry( "BrowserBgColor", &black );
    setupColors();

    m_optUndoLevels = m_pConfig->readUnsignedNumEntry( "Undo Levels", 30 );
    m_optSoftwareMixerOnly = m_pConfig->readBoolEntry( "Software Mixer Only", true );
    m_optResumePlayback = m_pConfig->readBoolEntry( "Resume Playback", false );

    //-1? See PlayerWidget::createVis() for revelations
    // -1 is because the createVis code sucks a little, but this works _and_ it's commented!
    m_optVisCurrent = m_pConfig->readUnsignedNumEntry( "Current Analyzer", 0 ) - 1;
    m_pPlayerWidget->createVis();

    m_optBrowserSortSpec = m_pConfig->readNumEntry( "Browser Sorting Spec", QDir::Name | QDir::DirsFirst );
    m_optTitleStream = m_pConfig->readBoolEntry( "Title Streaming", true );

    m_Volume = m_pConfig->readNumEntry( "Master Volume", 50 );
    slotVolumeChanged( m_Volume );
    m_pPlayerWidget->m_pSliderVol->setValue( m_Volume );

    QValueList<int> splitterList;
    splitterList = m_pConfig->readIntListEntry( "BrowserWinSplitter" );
    if ( splitterList.count() != 2 )
    {
        splitterList.clear();
        splitterList.append( 70 );
        splitterList.append( 140 );
    }
    m_pBrowserWin->m_pSplitter->setSizes( splitterList );

    m_pPlayerWidget->slotUpdateTrayIcon( m_optShowTrayIcon );

// Actions ==========

    m_pGlobalAccel->insert( "add", i18n( "Add Location" ), 0, CTRL + ALT + Key_A, 0,
                            this, SLOT( slotAddLocation() ), true, true );
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
    m_pGlobalAccel->readSettings( m_pConfig );
    m_pGlobalAccel->updateConnections();


    //FIXME use a global actionCollection (perhaps even at global scope)
    m_pPlayerWidget->m_pActionCollection->readShortcutSettings( QString::null, m_pConfig );
    m_pBrowserWin->m_pActionCollection->readShortcutSettings( QString::null, m_pConfig );

    kdDebug() << "end PlayerApp::readConfig()" << endl;
}


bool PlayerApp::queryClose()
{
    if ( m_optConfirmExit )
        if ( KMessageBox::questionYesNo( 0, i18n( "Really exit the program?" ) ) == KMessageBox::No )
            return false;

    return true;
}


void PlayerApp::setupTrackLength()
{
    if ( m_pPlayObject != NULL )
    {
        Arts::poTime timeO( m_pPlayObject->overallTime() );

        m_length = timeO.seconds;
        m_pPlayerWidget->m_pSlider->setMaxValue( static_cast<int>( timeO.seconds ) );
    }
    else
    {
       m_length = 0;
    }
}


//FIXME use const & QStrings!!
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


void PlayerApp::startXFade()
{
    kdDebug() << "void PlayerApp::startXFade()" << endl;

    if ( m_XFadeRunning )
        stopXFade();

    m_XFadeRunning = true;

    m_pPlayObjectXFade = m_pPlayObject;
    m_pPlayObject = NULL;
}


void PlayerApp::stopXFade()
{
    kdDebug() << "void PlayerApp::stopXFade()" << endl;

    m_XFadeRunning = false;

    if ( m_XFadeCurrent == "invalue2" )
        m_XFadeValue = 0.0;
    else
        m_XFadeValue = 1.0;

    m_XFade.percentage( m_XFadeValue );

    if ( m_pPlayObjectXFade != NULL )
    {
        m_pPlayObjectXFade->halt();

        delete m_pPlayObjectXFade;
        m_pPlayObjectXFade = NULL;
    }
}


void PlayerApp::setupColors()
{
    // we try to be smart: this code figures out contrasting colors for selection and alternate background rows
    int h, s, v;

    m_optBrowserBgColor.hsv( &h, &s, &v );
    if ( v < 128 )
        v += 50;
    else
        v -= 50;
    m_optBrowserBgAltColor.setHsv( h, s, v );

    m_optBrowserFgColor.hsv( &h, &s, &v );
    if ( v < 128 )
        v += 150;
    else
        v -= 150;
    if ( v < 0 )
        v = 0;
    if ( v > 255 )
        v = 255;
    m_optBrowserSelColor.setHsv( h, s, v );

    m_pBrowserWin->setPalettes( m_optBrowserFgColor, m_optBrowserBgColor, m_optBrowserBgAltColor );
}


// SLOTS -----------------------------------------------------------------

void PlayerApp::slotPrev() const { m_pBrowserWin->m_pPlaylistWidget->request( PlaylistWidget::Prev, m_bIsPlaying ); }
void PlayerApp::slotNext() const { m_pBrowserWin->m_pPlaylistWidget->request( PlaylistWidget::Next, m_bIsPlaying ); }
void PlayerApp::slotPlay() const { m_pBrowserWin->m_pPlaylistWidget->request( PlaylistWidget::Current ); }


#include <math.h> //FIXME: I put it here so we remember it's only used by this function and the one somewhere below

void PlayerApp::play( const KURL &url, const MetaBundle *tags )
{
    if ( m_optXFade && m_bIsPlaying && url != m_playingURL )
        startXFade();

    else if ( m_pPlayObject != NULL )
        slotStopCurrent();

    KDE::PlayObjectFactory factory( m_Server );

    if ( m_optTitleStream && !m_proxyError && !url.isLocalFile()  )
    {
        TitleProxy *pProxy = new TitleProxy( url );
        m_pPlayObject = factory.createPlayObject( pProxy->proxyUrl(), false );

        connect( m_pPlayObject, SIGNAL( destroyed() ),
                 pProxy, SLOT( deleteLater() ) );
        connect( pProxy, SIGNAL( metaData( QString, QString, QString ) ),
                 this, SLOT( receiveStreamMeta( QString, QString, QString ) ) );
        connect( pProxy, SIGNAL( error() ), this, SLOT( proxyError() ) );
    }
    else
    {
        m_pPlayObject = factory.createPlayObject( url, false ); //second parameter:
                                                                //create BUS(true/false)
    }

    m_proxyError = false;

    if ( m_pPlayObject == NULL )
    {
        kdDebug() << "Can't initialize Playobject. m_pPlayObject == NULL." << endl;
        slotNext();
        return;
    }
    if ( m_pPlayObject->isNull() )
    {
        kdDebug() << "Can't initialize Playobject. m_pPlayObject->isNull()." << endl;
        delete m_pPlayObject;
        m_pPlayObject = NULL;
        slotNext();
        return;
    }

    if ( m_pPlayObject->object().isNull() )
        connect( m_pPlayObject, SIGNAL( playObjectCreated() ), this, SLOT( slotConnectPlayObj() ) );
    else
        slotConnectPlayObj();


    if ( tags )
    {
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
    }
    else
    {
       m_pPlayerWidget->setScroll( url.fileName() );
    }


    kdDebug() << "[play()] Playing " << url.prettyURL() << endl;
    m_pPlayObject->play();
    m_bIsPlaying = true;

    //set track length stuff
    if ( m_pPlayObject->stream() )
    {
        m_pPlayerWidget->m_pSlider->setMaxValue( 0 );
        m_pPlayerWidget->setScroll( i18n( "Stream from: %1" ).arg( url.prettyURL() ), "?", "--" );
    }

    m_length = 0; //length will be established when arts knows after a few timer iterations
    m_playingURL = url;

    m_pPlayerWidget->m_pSlider->setValue( 0 );
    m_pPlayerWidget->m_pSlider->setMinValue( 0 );

    //interface consistency
    m_pPlayerWidget->m_pButtonPlay->setOn( true );
    m_pPlayerWidget->m_pButtonPause->setDown( false );
}


void PlayerApp::slotConnectPlayObj()
{
    if ( !m_pPlayObject->object().isNull() )
    {
        m_pPlayObject->object()._node()->start();

        if ( m_XFadeCurrent == "invalue1" )
            m_XFadeCurrent = "invalue2";
        else
            m_XFadeCurrent = "invalue1";

        if ( !m_XFadeRunning )
        {
            if ( m_XFadeCurrent == "invalue2" )
                m_XFade.percentage( m_XFadeValue = 0.0 );
            else
                m_XFade.percentage( m_XFadeValue = 1.0 );
        }

        Arts::connect( m_pPlayObject->object(), "left", m_XFade, ( m_XFadeCurrent + "_l" ).latin1() );
        Arts::connect( m_pPlayObject->object(), "right", m_XFade, ( m_XFadeCurrent + "_r" ).latin1() );
    }
}


void PlayerApp::proxyError()
{
    m_proxyError = true;

    slotStopCurrent(); //FIXME play() does this for us (?)
    slotPlay();
}


void PlayerApp::slotPause()
{
    if ( m_bIsPlaying && m_pPlayObject != NULL )
    {
        if ( m_pPlayObject->state() == Arts::posPaused )
        {
            m_pPlayObject->play();
            m_pPlayerWidget->m_pButtonPause->setDown( false );
        }
        else
        {
            m_pPlayObject->pause();
            m_pPlayerWidget->m_pButtonPause->setDown( true );
        }
    }
}


void PlayerApp::slotStop()
{
     stopXFade();
     slotStopCurrent();

     m_pPlayerWidget->m_pButtonPlay->setOn( false );
     m_pPlayerWidget->m_pButtonPause->setDown( false );
     m_pPlayerWidget->setScroll();
}


void PlayerApp::slotStopCurrent()
{
     if ( m_pPlayObject != NULL )
     {
         m_pPlayObject->halt();

         delete m_pPlayObject;
         m_pPlayObject = NULL;
     }

     m_bIsPlaying = false;
     m_length = 0;

     //FIXME would be nice to do this in slotStop at some point (when we no longer delay determination of length)
     m_pPlayerWidget->m_pSlider->setValue( 0 );
     m_pPlayerWidget->m_pSlider->setMinValue( 0 ); //FIXME disable it and setvalue(0) instead (?)
     m_pPlayerWidget->m_pSlider->setMaxValue( 0 );
     m_pPlayerWidget->timeDisplay( false, 0, 0, 0 );
}


bool PlayerApp::playObjectConfigurable()
{
    if ( m_pPlayObject && !m_pPlayObject->object().isNull() && !m_pPlayerWidget->m_pPlayObjConfigWidget )
    {
        Arts::TraderQuery query;
        query.supports( "Interface", "Arts::GuiFactory" );
        query.supports( "CanCreate", m_pPlayObject->object()._interfaceName() );

        std::vector<Arts::TraderOffer> *queryResults = query.query();
        bool yes = queryResults->size();
        delete queryResults;

        return yes;
    }

    return false;
}


void PlayerApp::slotSliderPressed()
{
    m_bSliderIsPressed = true;
}


void PlayerApp::slotSliderReleased()
{
    if ( m_bIsPlaying && m_pPlayObject != NULL )
    {
        Arts::poTime time;
        time.ms = 0;
        time.seconds = static_cast<long>( m_pPlayerWidget->m_pSlider->value() );
        time.custom = 0;
        time.customUnit = std::string();
        m_pPlayObject->seek( time );
    }

    m_bSliderIsPressed = false;
}


void PlayerApp::slotSliderChanged( int value )
{
    if ( m_bSliderIsPressed )
    {
        if ( m_optTimeDisplayRemaining )
        {
            value = m_length - value;
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
    value = 100 - value;

    if ( m_usingMixerHW )
    {
        value = value + ( value << 8 );
        ioctl( m_Mixer, MIXER_WRITE( 4 ), &value );
    }
    else
    {
        //convert percent to factor
        m_volumeControl.scaleFactor( 0.01 * static_cast<float>( value ) );
    }
}


void PlayerApp::slotMainTimer()
{
    if ( m_pPlayObject == NULL || m_pPlayObject->isNull() )
    {
        disableScope();
        return;
    }

    if ( ( m_length == 0 ) && !m_pPlayObject->stream() )
    {
       Arts::poTime timeO( m_pPlayObject->overallTime() );

       m_length = timeO.seconds;
       m_pPlayerWidget->m_pSlider->setMaxValue( static_cast<int>( m_length ) );

       kdDebug() << "length: " << m_length << endl;
    }

    if ( m_bSliderIsPressed || !m_bIsPlaying )
        return;

    //only enable scope when needed, so we save some cpu
    if ( m_pPlayObject->state() == Arts::posPlaying )
        enableScope();
    else
        disableScope();

    Arts::poTime timeC( m_pPlayObject->currentTime() );
    m_pPlayerWidget->m_pSlider->setValue( static_cast<int>( timeC.seconds ) );

    // <Draw TimeDisplay>
    if ( m_pPlayerWidget->isVisible() )
    {
        int seconds;
        if ( m_optTimeDisplayRemaining && !m_pPlayObject->stream() )
        {
            seconds = m_length - timeC.seconds;
            m_pPlayerWidget->timeDisplay( true, seconds / 60 / 60 % 60, seconds / 60 % 60, seconds % 60 );
        }
        else
        {
            seconds = timeC.seconds;
            m_pPlayerWidget->timeDisplay( false, seconds / 60 / 60 % 60, seconds / 60 % 60, seconds % 60 );
        }
    }
    // </Draw TimeDisplay>

    // <Crossfading>
    if ( ( !m_XFadeRunning ) && //this logic order is probably most efficient
         ( m_optXFade ) &&
         ( !m_optRepeatTrack ) &&
         ( !m_pPlayObject->stream() ) &&
         ( m_length ) &&
         ( m_length * 1000 - ( timeC.seconds * 1000 + timeC.ms ) < m_optXFadeLength )  )
    {
        slotNext();
        return;
    }
    if ( m_XFadeRunning )
    {
        float xfadeStep = 1.0 / m_optXFadeLength * MAIN_TIMER;

        if ( m_XFadeCurrent == "invalue2" )
            m_XFadeValue -= xfadeStep;
        else
            m_XFadeValue += xfadeStep;

        if ( m_XFadeValue < 0.0 )
            m_XFadeValue = 0.0;
        else if ( m_XFadeValue > 1.0 )
            m_XFadeValue = 1.0;

//         kdDebug() << "[slotMainTimer] m_XFadeValue: " << m_XFadeValue << endl;
        m_XFade.percentage( m_XFadeValue );

        if( m_XFadeValue == 0.0 || m_XFadeValue == 1.0 )
            stopXFade();
    }
    // </Crossfading>

    // check if track has ended
    if ( m_pPlayObject->state() == Arts::posIdle )
    {
        if ( m_optTrackDelay > 0 ) //this can occur syncronously to XFade and not be fatal
        {
            //delay before start of next track, without freezing the app
            m_DelayTime += MAIN_TIMER;
            if ( m_DelayTime >= (m_optTrackDelay * 1000) )
            {
                m_DelayTime = 0;
                slotNext();
            }
        }
        else if( !m_pBrowserWin->m_pPlaylistWidget->request( PlaylistWidget::Next, m_bIsPlaying ) )
        {
            slotStop();
        }
    }
}


void PlayerApp::enableScope()
{
    if ( !m_scopeId )
    {
        m_scope.start();
        m_scopeId = m_globalEffectStack.insertTop( m_scope, "Analyzer" );
    }
}


void PlayerApp::disableScope()
{
    if ( m_scopeId )
    {
        m_scope.stop();
        m_globalEffectStack.remove( m_scopeId );
        m_scopeId = 0;
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
        if ( m_scopeId )
        {
            std::vector<float> *pScopeVector = m_scope.scope();
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
    ConfigDlg *pDlg = new ConfigDlg();
    connect( pDlg, SIGNAL( sigShowTrayIcon( bool ) ), m_pPlayerWidget, SLOT( slotUpdateTrayIcon( bool ) ) );
    pDlg->show();
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

    if ( m_optHidePlaylistWindow )
       m_pBrowserWin->hide();
}

void PlayerApp::slotShow()
{
    if ( m_pPlayerWidget->m_pButtonPl->isOn() )
    {
        m_pBrowserWin->show();
    }
}


#include "playerapp.moc"
