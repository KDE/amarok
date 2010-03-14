/****************************************************************************************
* Copyright (c) 2010 Nathan Sala <sala.nathan@gmail.com>                               *
*                                                                                      *
* This program is free software; you can redistribute it and/or modify it under        *
* the terms of the GNU General Public License as published by the Free Software        *
* Foundation; either version 2 of the License, or (at your option) any later           *
* version.                                                                             *
*                                                                                      *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
* PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
*                                                                                      *
* You should have received a copy of the GNU General Public License along with         *
* this program.  If not, see <http://www.gnu.org/licenses/>.                           *
****************************************************************************************/

#include "TestDataEngine.h"
#include "/src/Amarok.h"

#include <KCmdLineArgs>
#include <QTextCodec>
#include <KSplashScreen>
#include <KGlobal>

TestDataEngine::TestDataEngine()
{
    const KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();
    const bool restoreSession = args->count() == 0 || args->isSet( "append" ) || args->isSet( "queue" )
                                || Amarok::config().readEntry( "AppendAsDefault", false );

    QTextCodec* utf8codec = QTextCodec::codecForName( "UTF-8" );
    QTextCodec::setCodecForCStrings( utf8codec ); //We need this to make CollectionViewItem showing the right characters.

    new Amarok::DefaultApplicationController();
    Amarok::Components::applicationController()->start();

    KSplashScreen* splash = 0;
    if( AmarokConfig::showSplashscreen() && !isSessionRestored() )
    {
        QPixmap splashimg( KGlobal::dirs()->findResource( "data", "amarok/images/splash_screen.jpg" ) );
        splash = new KSplashScreen( splashimg, Qt::WindowStaysOnTopHint );
        splash->show();
    }

    PERF_LOG( "Creating MainWindow" )
    m_mainWindow = new MainWindow();
    PERF_LOG( "Done creating MainWindow" )

    m_tray = new Amarok::TrayIcon( mainWindow() );

    PERF_LOG( "Creating DBus handlers" )
    new Amarok::RootDBusHandler();
    new Amarok::PlayerDBusHandler();
    new Amarok::TracklistDBusHandler();
    new CollectionDBusHandler( this );
    QDBusConnection::sessionBus().registerService("org.mpris.amarok");
    PERF_LOG( "Done creating DBus handlers" )

    if( splash ) // close splash correctly
    {
        splash->close();
        delete splash;
    }

    //DON'T DELETE THIS NEXT LINE or the app crashes when you click the X (unless we reimplement closeEvent)
    //Reason: in ~App we have to call the deleteBrowsers method or else we run afoul of refcount foobar in KHTMLPart
    //But if you click the X (not Action->Quit) it automatically kills MainWindow because KMainWindow sets this
    //for us as default (bad KMainWindow)
    mainWindow()->setAttribute( Qt::WA_DeleteOnClose, false );
    //init playlist window as soon as the database is guaranteed to be usable

    // Create engine, show TrayIcon etc.
    applySettings( true );

    // Must be created _after_ MainWindow.
    PERF_LOG( "Starting ScriptManager" )
    ScriptManager::instance();
    PERF_LOG( "ScriptManager started" )

    The::engineController()->setVolume( AmarokConfig::masterVolume() );
    The::engineController()->setMuted( AmarokConfig::muteState() );

    Amarok::KNotificationBackend::instance()->setEnabled( AmarokConfig::kNotifyEnabled() );
    Amarok::OSD::instance()->applySettings(); // Create after setting volume (don't show OSD for that)


    if( AmarokConfig::resumePlayback() && restoreSession && !args->isSet( "stop" ) ) {
        //restore session as long as the user didn't specify media to play etc.
        //do this after applySettings() so OSD displays correctly
        The::engineController()->restoreSession();
    }

    if( AmarokConfig::monitorChanges() )
        CollectionManager::instance()->checkCollectionChanges();

    // Restore keyboard shortcuts etc from config
    Amarok::actionCollection()->readSettings();

    PERF_LOG( "App init done" )
    KConfigGroup config = KGlobal::config()->group( "General" );
}