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
#include "browserwidget.h"
#include "browserwin.h"
#include "configdlg.h"
#include "effectwidget.h"
#include "expandbutton.h"
#include "playerapp.h"
#include "playerwidget.h"
#include "playlistitem.h"
#include "playlistwidget.h"
#include "viswidget.h"

#include "debugareas.h"

#include <vector>
#include <string>

#include <kaboutapplication.h>
#include <kaction.h>
#include <kapp.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kfile.h>
#include <kfiledialog.h>
#include <kglobalaccel.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kshortcut.h>
#include <kstandarddirs.h>
#include <ktip.h>
#include <kuniqueapplication.h>
#include <kurl.h>
#include <kurlrequester.h>
#include <kurlrequesterdlg.h>

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

#include <qdialog.h>
#include <qdir.h>
#include <qpoint.h>
#include <qpopupmenu.h>
#include <qsize.h>
#include <qstring.h>
#include <qtimer.h>
#include <qvaluelist.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/wait.h>

#define MAIN_TIMER 150

PlayerApp::PlayerApp() :
        KUniqueApplication( true, true, false ),
        m_bgColor( Qt::black ),
        m_fgColor( QColor( 0x80, 0xa0, 0xff ) ),
        m_pPlayObject( NULL ),
        m_pPlayObjectXFade( NULL ),
        m_pArtsDispatcher( NULL ),
        m_length( 0 ),
        m_playRetryCounter( 0 ),
        m_pEffectWidget( NULL ),
        m_bIsPlaying( false ),
        m_bChangingSlider( false ),
        m_XFadeRunning( false ),
        m_XFadeValue( 1.0 ),
        m_XFadeCurrent( "invalue1" )
{
    setName( "amarok" );

    pApp = this; //global

    m_pConfig = kapp->config();
    m_pGlobalAccel = new KGlobalAccel( this );

    initArts();
    if ( !initScope() )
    {
        KMessageBox::error( 0, i18n( "Cannot find libamarokarts! Maybe installed in the wrong directory? Aborting.." ), i18n( "Fatal Error" ) );
        return ;
    }
    initPlayerWidget();
    initMixer();
    initBrowserWin();

    readConfig();

//    connect( this, SIGNAL( sigplay() ), this, SLOT( slotPlay() ) );
    connect( this, SIGNAL( saveYourself() ), this, SLOT( saveSessionState() ) );

    connect( m_pPlayerWidget, SIGNAL( sigMinimized() ), this, SLOT( slotWidgetMinimized() ) );
    connect( m_pPlayerWidget, SIGNAL( sigRestored() ), this, SLOT( slotWidgetRestored() ) );

    m_pMainTimer = new QTimer( this );
    connect( m_pMainTimer, SIGNAL( timeout() ), this, SLOT( slotMainTimer() ) );

    m_pMainTimer->start( MAIN_TIMER );

    m_pAnimTimer = new QTimer( this );
    connect( m_pAnimTimer, SIGNAL( timeout() ), this, SLOT( slotAnimTimer() ) );
    m_pAnimTimer->start( 25 );

    m_pPlayerWidget->show();

    KTipDialog::showTip( "amarok/data/startupTip.txt", false );
}


PlayerApp::~PlayerApp()
{
    kdDebug(DA_COMMON) << "PlayerApp:~PlayerApp()" << endl;

    slotStop();

    killTimers();
    saveConfig();

    delete m_pEffectWidget;
    delete m_pPlayerWidget;

    m_XFade = Amarok::Synth_STEREO_XFADE::null();
    m_Scope = Arts::StereoFFTScope::null();
    m_volumeControl = Arts::StereoVolumeControl::null();
    m_effectStack = Arts::StereoEffectStack::null();
    m_globalEffectStack = Arts::StereoEffectStack::null();
    m_amanPlay = Arts::Synth_AMAN_PLAY::null();
    m_Server = Arts::SoundServerV2::null();

    delete m_pArtsDispatcher;
}


int PlayerApp::newInstance()
{
    KCmdLineArgs * args = KCmdLineArgs::parsedArgs();

    QCString playlistUrl = args->getOption( "playlist" );

    if ( !playlistUrl.isEmpty() )             //playlist
    {
        m_pBrowserWin->m_pPlaylistWidget->clear();
        m_pBrowserWin->m_pPlaylistWidget->loadPlaylist( KCmdLineArgs::makeURL( playlistUrl ).path(), 0 );
        m_pBrowserWin->m_pPlaylistWidget->writeUndo();

    }

    if ( args->count() > 0 )
    {
        if ( args->isSet( "e" ) )             //enqueue
        {
            for ( int i = 0; i < args->count(); i++ )
            {
                if ( !m_pBrowserWin->m_pPlaylistWidget->
                     loadPlaylist( args->url( i ), m_pBrowserWin->m_pPlaylistWidget->lastItem() ) )
                {
                    if ( isFileValid( args->url( i ) ) )
                        m_pBrowserWin->m_pPlaylistWidget->addItem( ( PlaylistItem* ) 1, args->url( i ) );
                }
            }
            m_pBrowserWin->m_pPlaylistWidget->writeUndo();
        }
        else                              //URLs
        {
            m_pBrowserWin->m_pPlaylistWidget->clear();

            for ( int i = 0; i < args->count(); i++ )
            {
                if ( !m_pBrowserWin->m_pPlaylistWidget->loadPlaylist( args->url( i ), 0 ) )
                {
                    if ( isFileValid( args->url( i ) ) )
                        m_pBrowserWin->m_pPlaylistWidget->addItem( 0, args->url( i ) );
                }
            }
            m_pBrowserWin->m_pPlaylistWidget->writeUndo();
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


void PlayerApp::restore()
{
    //attempt to restore previous session
    KConfig * config = sessionConfig();

    KURL url = config->readEntry( "track" );
    int seconds = config->readNumEntry( "position" );

    //FIXME REFACTOR this is duplicated in slotAddLocation, reduce LOC

    if ( !url.isEmpty() && url.isValid() )
    {
        if ( isFileValid( url ) )
        {
            //FIXME see if item is in playlist already first (should be!)
            PlaylistItem * item = new PlaylistItem( m_pBrowserWin->m_pPlaylistWidget, url );

            //FIXME I just copied this code, do I need all these properties?
            Arts::poTime time;
            time.ms = 0;
            time.seconds = seconds;
            time.custom = 0;
            time.customUnit = std::string();

            m_pBrowserWin->m_pPlaylistWidget->setCurrentTrack( item );
            slotPlay();

            // try to fix a crash on session restore when there's nothing to restore
            if ( m_pPlayObject && !m_pPlayObject->isNull() )
                m_pPlayObject->seek( time );
        }
    }
}


//session management
void PlayerApp::saveSessionState()
{
    KConfig * config = sessionConfig();

    if( m_bIsPlaying )
    {
      Arts::poTime timeC( m_pPlayObject->currentTime() );

      config->writeEntry( "track", static_cast<PlaylistItem*>( m_pBrowserWin->m_pPlaylistWidget->currentTrack() ) ->url().url() );
      config->writeEntry( "position", timeC.seconds );
    }
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
            kdDebug(DA_COMMON) << "killall artsd succeeded." << endl;
        }
    }
    m_pArtsDispatcher = new KArtsDispatcher();

    // *** most of the following code was taken from noatun's engine.cpp
    m_Server = Arts::Reference( "global:Arts_SoundServerV2" );
    if ( m_Server.isNull() || m_Server.error() )
    {
        qDebug( "aRtsd not running.. trying to start" );
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
        exit( 1 );
    }
    // *** until here

    m_amanPlay = Arts::DynamicCast( m_Server.createObject( "Arts::Synth_AMAN_PLAY" ) );
    m_amanPlay.title( "amarok" );
    m_amanPlay.autoRestoreID( "amarok" );
    m_amanPlay.start();

    m_XFade = Arts::DynamicCast( m_Server.createObject( "Amarok::Synth_STEREO_XFADE" ) );
    m_XFade.percentage( m_XFadeValue );
    m_XFade.start();

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
    //TEST
    kdDebug(DA_COMMON) << "begin PlayerApp::initPlayerWidget()" << endl;

    m_pPlayerWidget = new PlayerWidget();
    //    setCentralWidget(m_pPlayerWidget);

    m_bSliderIsPressed = false;

    m_pPlayerWidget->m_pSlider->setMinValue( 0 );
    m_pPlayerWidget->m_pSlider->setMaxValue( 0 );
    m_pPlayerWidget->m_pSlider->setValue( 0 );

    m_pPlayerWidget->m_pSliderVol->setMinValue( 0 );
    m_pPlayerWidget->m_pSliderVol->setMaxValue( 100 );
    m_pPlayerWidget->m_pSliderVol->setValue( m_Volume );


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

    connect( m_pPlayerWidget->m_pButtonEq, SIGNAL( toggled( bool ) ),
             this, SLOT( slotEq( bool ) ) );

    connect( m_pPlayerWidget->m_pButtonLogo, SIGNAL( clicked() ),
             this, SLOT( slotShowAbout() ) );

    //TEST
    kdDebug(DA_COMMON) << "end PlayerApp::initPlayerWidget()" << endl;
}


void PlayerApp::initMixer()
{
    //TEST
    kdDebug(DA_COMMON) << "begin PlayerApp::initMixer()" << endl;

    if ( !m_optSoftwareMixerOnly && initMixerHW() )
    {
        m_usingMixerHW = true;
    }
    else
    {
        // Hardware mixer doesn't work --> use arts software-mixing
        kdDebug(DA_COMMON) << "Cannot initialise Hardware mixer. Switching to software mixing." << endl;

        m_volumeControl = Arts::DynamicCast( m_Server.createObject( "Arts::StereoVolumeControl" ) );

        if ( m_volumeControl.isNull() )
        {
            kdDebug(DA_COMMON) << "Initialising arts softwaremixing failed!" << endl;
            return ;
        }

        m_usingMixerHW = false;
        m_volumeControl.start();
        m_globalEffectStack.insertBottom( m_volumeControl, "Volume Control" );

        //TEST
        kdDebug(DA_COMMON) << "end PlayerApp::initMixer()" << endl;
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


bool PlayerApp::initScope()
{
    //TEST
    kdDebug(DA_COMMON) << "begin PlayerApp::initScope()" << endl;

    m_Scope = Arts::DynamicCast( m_Server.createObject( "Arts::StereoFFTScope" ) );

    if ( m_Scope.isNull() )
    {
        kdDebug(DA_COMMON) << "m_Scope.isNull()!" << endl;
        return false;
    }

    m_scopeActive = false;
    m_globalEffectStack.insertBottom( m_Scope, "Analyzer" );

    //TEST
    kdDebug(DA_COMMON) << "end PlayerApp::initScope()" << endl;
    return true;
}


void PlayerApp::initBrowserWin()
{
    //TEST
    kdDebug(DA_COMMON) << "begin PlayerApp::initBrowserWin()" << endl;

    m_pBrowserWin = new BrowserWin();

    connect( m_pBrowserWin->m_pButtonAdd, SIGNAL( clicked() ),
             this, SLOT( slotAddLocation() ) );

    connect( m_pBrowserWin->m_pButtonSave, SIGNAL( clicked() ),
             this, SLOT( slotSavePlaylist() ) );

    connect( m_pBrowserWin->m_pButtonClear, SIGNAL( clicked() ),
             this, SLOT( slotClearPlaylistAsk() ) );

    connect( m_pBrowserWin->m_pButtonUndo, SIGNAL( clicked() ),
             m_pBrowserWin->m_pPlaylistWidget, SLOT( doUndo() ) );

    connect( m_pBrowserWin->m_pButtonRedo, SIGNAL( clicked() ),
             m_pBrowserWin->m_pPlaylistWidget, SLOT( doRedo() ) );

    connect( m_pBrowserWin->m_pButtonPlay, SIGNAL( clicked() ),
             this, SLOT( slotPlay() ) );

    connect( m_pBrowserWin->m_pButtonPause, SIGNAL( clicked() ),
             this, SLOT( slotPause() ) );

    connect( m_pBrowserWin->m_pButtonStop, SIGNAL( clicked() ),
             this, SLOT( slotStop() ) );

    connect( m_pBrowserWin->m_pButtonNext, SIGNAL( clicked() ),
             this, SLOT( slotNext() ) );

    connect( m_pBrowserWin->m_pButtonPrev, SIGNAL( clicked() ),
             this, SLOT( slotPrev() ) );

    connect( m_pBrowserWin->m_pPlaylistWidget, SIGNAL( doubleClicked( QListViewItem* ) ),
             this, SLOT( slotItemDoubleClicked( QListViewItem* ) ) );

    connect( m_pBrowserWin, SIGNAL( signalHide() ),
             this, SLOT( slotPlaylistHide() ) );

    //TEST
    kdDebug(DA_COMMON) << "end PlayerApp::initBrowserWin()" << endl;
}


// METHODS --------------------------------------------------------------------------

QString PlayerApp::convertDigit( const long &digit )
{
    QString str, str1;
    str.setNum( digit );

    if ( digit > 9 )
    {
        return str;
    }

    str1 = "0" + str;
    return str1;
}


bool PlayerApp::isFileValid( const KURL &url )
{
    KFileItem fileItem( KFileItem::Unknown, KFileItem::Unknown, url );
    KMimeType::Ptr mimeTypePtr = fileItem.determineMimeType();

    Arts::TraderQuery query;
    query.supports( "Interface", "Arts::PlayObject" );
    query.supports( "MimeType", mimeTypePtr->name().latin1() );
    std::vector<Arts::TraderOffer> *offers = query.query();

    if ( offers->empty() )
    {
        delete offers;
        return false;
    }

    delete offers;
    return true;
}


void PlayerApp::saveConfig()
{
    m_pConfig->setGroup( "" );
    m_pConfig->writeEntry ( "Version", APP_VERSION );

    m_pConfig->setGroup( "General Options" );

    m_pConfig->writeEntry( "Master Volume", m_Volume );
    m_pConfig->writeEntry( "CurrentDirectory" , m_pBrowserWin->m_pBrowserWidget->m_pDirLister->url().path() );
    m_pConfig->writeEntry( "PlayerPos", m_pPlayerWidget->pos() );
    m_pConfig->writeEntry( "BrowserWinPos", m_pBrowserWin->pos() );
    m_pConfig->writeEntry( "BrowserWinSize", m_pBrowserWin->size() );
    m_pConfig->writeEntry( "BrowserWinSplitter", m_pBrowserWin->m_pSplitter->sizes() );
    m_pConfig->writeEntry( "BrowserWin Enabled", m_pPlayerWidget->m_pButtonPl->isOn() );
    m_pConfig->writeEntry( "Save Playlist", m_optSavePlaylist );
    m_pConfig->writeEntry( "Confirm Clear", m_optConfirmClear );
    m_pConfig->writeEntry( "Confirm Exit", m_optConfirmExit );
    m_pConfig->writeEntry( "Follow Symlinks", m_optFollowSymlinks );
    m_pConfig->writeEntry( "Drop Mode", m_optDropMode );
    m_pConfig->writeEntry( "Time Display Remaining", m_optTimeDisplayRemaining );
    m_pConfig->writeEntry( "Repeat Track", m_optRepeatTrack );
    m_pConfig->writeEntry( "Repeat Playlist", m_optRepeatPlaylist );
    m_pConfig->writeEntry( "Random Mode", m_optRandomMode );
    m_pConfig->writeEntry( "Show MetaInfo", m_optReadMetaInfo );
    m_pConfig->writeEntry( "Show Tray Icon", m_optShowTrayIcon );
    m_pConfig->writeEntry( "Crossfading", m_optXFade );
    m_pConfig->writeEntry( "Crossfade Length", m_optXFadeLength );
    m_pConfig->writeEntry( "Hide Playlist Window", m_optHidePlaylistWindow );
    m_pConfig->writeEntry( "Browser Window Font", m_optBrowserWindowFont );
    m_pConfig->writeEntry( "Player Widget Font", m_optPlayerWidgetFont);
    m_pConfig->writeEntry( "Player Widget Scroll Font", m_optPlayerWidgetScrollFont );
    m_pConfig->writeEntry( "BrowserUseCustomColors", m_optBrowserUseCustomColors );
    m_pConfig->writeEntry( "BrowserFgColor", m_optBrowserFgColor );
    m_pConfig->writeEntry( "BrowserBgColor", m_optBrowserBgColor );
    m_pConfig->writeEntry( "Undo Levels", m_optUndoLevels );
    m_pConfig->writeEntry( "Software Mixer Only", m_optSoftwareMixerOnly );
    m_pConfig->writeEntry( "Current Analyzer", m_optVisCurrent );
    m_pConfig->writeEntry( "Browser Sorting Spec", m_optBrowserSortSpec );

    // Write playlist columns layout
    m_pBrowserWin->m_pPlaylistWidget->saveLayout(m_pConfig, "PlaylistColumnsLayout");

    //store current item
    PlaylistItem *item = static_cast<PlaylistItem*>( m_pBrowserWin->m_pPlaylistWidget->currentTrack() );
    if ( item != NULL )
    {
        if ( item->url().protocol() == "file" )
            m_pConfig->writeEntry( "CurrentSelection", item->url().path() );
        else
            m_pConfig->writeEntry( "CurrentSelection", item->url().url() );
    }

    m_pBrowserWin->m_pPlaylistWidget->saveM3u( kapp->dirs() ->saveLocation(
        "data", kapp->instanceName() + "/" ) + "current.m3u" );
}


void PlayerApp::readConfig()
{
    //TEST
    kdDebug(DA_COMMON) << "begin PlayerApp::readConfig()" << endl;

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

    m_pBrowserWin->m_pBrowserWidget->readDir( m_pConfig->readPathEntry( "CurrentDirectory", "/" ) );
    m_pPlayerWidget->move( m_pConfig->readPointEntry( "PlayerPos", &pointZero ) );
    m_pBrowserWin->move( m_pConfig->readPointEntry( "BrowserWinPos", &pointZero ) );
    m_pBrowserWin->resize( m_pConfig->readSizeEntry( "BrowserWinSize", &arbitrarySize ) );
    m_optSavePlaylist = m_pConfig->readBoolEntry( "Save Playlist", false );
    m_optConfirmClear = m_pConfig->readBoolEntry( "Confirm Clear", false );
    m_optConfirmExit = m_pConfig->readBoolEntry( "Confirm Exit", false );
    m_optFollowSymlinks = m_pConfig->readBoolEntry( "Follow Symlinks", false );
    m_optDropMode = m_pConfig->readEntry( "Drop Mode", "Recursively" );
    m_optTimeDisplayRemaining = m_pConfig->readBoolEntry( "Time Display Remaining", false );
    m_optRepeatTrack = m_pConfig->readBoolEntry( "Repeat Track", false );
    m_optRepeatPlaylist = m_pConfig->readBoolEntry( "Repeat Playlist", false );
    m_optRandomMode = m_pConfig->readBoolEntry( "Random Mode", false );
    m_optReadMetaInfo = m_pConfig->readBoolEntry( "Show MetaInfo", false );
    m_optShowTrayIcon = m_pConfig->readBoolEntry( "Show Tray Icon", true );
    m_optXFade = m_pConfig->readBoolEntry( "Crossfading", true );
    m_optXFadeLength = m_pConfig->readNumEntry( "Crossfade Length", 3000 );
    m_optHidePlaylistWindow = m_pConfig->readBoolEntry( "Hide Playlist Window", true );

    m_optBrowserWindowFont = m_pConfig->readFontEntry( "Browser Window Font", &defaultFont );
    m_optPlayerWidgetFont = m_pConfig->readFontEntry( "Player Widget Font", &defaultFont );
    m_optPlayerWidgetScrollFont = m_pConfig->readFontEntry( "Player Widget Scroll Font", &defaultFont );
    m_pBrowserWin->slotUpdateFonts();

    m_optBrowserUseCustomColors = m_pConfig->readBoolEntry( "BrowserUseCustomColors", false );
    m_optBrowserFgColor = m_pConfig->readColorEntry( "BrowserFgColor", &defaultColor );
    m_optBrowserBgColor = m_pConfig->readColorEntry( "BrowserBgColor", &black );
    setupColors();

    m_optUndoLevels = m_pConfig->readUnsignedNumEntry( "Undo Levels", 30 );
    m_optSoftwareMixerOnly = m_pConfig->readBoolEntry( "Software Mixer Only", false );

    m_optVisCurrent = m_pConfig->readUnsignedNumEntry( "Current Analyzer", 0 );
    m_pPlayerWidget->createVis();

    m_optBrowserSortSpec = m_pConfig->readNumEntry( "Browser Sorting Spec", QDir::Name | QDir::DirsFirst );

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

    if ( m_pConfig->readBoolEntry( "BrowserWin Enabled" ) == true )
    {
        m_pPlayerWidget->m_pButtonPl->setOn( true );
        m_pBrowserWin->show();
    }

    m_pBrowserWin->m_pPlaylistWidget->clear();
    m_pBrowserWin->m_pPlaylistWidget->loadPlaylist( kapp->dirs()->saveLocation(
            "data", kapp->instanceName() + "/" ) + "current.m3u", 0 );
    m_pBrowserWin->m_pPlaylistWidget->writeUndo();

    m_pPlayerWidget->slotUpdateTrayIcon( m_optShowTrayIcon );

    // Read playlist columns layout
    m_pBrowserWin->m_pPlaylistWidget->restoreLayout(m_pConfig, "PlaylistColumnsLayout");

    KURL currentlyPlaying = m_pConfig->readEntry( "CurrentSelection" );

    kdDebug(DA_COMMON) << "Attempting to select: " << currentlyPlaying.path() << endl;

    for ( PlaylistItem * item = static_cast<PlaylistItem*>( m_pBrowserWin->m_pPlaylistWidget->firstChild() );
            item;
            item = static_cast<PlaylistItem*>( item->nextSibling() ) )
    {
        if ( item->url() == currentlyPlaying )
        {
            //FIXME: should think about making this all one call
            m_pBrowserWin->m_pPlaylistWidget->setCurrentItem( item );
            m_pBrowserWin->m_pPlaylistWidget->ensureItemVisible( item );
            m_pBrowserWin->m_pPlaylistWidget->slotGlowTimer();
            break;
        }
    }
    
// Actions ==========
    m_pGlobalAccel->insert( "add", "Add Location", 0, CTRL + ALT + Key_A, 0,
                            this, SLOT( slotAddLocation() ), true, true );
    m_pGlobalAccel->insert( "play", "Play", 0, CTRL + ALT + Key_P, 0,
                            this, SLOT( slotPlay() ), true, true );
    m_pGlobalAccel->insert( "stop", "Stop", 0, CTRL + ALT + Key_S, 0,
                            this, SLOT( slotStop() ), true, true );
    m_pGlobalAccel->insert( "next", "Next Track", 0, CTRL + ALT + Key_N, 0,
                            this, SLOT( slotNext() ), true, true );
    m_pGlobalAccel->insert( "prev", "Previous Track", 0, CTRL + ALT + Key_R, 0,
                            this, SLOT( slotPrev() ), true, true );
    
    m_pGlobalAccel->readSettings( m_pConfig );
    m_pGlobalAccel->updateConnections();

    m_pPlayerWidget->m_pActionCollection->readShortcutSettings( QString::null, m_pConfig );
    m_pBrowserWin->m_pActionCollection->readShortcutSettings( QString::null, m_pConfig );
    
    //TEST
    kdDebug(DA_COMMON) << "end PlayerApp::readConfig()" << endl;
}


bool PlayerApp::queryClose()
{
    if ( m_optConfirmExit )
        if ( KMessageBox::questionYesNo( 0, i18n( "Really exit the program?" ) ) == KMessageBox::No )
            return false;

    return true;
}


void PlayerApp::getTrackLength()
{
    if ( m_pPlayObject != NULL )
    {
        Arts::poTime timeO( m_pPlayObject->overallTime() );

        m_length = timeO.seconds;
        m_pPlayerWidget->m_pSlider->setMaxValue( static_cast<int>( timeO.seconds ) );
    }
}


void PlayerApp::setupScrolltext()
{
    PlaylistItem *item = static_cast<PlaylistItem*>( m_pBrowserWin->m_pPlaylistWidget->currentTrack() );

    if ( item != NULL )
    {
        KFileMetaInfo metaInfo( item->url().path(), QString::null, KFileMetaInfo::Everything );

        if ( metaInfo.isValid() && !metaInfo.isEmpty() )
        {
            QString str, strNum;
            if ( metaInfo.item( "Artist" ).string() == "---" ||
                    metaInfo.item( "Title" ).string() == "---" )
            {
                str.append( item->text( 0 ) + " (" );
            }
            else
            {
                str.append( metaInfo.item( "Artist" ).string() + " - " );
                str.append( metaInfo.item( "Title" ).string() + " (" );
            }

            int totSeconds, totMinutes, totHours;
            totSeconds = ( m_length % 60 );
            totMinutes = ( m_length / 60 % 60 );
            totHours = ( m_length / 60 / 60 % 60 );
            if ( totHours )
            {
                strNum.setNum( totHours );
                str.append( strNum + ":" );
            }
            strNum.setNum( totMinutes );
            str.append( strNum + ":" );
            str.append( convertDigit( totSeconds ) + ")" );

            m_pPlayerWidget->setScroll( str,
                                        metaInfo.item( "Bitrate" ).string(),
                                        metaInfo.item( "Sample Rate" ).string() );
        }
        else
        {
            QString str( m_pPlayObject->mediaName() );

            if ( str.isEmpty() )
                m_pPlayerWidget->setScroll( item->text( 0 ), " ? ", " ? " );
            else
                m_pPlayerWidget->setScroll( str, " ? ", " ? " );
        }
    }
}


void PlayerApp::startXFade()
{
    kdDebug(DA_COMMON) << "void PlayerApp::startXFade()" << endl;

    m_XFadeRunning = true;

    if ( m_XFadeCurrent == "invalue1" )
        m_XFadeCurrent = "invalue2";
    else
        m_XFadeCurrent = "invalue1";

    m_pPlayObjectXFade = m_pPlayObject;
    m_pPlayObject = NULL;

    slotNext();
}


void PlayerApp::stopXFade()
{
    kdDebug(DA_COMMON) << "void PlayerApp::stopXFade()" << endl;

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
        v += 35;
    else
        v -= 35;
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

    m_pBrowserWin->m_pBrowserWidget->setPaletteBackgroundColor( m_optBrowserBgColor );
    m_pBrowserWin->m_pPlaylistWidget->setPaletteBackgroundColor( m_optBrowserBgColor );

    m_pBrowserWin->m_pBrowserLineEdit->setPaletteBackgroundColor( m_optBrowserBgColor );
    m_pBrowserWin->m_pBrowserLineEdit->setPaletteForegroundColor( m_optBrowserFgColor );
    
    m_pBrowserWin->m_pPlaylistLineEdit->setPaletteBackgroundColor( m_optBrowserBgColor );
    m_pBrowserWin->m_pPlaylistLineEdit->setPaletteForegroundColor( m_optBrowserFgColor );
    
    m_pBrowserWin->update();
    m_pBrowserWin->m_pBrowserWidget->triggerUpdate();
    m_pBrowserWin->m_pPlaylistWidget->triggerUpdate();
}


// SLOTS -----------------------------------------------------------------

void PlayerApp::slotPrev()
{
    if ( m_pBrowserWin->m_pPlaylistWidget->childCount() == 0 )
        return;

    QListViewItem *pItem = m_pBrowserWin->m_pPlaylistWidget->currentTrack(); //NULL is handled later

    if ( pItem != NULL ) //optRepeatTrack only applies to next since that is called automatically
    {
        //I've talked on a few channels, people hate it when media players restart the current track
        //first before going to the previous one (most players do this), so let's not do it!
        pItem = pItem->itemAbove();
    }

    //if pItem == NULL and bIsPlaying == TRUE  then we reached the beginning of the playlist
    //if pItem == NULL and bIsPlaying == FALSE then nothing is selected
    //FIXME always the chance that nothing is selected and bIsPlaying (eg deleted currentTrack from playlist)
    //      current behavior will stop playback, is this acceptable?
    //    * perhaps if user expects playback to continue we should continue playing from the beginning
    //    * perhaps we should stop playback dead if the playing item is removed from the playlist (this gives a more consistent interface) <berkus>: no no no when a track is deleted we should continue playing it (this is consistent with WinAmp classic and also i hate when it stops playing in the middle of the song when i'm trying to compose a new playlist already).
    //FIXME detection of empty playlist seems broken for above behavior
    //FIXME <markey> this comment is becoming a f*cking bible. may I add greetings to my grandma?

    if ( pItem == NULL )
    {
        if ( m_bIsPlaying && !!m_optRepeatPlaylist )
        {
            //no previous track, don't restart this track because we don't restart tracks when previous is pushed as a rule
            //also if we are repeating playlists then there is a previous track, i.e. the last track
            return ;
        }
        else
        {
            //nothing is selected and we are not playing, select last track
            pItem = m_pBrowserWin->m_pPlaylistWidget->lastItem();
        }
    }

    if ( pItem != NULL )
    {
        m_pBrowserWin->m_pPlaylistWidget->setCurrentTrack( pItem );
        m_pBrowserWin->m_pPlaylistWidget->unglowItems();
        m_pBrowserWin->m_pPlaylistWidget->ensureItemVisible( pItem );

        if ( m_bIsPlaying )
        {
            slotPlay();
        }
    }
}


void PlayerApp::slotPlay()
{
  /*
   //uncomment to allow switching on/off of viseffect dynamics (for testing)
   if( m_bIsPlaying )
     m_pPlayerWidget->m_pVis->m_dynamics = !m_pPlayerWidget->m_pVis->m_dynamics;
  */

    PlaylistItem* item = static_cast<PlaylistItem*>( m_pBrowserWin->m_pPlaylistWidget->currentTrack() );

    if ( item == NULL )
    {
        item = static_cast<PlaylistItem*>( m_pBrowserWin->m_pPlaylistWidget->firstChild() );
        PlaylistItem *tmpItem = item;

        if ( !tmpItem )
            return ;

        while ( tmpItem )
        {
            if ( tmpItem->isSelected() )
                break;
            tmpItem = static_cast<PlaylistItem*>( tmpItem->nextSibling() );
        }
        if ( tmpItem )                                //skip to the first selected item
            item = tmpItem;

        m_pBrowserWin->m_pPlaylistWidget->setCurrentTrack( item );
    }

    slotStop();

    m_pPlayerWidget->m_pButtonPlay->setOn( true ); //interface consistency
    KDE::PlayObjectFactory factory( m_Server );
    m_pPlayObject = factory.createPlayObject( item->url(), false ); //second parameter: create BUS(true/false)
    m_bIsPlaying = true;

    if ( m_pPlayObject == NULL )
    {
        kdDebug(DA_COMMON) << "Can't initialize Playobject. m_pPlayObject == NULL." << endl;
        slotNext();
        return ;
    }
    if ( m_pPlayObject->isNull() )
    {
        kdDebug(DA_COMMON) << "Can't initialize Playobject. m_pPlayObject->isNull()." << endl;
        delete m_pPlayObject;
        m_pPlayObject = NULL;
        slotNext();
        return ;
    }

    if ( m_pPlayObject->object().isNull() )
        connect( m_pPlayObject, SIGNAL( playObjectCreated() ), this, SLOT( slotConnectPlayObj() ) );
    else
        slotConnectPlayObj();

    m_pPlayObject->play();

    m_pBrowserWin->m_pPlaylistWidget->unglowItems();
    m_pBrowserWin->m_pPlaylistWidget->ensureItemVisible( item );

    if ( m_pPlayObject->stream() )
    {
        m_length = 0;
        m_pPlayerWidget->m_pSlider->setMaxValue( 0 );
        m_pPlayerWidget->setScroll( i18n( "Stream from: " ) + item->text( 0 ), "--", "--" );
    }

    m_pPlayerWidget->m_pSlider->setValue( 0 );
    m_pPlayerWidget->m_pSlider->setMinValue( 0 );

    m_pPlayerWidget->m_pButtonPause->setDown( false );
}



void PlayerApp::slotConnectPlayObj()
{
    if ( !m_pPlayObject->object().isNull() )
    {
        m_pPlayObject->object()._node()->start();

        Arts::connect( m_pPlayObject->object(), "left", m_XFade, ( m_XFadeCurrent + "_l" ).latin1() );
        Arts::connect( m_pPlayObject->object(), "right", m_XFade, ( m_XFadeCurrent + "_r" ).latin1() );
    }
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
    m_pPlayerWidget->m_pButtonPlay->setOn( false );

    if ( m_pPlayObject != NULL )
    {
        m_pPlayObject->halt();

        delete m_pPlayObject;
        m_pPlayObject = NULL;
    }

//     stopXFade();

    m_bIsPlaying = false;
    m_length = 0;
    m_pPlayerWidget->m_pButtonPause->setDown( false );
    m_pPlayerWidget->m_pSlider->setValue( 0 );
    m_pPlayerWidget->m_pSlider->setMinValue( 0 );
    m_pPlayerWidget->m_pSlider->setMaxValue( 0 );
    m_pPlayerWidget->setScroll( i18n( "   I feel empty   " ), " ", " " );
    m_pPlayerWidget->timeDisplay( false, 0, 0, 0 );

    delete m_pPlayerWidget->m_pPlayObjConfigWidget;
    m_pPlayerWidget->m_pPlayObjConfigWidget = NULL;
}


void PlayerApp::slotNext()
{
    QListViewItem *pItem = m_pBrowserWin->m_pPlaylistWidget->currentTrack(); //NULL is handled later

    // random mode
    if ( m_optRandomMode && m_pBrowserWin->m_pPlaylistWidget->childCount() > 3 )
    {
        QListViewItem *pNextItem;
        int number;

        do
        {
            number = KApplication::random() % m_pBrowserWin->m_pPlaylistWidget->childCount();
            pNextItem = m_pBrowserWin->m_pPlaylistWidget->itemAtIndex( number );
        }
        while ( pNextItem == pItem );    // try not to play same track twice in a row

        pItem = pNextItem;
    }
    else if ( !m_optRepeatTrack && pItem != NULL )
    {
        pItem = pItem->nextSibling();
    }

    //if pItem == NULL and bIsPlaying == TRUE  then we reached end of playlist
    //if pItem == NULL and bIsPlaying == FALSE then nothing is selected
    //FIXME always the chance that nothing is selected and bIsPlaying (eg deleted currentTrack from playlist)
    //      current behavior will stop playback, is this acceptable?
    //FIXME detection of empty playlist seems broken for above behavior

    if ( pItem == NULL )
    {
        if ( m_pBrowserWin->m_pPlaylistWidget->childCount() == 0 || ( !m_optRepeatPlaylist && m_bIsPlaying ) )
        {
            slotStop();
            return ;
        }
        else
        {
            pItem = m_pBrowserWin->m_pPlaylistWidget->firstChild();
        }
    }

    if ( pItem != NULL )
    {
        m_pBrowserWin->m_pPlaylistWidget->setCurrentTrack( pItem );
        m_pBrowserWin->m_pPlaylistWidget->unglowItems();
        m_pBrowserWin->m_pPlaylistWidget->ensureItemVisible( pItem );

        if ( m_bIsPlaying )
        {
            slotPlay();
        }
    }
}


bool PlayerApp::playObjectConfigurable()
{
    if ( m_pPlayObject && !m_pPlayerWidget->m_pPlayObjConfigWidget )
    {
        Arts::TraderQuery query;
        query.supports( "Interface", "Arts::GuiFactory" );
        query.supports( "CanCreate", pApp->m_pPlayObject->object()._interfaceName() );

        std::vector<Arts::TraderOffer> *queryResults = query.query();
        bool yes = queryResults->size();
        delete queryResults;

        return yes;
    }

    return false;
}


void PlayerApp::slotSavePlaylist()
{
    QString path = KFileDialog::getSaveFileName( m_pBrowserWin->m_pBrowserWidget->m_pDirLister->url().path(), "*.m3u" );

    if ( !path.isEmpty() )
    {
        if ( path.right( 4 ) != ".m3u" )
            path += ".m3u";

        m_pBrowserWin->m_pPlaylistWidget->saveM3u( path );
    }
}


void PlayerApp::slotClearPlaylistAsk()
{
    if ( m_optConfirmClear )
    {
        if ( KMessageBox::questionYesNo( 0, i18n( "Really clear playlist?" ) ) == KMessageBox::No )
            return ;
    }

    m_pBrowserWin->m_pPlaylistWidget->clear();
    m_pBrowserWin->m_pPlaylistWidget->writeUndo();
}


void PlayerApp::slotAddLocation()
{
    KURLRequesterDlg dlg( QString::null, 0, 0 );
    dlg.setCaption( makeStdCaption( i18n( "Enter file or URL" ) ) );
    dlg.urlRequester()->setMode( KFile::File | KFile::ExistingOnly );
    dlg.exec();

    KURL url = dlg.selectedURL();

    if ( !url.isEmpty() && url.isValid() )
    {
        if ( !m_pBrowserWin->m_pPlaylistWidget->loadPlaylist( url, 0 ) )
        {
            if ( isFileValid( url ) )
                new PlaylistItem( m_pBrowserWin->m_pPlaylistWidget, url );
        }
    }
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
        if ( m_scopeActive )
        {
            m_Scope.stop();
            m_scopeActive = false;
        }
        return;
    }

    if ( ( m_length == 0 ) && ( !m_pPlayObject->stream() ) )
    {
        getTrackLength();
        setupScrolltext();
    }

    if ( m_bSliderIsPressed )
        return;
    if ( !m_bIsPlaying )
        return;

    Arts::poTime timeC( m_pPlayObject->currentTime() );
    m_pPlayerWidget->m_pSlider->setValue( static_cast<int>( timeC.seconds ) );

    // <Draw TimeDisplay>
    if ( m_pPlayerWidget->isVisible() )
    {
        int seconds;
        if ( m_optTimeDisplayRemaining )
            seconds = m_length - timeC.seconds;
        else
            seconds = timeC.seconds;

        m_pPlayerWidget->timeDisplay( m_optTimeDisplayRemaining, seconds / 60 / 60 % 60,
                                      seconds / 60 % 60, seconds % 60 );
    }
    // </Draw TimeDisplay>

    // <Crossfading>
    if ( ( m_optXFade ) &&
         ( !m_pPlayObject->stream() ) &&
         ( !m_XFadeRunning ) &&
         ( m_length * 1000 - ( timeC.seconds * 1000 + timeC.ms ) < m_optXFadeLength )  )
    {
        startXFade();
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

        m_XFade.percentage( m_XFadeValue );

        if( m_XFadeValue == 0.0 || m_XFadeValue == 1.0 )
            stopXFade();
    }
    // </Crossfading>

    // check if track has ended
    if ( m_pPlayObject->state() == Arts::posIdle )
    {
        slotNext();
        return;
    }

    if ( m_pPlayObject->state() == Arts::posPlaying )
    {
        if ( !m_scopeActive )
        {
            m_Scope.start();
            m_scopeActive = true;
        }
    }
    else
    {
        if ( m_scopeActive )
        {
            m_Scope.stop();
            m_scopeActive = false;
        }
    }
}


void PlayerApp::slotAnimTimer()
{
    if ( m_pPlayerWidget->isVisible() )
    {
        m_pPlayerWidget->drawScroll();

        if ( m_scopeActive )
        {
            std::vector<float> *pScopeVector = m_Scope.scope();
            m_pPlayerWidget->m_pVis->drawAnalyzer( pScopeVector );
        }
        else
            m_pPlayerWidget->m_pVis->drawAnalyzer( NULL );
    }
}


void PlayerApp::slotItemDoubleClicked( QListViewItem *item )
{
   if (item) {
    m_pBrowserWin->m_pPlaylistWidget->setCurrentTrack( static_cast<PlaylistItem*>( item ) );
    slotPlay();
   }
}


void PlayerApp::slotShowAbout()
{
    KAboutApplication dia;
    dia.exec();

//    FIXME would be nice to get the amarok logo from the site in here.
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


void PlayerApp::slotPlaylistHide()
{
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
    {
        m_pEffectWidget = new EffectWidget( m_pPlayerWidget );
    }

    m_pEffectWidget->show();
    return ;
}


void PlayerApp::slotShowTip()
{
    KTipDialog::showTip( "amarok/data/startupTip.txt", true );
}


void PlayerApp::slotSetRepeatTrack()
{
    int id = m_pPlayerWidget->m_IdRepeatTrack ;

    if ( m_pPlayerWidget->m_pPopupMenu->isItemChecked( id ) )
    {
        m_optRepeatTrack = false;
        m_pPlayerWidget->m_pPopupMenu->setItemChecked( id, false );
    }

    else
    {
        m_optRepeatTrack = true;
        m_pPlayerWidget->m_pPopupMenu->setItemChecked( id, true );
    }
}


void PlayerApp::slotSetRepeatPlaylist()
{
    int id = m_pPlayerWidget->m_IdRepeatPlaylist;

    if ( m_pPlayerWidget->m_pPopupMenu->isItemChecked( id ) )
    {
        m_optRepeatPlaylist = false;
        m_pPlayerWidget->m_pPopupMenu->setItemChecked( id, false );
    }

    else
    {
        m_optRepeatPlaylist = true;
        m_pPlayerWidget->m_pPopupMenu->setItemChecked( id, true );
    }
}


void PlayerApp::slotSetRandomMode()
{
    int id = m_pPlayerWidget->m_IdRandomMode;

    if ( m_pPlayerWidget->m_pPopupMenu->isItemChecked( id ) )
    {
        m_optRandomMode = false;
        m_pPlayerWidget->m_pPopupMenu->setItemChecked( id, false );
    }

    else
    {
        m_optRandomMode = true;
        m_pPlayerWidget->m_pPopupMenu->setItemChecked( id, true );
    }
}


void PlayerApp::slotShowHelp()
{
    KApplication::KApp->invokeHelp( QString::null, "amarok" );
}


void PlayerApp::slotWidgetMinimized()
{
    if ( m_optHidePlaylistWindow && m_pPlayerWidget->m_pButtonPl->isOn() )
        m_pBrowserWin->hide();
}


void PlayerApp::slotWidgetRestored()
{
    if ( m_optHidePlaylistWindow && m_pPlayerWidget->m_pButtonPl->isOn() )
        m_pBrowserWin->show();
}


#include "playerapp.moc"
