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

#include "playerapp.h"
#include "playerwidget.h"
#include "browserwin.h"
#include "browserwidget.h"
#include "playlistwidget.h"
#include "playlistitem.h"
#include "viswidget.h"
#include "expandbutton.h"
#include "Options1.h"
#include "effectwidget.h"
#include "amarokarts/amarokarts.h"

#include <vector>
#include <string>

#include <kaboutapplication.h>
#include <kaction.h>
#include <kapp.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <kdirlister.h>
#include <kfile.h>
#include <kfiledialog.h>
#include <kfileitem.h>
#include <kglobalaccel.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kshortcut.h>
#include <kstandarddirs.h>
#include <ktip.h>
#include <kuniqueapplication.h>
#include <kurl.h>
#include <kurlrequester.h>
#include <kurlrequesterdlg.h>

#include <kio/netaccess.h>

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

#include <qpopupmenu.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdialog.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qpoint.h>
#include <qsize.h>
#include <qstring.h>
#include <qtimer.h>
#include <qvaluelist.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/wait.h>


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
        m_XFadeCurrent( "invalue1" ),
        m_XFadeValue( 1.0 )
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

    connect( this, SIGNAL( sigplay() ), this, SLOT( slotPlay() ) );
    connect( this, SIGNAL( saveYourself() ), this, SLOT( saveSessionState() ) );
    connect( this, SIGNAL( sigShowTrayIcon( bool ) ), m_pPlayerWidget, SLOT( slotUpdateTrayIcon( bool ) ) );

    connect( m_pPlayerWidget, SIGNAL( sigMinimized() ), this, SLOT( slotWidgetMinimized() ) );
    connect( m_pPlayerWidget, SIGNAL( sigRestored() ), this, SLOT( slotWidgetRestored() ) );

    m_pMainTimer = new QTimer( this );
    connect( m_pMainTimer, SIGNAL( timeout() ), this, SLOT( slotMainTimer() ) );

    m_pMainTimer->start( 150 );

    m_pAnimTimer = new QTimer( this );
    connect( m_pAnimTimer, SIGNAL( timeout() ), this, SLOT( slotAnimTimer() ) );
    m_pAnimTimer->start( 30 );

    m_pPlayerWidget->show();

    KTipDialog::showTip( "amarok/data/startupTip.txt", false );
}


PlayerApp::~PlayerApp()
{
    slotStop();

    killTimers();
    saveConfig();

    delete m_pEffectWidget;
    delete m_pPlayerWidget;

    m_XFadeL = Arts::Synth_XFADE::null();
    m_XFadeR = Arts::Synth_XFADE::null();
    m_synthData = Arts::Synth_DATA::null();
    m_Scope = Amarok::WinSkinFFT::null();
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

    if ( !playlistUrl.isEmpty() )            //playlist
    {
        slotClearPlaylist();
        loadPlaylist( KCmdLineArgs::makeURL( playlistUrl ).path(), 0 );
    }

    if ( args->count() > 0 )
    {
        if ( args->isSet( "e" ) )            //enqueue
        {
            for ( int i = 0; i < args->count(); i++ )
            {
                if ( !loadPlaylist( args->url( i ), m_pBrowserWin->m_pPlaylistWidget->lastItem() ) )
                {
                    if ( m_pBrowserWin->isFileValid( args->url( i ) ) )
                        m_pBrowserWin->m_pPlaylistWidget->addItem( ( PlaylistItem* ) 1, args->url( i ) );
                }
            }
        }
        else                              //URLs
        {
            slotClearPlaylist();

            for ( int i = 0; i < args->count(); i++ )
            {
                if ( !loadPlaylist( args->url( i ), 0 ) )
                {
                    if ( m_pBrowserWin->isFileValid( args->url( i ) ) )
                        m_pBrowserWin->m_pPlaylistWidget->addItem( 0, args->url( i ) );
                }
            }
            slotPlay();
        }
    }

    if ( args->isSet( "r" ) )                //rewind
        pApp->slotPrev();
    if ( args->isSet( "f" ) )                //forward
        pApp->slotNext();
    if ( args->isSet( "p" ) )                //play
        pApp->slotPlay();
    if ( args->isSet( "s" ) )                //stop
        pApp->slotStop();

    return KUniqueApplication::newInstance();
}


void PlayerApp::restore()
{
    //attempt to restore previous session
    KConfig * config = sessionConfig();

    KURL url = config->readEntry( "track" );
    int seconds = config->readNumEntry( "position" );

    //FIXME this is duplicated in slotAddLocation, reduce LOC

    if ( !url.isEmpty() && url.isValid() )
    {
        if ( m_pBrowserWin->isFileValid( url ) )
        {
            PlaylistItem * item = new PlaylistItem( m_pBrowserWin->m_pPlaylistWidget, url );

            m_pBrowserWin->m_pPlaylistWidget->setCurrentTrack( item );
            slotPlay();

            //FIXME I just copied this code, do I need all these properties?
            Arts::poTime time;
            time.ms = 0;
            time.seconds = seconds;
            time.custom = 0;
            time.customUnit = std::string();

          // try to fix a crash on session restore when there's nothing to restore
          if (m_pPlayObject && !m_pPlayObject->isNull())
            m_pPlayObject->seek( time );
        }
    }
}


//session management
void PlayerApp::saveSessionState()
{
    KConfig * config = sessionConfig();

    Arts::poTime timeC( m_pPlayObject->currentTime() );

    config->writeEntry( "track", static_cast<PlaylistItem*>( m_pBrowserWin->m_pPlaylistWidget->currentTrack() ) ->url().url() );
    config->writeEntry( "position", timeC.seconds );
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

    m_synthData = Arts::DynamicCast( m_Server.createObject( "Arts::Synth_DATA" ) );
    m_synthData.value( m_XFadeValue );
    m_synthData.start();

    m_XFadeL = Arts::DynamicCast( m_Server.createObject( "Arts::Synth_XFADE" ) );
    m_XFadeR = Arts::DynamicCast( m_Server.createObject( "Arts::Synth_XFADE" ) );
    m_XFadeL.start();
    m_XFadeR.start();

    m_globalEffectStack = Arts::DynamicCast( m_Server.createObject( "Arts::StereoEffectStack" ) );
    m_globalEffectStack.start();

    m_effectStack = Arts::DynamicCast( m_Server.createObject( "Arts::StereoEffectStack" ) );
    m_effectStack.start();
    long id = m_globalEffectStack.insertBottom( m_effectStack, "Effect Stack" );

    Arts::connect( m_synthData, std::string( "outvalue" ), m_XFadeL, std::string( "percentage" ) );
    Arts::connect( m_synthData, std::string( "outvalue" ), m_XFadeR, std::string( "percentage" ) );

    Arts::connect( m_XFadeL, std::string( "outvalue" ), m_globalEffectStack, std::string( "inleft" ) );
    Arts::connect( m_XFadeR, std::string( "outvalue" ), m_globalEffectStack, std::string( "inright" ) );

    Arts::connect( m_globalEffectStack, m_amanPlay );
}


void PlayerApp::initPlayerWidget()
{
    //TEST
    kdDebug() << "begin PlayerApp::initPlayerWidget()" << endl;

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
    kdDebug() << "end PlayerApp::initPlayerWidget()" << endl;
}


void PlayerApp::initMixer()
{
    //TEST
    kdDebug() << "begin PlayerApp::initMixer()" << endl;

    if ( initMixerHW() )
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
        long id = m_globalEffectStack.insertBottom( m_volumeControl, "Volume Control" );

        //TEST
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


bool PlayerApp::initScope()
{
    //TEST
    kdDebug() << "begin PlayerApp::initScope()" << endl;

    m_Scope = Arts::DynamicCast( m_Server.createObject( "Amarok::WinSkinFFT" ) );

    if ( ( m_Scope ).isNull() )
    {
        kdDebug() << "*m_Scope.isNull()!" << endl;
        return false;
    }

    m_scopeActive = false;
    long id = m_globalEffectStack.insertBottom( m_Scope, "Analyzer" );

    //TEST
    kdDebug() << "end PlayerApp::initScope()" << endl;
    return true;
}


void PlayerApp::initBrowserWin()
{
    //TEST
    kdDebug() << "begin PlayerApp::initBrowserWin()" << endl;

    m_pBrowserWin = new BrowserWin();

    connect( m_pBrowserWin->m_pButtonAdd, SIGNAL( clicked() ),
             this, SLOT( slotAddLocation() ) );

    connect( m_pBrowserWin->m_pButtonSave, SIGNAL( clicked() ),
             this, SLOT( slotSavePlaylist() ) );

    connect( m_pBrowserWin->m_pButtonClear, SIGNAL( clicked() ),
             this, SLOT( slotClearPlaylistAsk() ) );

    connect( m_pBrowserWin->m_pButtonUndo, SIGNAL( clicked() ),
             this, SLOT( slotUndoPlaylist() ) );

    connect( m_pBrowserWin->m_pButtonRedo, SIGNAL( clicked() ),
             this, SLOT( slotRedoPlaylist() ) );

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
    kdDebug() << "end PlayerApp::initBrowserWin()" << endl;
}


// METHODS --------------------------------------------------------------------------

bool PlayerApp::loadPlaylist( KURL url, QListViewItem *destination )
{
    bool success = false;
    QString tmpFile;
    PlaylistItem *pCurr = static_cast<PlaylistItem*>( destination );

    if ( url.path().lower().endsWith( ".m3u" ) )
    {
        if ( url.isLocalFile() )
            tmpFile = url.path();
        else
            KIO::NetAccess::download( url, tmpFile );

        QFile file( tmpFile );
        if ( file.open( IO_ReadOnly ) )
        {
            QTextStream stream( &file );

            while ( QString str = stream.readLine() )
            {
                if ( !str.startsWith( "#" ) )
                {
                    pCurr = m_pBrowserWin->m_pPlaylistWidget->addItem( pCurr, str );
                }
            }
            file.close();
            success = true;
        }
    }
    if ( url.path().lower().endsWith( ".pls" ) )
    {
        if ( url.isLocalFile() )
            tmpFile = url.path();
        else
            KIO::NetAccess::download( url, tmpFile );

        QFile file( tmpFile );
        if ( file.open( IO_ReadOnly ) )
        {
            QTextStream stream( &file );

            while ( QString str = stream.readLine() )
            {
                if ( str.startsWith( "File" ) )
                {
                    pCurr = m_pBrowserWin->m_pPlaylistWidget->addItem( pCurr, str.section( "=", -1 ) );
                    str = stream.readLine();

                    if ( str.startsWith( "Title" ) )
                        pCurr->setText( 0, str.section( "=", -1 ) );
                }
            }
            file.close();
            success = true;
        }
    }
    KIO::NetAccess::removeTempFile( tmpFile );
    return success;
}


void PlayerApp::saveM3u( QString fileName )
{
    QFile file( fileName );

    if ( !file.open( IO_WriteOnly ) )
        return ;

    PlaylistItem* item = static_cast<PlaylistItem*>( m_pBrowserWin->m_pPlaylistWidget->firstChild() );
    QTextStream stream( &file );
    stream << "#EXTM3U\n";

    while ( item != NULL )
    {
        if ( item->url().protocol() == "file" )
            stream << item->url().path();
        else
            stream << item->url().url();

        stream << "\n";
        item = static_cast<PlaylistItem*>( item->nextSibling() );
    }

    file.close();
}


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
    m_pConfig->writeEntry( "Hide Playlist Window", m_optHidePlaylistWindow );

    //store current item
    PlaylistItem *item = static_cast<PlaylistItem*>( m_pBrowserWin->m_pPlaylistWidget->currentTrack() );
    if ( item != NULL )
    {
        if ( item->url().protocol() == "file" )
            m_pConfig->writeEntry( "CurrentSelection", item->url().path() );
        else
            m_pConfig->writeEntry( "CurrentSelection", item->url().url() );
    }

    saveM3u( kapp->dirs() ->saveLocation( "data", kapp->instanceName() + "/" ) + "current.m3u" );
}


void PlayerApp::readConfig()
{
    //TEST
    kdDebug() << "begin PlayerApp::readConfig()" << endl;

    // FIXME: ok, the compiler warning is gone now. but the result is the same: those variables are
    //        still temporary. so what have we gained? frankly, why does KConfig take a pointer here,
    //        anyway? I've looked at KConfig sources, and it seems, it gets just dereferened once and
    //        that's it. so &( QPoint( 0, 0 ) ); would have done the same. what do you guys think?
    QPoint pointZero = QPoint( 0, 0 );
    QSize arbitrarySize = QSize ( 600, 450 );

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
    m_optReadMetaInfo = m_pConfig->readBoolEntry( "Show MetaInfo", false );
    m_optShowTrayIcon = m_pConfig->readBoolEntry( "Show Tray Icon", true );
    m_optXFade = m_pConfig->readBoolEntry( "Crossfading", true );
    m_optHidePlaylistWindow = m_pConfig->readBoolEntry( "Hide Playlist Window", true );

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

    slotClearPlaylist();
    loadPlaylist( kapp->dirs() ->saveLocation( "data", kapp->instanceName() + "/" ) + "current.m3u", 0 );

    KURL currentlyPlaying = m_pConfig->readEntry( "CurrentSelection" );

    kdDebug() << "Attempting to select: " << currentlyPlaying.path() << endl;

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


    m_pGlobalAccel->insert( "add", "Add Location", 0, CTRL + SHIFT + Key_A, 0, this, SLOT( slotAddLocation() ), true, true );
    m_pGlobalAccel->insert( "play", "Play", 0, CTRL + SHIFT + Key_P, 0, this, SLOT( slotPlay() ), true, true );
    m_pGlobalAccel->insert( "stop", "Stop", 0, CTRL + SHIFT + Key_S, 0, this, SLOT( slotStop() ), true, true );
    m_pGlobalAccel->insert( "next", "Next Track", 0, CTRL + SHIFT + Key_N, 0, this, SLOT( slotNext() ), true, true );
    m_pGlobalAccel->insert( "prev", "Previous Track", 0, CTRL + SHIFT + Key_R, 0, this, SLOT( slotPrev() ), true, true );
    m_pGlobalAccel->readSettings( m_pConfig );
    m_pGlobalAccel->updateConnections();

    m_pPlayerWidget->m_pActionCollection->readShortcutSettings( QString::null, m_pConfig );

    new KAction( "Copy Current Title to Clipboard", CTRL + Key_C, m_pPlayerWidget, SLOT( slotCopyClipboard() ), m_pPlayerWidget->m_pActionCollection, "copy_clipboard" );

    m_pBrowserWin->m_pActionCollection->readShortcutSettings( QString::null, m_pConfig );
    new KAction( "Go one item up", Key_Up, m_pBrowserWin, SLOT( slotKeyUp() ), m_pBrowserWin->m_pActionCollection, "up" );
    new KAction( "Go one item down", Key_Down, m_pBrowserWin, SLOT( slotKeyDown() ), m_pBrowserWin->m_pActionCollection, "down" );
    new KAction( "Go one page up", Key_PageUp, m_pBrowserWin, SLOT( slotKeyPageUp() ), m_pBrowserWin->m_pActionCollection, "page_up" );
    new KAction( "Go one page down", Key_PageDown, m_pBrowserWin, SLOT( slotKeyPageDown() ), m_pBrowserWin->m_pActionCollection, "page_down" );
    new KAction( "Enter item", SHIFT + Key_Return, m_pBrowserWin, SLOT( slotKeyEnter() ), m_pBrowserWin->m_pActionCollection, "enter" );
    new KAction( "Delete item", SHIFT + Key_Delete, m_pBrowserWin, SLOT( slotKeyDelete() ), m_pBrowserWin->m_pActionCollection, "delete" );

    //TEST
    kdDebug() << "end PlayerApp::readConfig()" << endl;
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
    PlaylistItem * item = static_cast<PlaylistItem*>( m_pBrowserWin->m_pPlaylistWidget->currentTrack() );

    if ( item == NULL )
        return ;
    // let aRts calculate length
    Arts::poTime timeO( m_pPlayObject->overallTime() );
    m_length = timeO.seconds;
    m_pPlayerWidget->m_pSlider->setMaxValue( static_cast<int>( timeO.seconds ) );

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


void PlayerApp::toggleXFade( bool on )
{
    if ( on )
    {
        m_optXFade = true;
    }

    else
    {
        m_optXFade = false;
    }
}


void PlayerApp::startXFade()
{
    m_XFadeRunning = true;

    if ( m_XFadeCurrent == "invalue1" )
        m_XFadeCurrent = "invalue2";
    else
        m_XFadeCurrent = "invalue1";

    m_pPlayObjectXFade = m_pPlayObject;
    m_pPlayObject = NULL;
    m_length = 0;

    delete m_pPlayerWidget->m_pPlayObjConfigWidget;
    m_pPlayerWidget->m_pPlayObjConfigWidget = NULL;

    slotNext();
}


void PlayerApp::stopXFade()
{
    if ( m_pPlayObjectXFade != NULL )
    {
        m_pPlayObjectXFade->halt();
        m_pPlayObjectXFade->object()._node()->stop();

        delete m_pPlayObjectXFade;
        m_pPlayObjectXFade = NULL;

        m_XFadeRunning = false;
    }
}


// SLOTS -----------------------------------------------------------------

void PlayerApp::slotPrev()
{
    // do nothing when list is empty
    if ( m_pBrowserWin->m_pPlaylistWidget->childCount() == 0 )
    {
        m_pBrowserWin->m_pPlaylistWidget->setCurrentTrack( NULL );
        return ;
    }

    QListViewItem *pItem = m_pBrowserWin->m_pPlaylistWidget->currentTrack();

    if ( pItem == NULL )
        return ;

    pItem = pItem->itemAbove();

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
    //Markey: I moved this function above the item determination function below
    // although I'm not 100% sure it was a good idea as I can't tell if it is necessary to setCurrentTrack()
    // please check!

    if ( m_bIsPlaying && !m_pPlayerWidget->m_pButtonPlay->isOn() )    //bit of a hack really
    {
        slotStop();
        return ;
    }


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
        if ( tmpItem )                               //skip to the first selected item
            item = tmpItem;
    }

    m_pBrowserWin->m_pPlaylistWidget->setCurrentTrack( item );

    m_pPlayerWidget->m_pButtonPlay->setOn( true ); //interface consistency

    m_length = 0;
    KDE::PlayObjectFactory factory( m_Server );
    factory.setAllowStreaming( true );
    m_pPlayObject = NULL;
    //second parameter: create BUS(true/false)
    m_pPlayObject = factory.createPlayObject( item->url(), false );
    m_bIsPlaying = true;

    if ( m_pPlayObject == NULL )
    {
        kdDebug() << "Can't initialize Playobject. m_pPlayObject == NULL." << endl;
        slotNext();
        return ;
    }
    if ( m_pPlayObject->isNull() )
    {
        kdDebug() << "Can't initialize Playobject. m_pPlayObject->isNull()." << endl;
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
        m_pPlayerWidget->timeDisplay( false, 0, 0, 0 );

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

        Arts::connect( m_pPlayObject->object(), std::string( "left" ), m_XFadeL, m_XFadeCurrent.latin1() );
        Arts::connect( m_pPlayObject->object(), std::string( "right" ), m_XFadeR, m_XFadeCurrent.latin1() );

/*        Arts::connect( m_pPlayObject->object(), std::string( "left" ), m_globalEffectStack, std::string( "inleft" ) );
        Arts::connect( m_pPlayObject->object(), std::string( "right" ), m_globalEffectStack, std::string( "inright" ) );*/
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
    if ( m_bIsPlaying )
    {
        m_pPlayerWidget->m_pButtonPlay->setOn( false );

        if ( m_pPlayObject != NULL )
        {
            m_pPlayObject->halt();
            m_pPlayObject->object()._node() ->stop();

            delete m_pPlayObject;
            m_pPlayObject = NULL;
        }

        if ( m_pPlayObjectXFade != NULL )
        {
            m_pPlayObjectXFade->halt();
            m_pPlayObjectXFade->object()._node() ->stop();

            delete m_pPlayObjectXFade;
            m_pPlayObjectXFade = NULL;
        }

        m_bIsPlaying = false;
        m_length = 0;
        m_pPlayerWidget->m_pButtonPause->setDown( false );
        m_pPlayerWidget->m_pSlider->setValue( 0 );
        m_pPlayerWidget->m_pSlider->setMinValue( 0 );
        m_pPlayerWidget->m_pSlider->setMaxValue( 0 );
        m_pPlayerWidget->setScroll( i18n( "no file loaded" ), " ", " " );
        m_pPlayerWidget->timeDisplay( false, 0, 0, 0 );

        delete m_pPlayerWidget->m_pPlayObjConfigWidget;
        m_pPlayerWidget->m_pPlayObjConfigWidget = NULL;
    }
}


void PlayerApp::slotNext()
{
    QListViewItem * pItem = m_pBrowserWin->m_pPlaylistWidget->currentTrack();

    if ( pItem == NULL )
    {
        slotStop();
        return ;
    }

    if ( !m_optRepeatTrack )
    {
        pItem = pItem->nextSibling();
    }
    //     else if ( m_optRandomMode )
    //     {
    //     }

    if ( pItem == NULL )
    {
        //do nothing when list is empty
        if ( m_pBrowserWin->m_pPlaylistWidget->childCount() == 0 || !m_optRepeatPlaylist )
        {
            m_pBrowserWin->m_pPlaylistWidget->setCurrentTrack( NULL );
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


void PlayerApp::slotLoadPlaylist()
{
    KURLRequesterDlg dlg( QString::null, 0, 0 );
    dlg.setCaption( makeStdCaption( i18n( "Enter file or URL" ) ) );
    dlg.setIcon( icon() );
    dlg.fileDialog() ->setFilter( "*.m3u *.pls *.M3U *.PLS|Playlist Files" );
    dlg.urlRequester() ->setMode( KFile::File | KFile::ExistingOnly );
    dlg.exec();

    KURL url = dlg.selectedURL();

    if ( !url.isEmpty() && url.isValid() )
    {
        slotClearPlaylist();
        loadPlaylist( url, 0 );
    }
}


void PlayerApp::slotSavePlaylist()
{
    QString path = KFileDialog::getSaveFileName( m_pBrowserWin->m_pBrowserWidget->m_pDirLister->url().path(), "*.m3u" );

    if ( !path.isEmpty() )
    {
        if ( path.right( 4 ) != ".m3u" )
            path += ".m3u";

        saveM3u( path );
    }
}


void PlayerApp::slotClearPlaylist()
{
    m_pBrowserWin->m_pPlaylistWidget->clear();
    m_pBrowserWin->m_pPlaylistWidget->setCurrentTrack( NULL );
    m_pBrowserWin->m_pPlaylistLineEdit->clear();
}


void PlayerApp::slotClearPlaylistAsk()
{
    if ( m_optConfirmClear )
    {
        if ( KMessageBox::questionYesNo( 0, i18n( "Really clear playlist?" ) ) == KMessageBox::No )
            return ;
    }

    slotClearPlaylist();
}


void PlayerApp::slotUndoPlaylist()
{
    kdDebug() << "PlayerApp::slotUndoPlaylist()" << endl;
    KMessageBox::sorry( 0, "Not yet implemented. /me gives user a chocolate cookie." );
}


void PlayerApp::slotRedoPlaylist()
{
    kdDebug() << "PlayerApp::slotRedoPlaylist()" << endl;
    KMessageBox::sorry( 0, "Not yet implemented. /me gives user a chocolate cookie." );
}


void PlayerApp::slotAddLocation()
{
    KURLRequesterDlg dlg( QString::null, 0, 0 );
    dlg.setCaption( makeStdCaption( i18n( "Enter file or URL" ) ) );
    dlg.setIcon( icon() );
    dlg.urlRequester() ->setMode( KFile::File | KFile::ExistingOnly );
    dlg.exec();

    KURL url = dlg.selectedURL();

    if ( !url.isEmpty() && url.isValid() )
    {
        if ( !loadPlaylist( url, 0 ) )
        {
            if ( m_pBrowserWin->isFileValid( url ) )
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
    if ( m_pPlayerWidget->isVisible() )
    {
        if ( m_optTimeDisplayRemaining )
        {
            int sliderSeconds = m_length - m_pPlayerWidget->m_pSlider->value();
            m_pPlayerWidget->timeDisplay( true, sliderSeconds / 60 / 60 % 60, sliderSeconds / 60 % 60, sliderSeconds % 60 );
        }
        else
        {
            int sliderSeconds = m_pPlayerWidget->m_pSlider->value();
            m_pPlayerWidget->timeDisplay( false, sliderSeconds / 60 / 60 % 60, sliderSeconds / 60 % 60, sliderSeconds % 60 );
        }
    }

    if ( m_pPlayObject == NULL || m_pPlayObject->isNull() )
    {
        if ( m_scopeActive )
        {
            m_Scope.stop();
            m_scopeActive = false;
        }

        return ;
    }

    if ( ( m_length == 0 ) && ( !m_pPlayObject->stream() ) )
        getTrackLength();

    if ( m_bSliderIsPressed )
        return ;
    if ( !m_bIsPlaying )
        return ;

    Arts::poTime timeC( m_pPlayObject->currentTime() );
    m_pPlayerWidget->m_pSlider->setValue( static_cast<int>( timeC.seconds ) );

    // <Crossfading>
    if ( m_optXFade and m_length - timeC.seconds < 4 and not m_XFadeRunning )
    {
        startXFade();
    }
    if ( m_XFadeRunning )
    {
        if ( m_XFadeCurrent == "invalue2" )
        {
            m_XFadeValue -= 0.04;
            if ( m_XFadeValue < 0.0 )
            {
                stopXFade();
                m_XFadeValue = 0.0;
            }
        }
        else
            m_XFadeValue += 0.04;
            if ( m_XFadeValue > 1.0 )
            {
                stopXFade();
                m_XFadeValue = 1.0;
            }

        m_synthData.value( m_XFadeValue );
    }
    // </Crossfading>

    // check if track has ended
    if ( m_pPlayObject->state() == Arts::posIdle )
    {
        slotNext();
        return ;
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

            if ( pScopeVector->size() != 0 )
                m_pPlayerWidget->m_pVis->drawAnalyzer( pScopeVector );
            else
                m_pPlayerWidget->m_pVis->drawAnalyzer( NULL );
        }
        else
            m_pPlayerWidget->m_pVis->drawAnalyzer( NULL );
    }
}


void PlayerApp::slotItemDoubleClicked( QListViewItem *item )
{
    m_pBrowserWin->m_pPlaylistWidget->setCurrentTrack( static_cast<PlaylistItem*>( item ) );
    slotPlay();
}


void PlayerApp::slotShowAbout()
{
    KAboutApplication dia;
    dia.exec();
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
    KDialogBase * pDia = new KDialogBase( KDialogBase::IconList, i18n( "Options" ),
                                          KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok );

    QFrame *frame;
    KIconLoader iconLoader;

    frame = pDia->addPage( i18n( "General" ) , i18n( "Configure general options" ),
                           iconLoader.loadIcon( "misc", KIcon::NoGroup, KIcon::SizeMedium ) );

    Options1 *opt1 = new Options1( frame );
    opt1->checkBox1->setChecked( m_optSavePlaylist );
    opt1->checkBox7->setChecked( m_optConfirmClear );
    opt1->checkBox6->setChecked( m_optConfirmExit );
    opt1->checkBox4->setChecked( m_optReadMetaInfo );
    opt1->checkBox3->setChecked( m_optShowTrayIcon );
    opt1->checkBox5->setChecked( m_optHidePlaylistWindow );

    if ( m_optDropMode == "Ask" )
        opt1->comboBox1->setCurrentItem( 0 );
    if ( m_optDropMode == "Recursively" )
        opt1->comboBox1->setCurrentItem( 1 );
    if ( m_optDropMode == "NonRecursively" )
        opt1->comboBox1->setCurrentItem( 2 );

    //  frame = pDia->addVBoxPage( QString( "Sound" ) , QString( "Configure sound options" ),
    //                             iconLoader.loadIcon( "sound", KIcon::NoGroup, KIcon::SizeMedium ) );

    pDia->resize( 500, 390 );

    if ( pDia->exec() == QDialog::Accepted )
    {
        if ( opt1->checkBox1->isChecked() )
            m_optSavePlaylist = true;
        else
            m_optSavePlaylist = false;

        if ( opt1->checkBox7->isChecked() )
            m_optConfirmClear = true;
        else
            m_optConfirmClear = false;

        if ( opt1->checkBox6->isChecked() )
            m_optConfirmExit = true;
        else
            m_optConfirmExit = false;

        if ( opt1->checkBox4->isChecked() )
            m_optReadMetaInfo = true;
        else
            m_optReadMetaInfo = false;

        if ( opt1->checkBox3->isChecked() )
            m_optShowTrayIcon = true;
        else
            m_optShowTrayIcon = false;

        if ( opt1->checkBox5->isChecked() )
            m_optHidePlaylistWindow = true;
        else
            m_optHidePlaylistWindow = false;

        switch ( opt1->comboBox1->currentItem() )
        {
            case 0:
                m_optDropMode = "Ask";
                break;
            case 1:
                m_optDropMode = "Recursively";
                break;
            case 2:
                m_optDropMode = "NonRecursively";
                break;
        }
        emit sigShowTrayIcon( m_optShowTrayIcon );
    }
    delete pDia;
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


void PlayerApp::slotShowHelp()
{
    KApplication::KApp->invokeHelp( QString::null, "amarok" );
}

void PlayerApp::slotWidgetMinimized()
{
   if (m_optHidePlaylistWindow && m_pPlayerWidget->m_pButtonPl->isOn())
      m_pBrowserWin->hide();
}

void PlayerApp::slotWidgetRestored()
{
   if (m_optHidePlaylistWindow && m_pPlayerWidget->m_pButtonPl->isOn())
      m_pBrowserWin->show();
}

#include "playerapp.moc"
