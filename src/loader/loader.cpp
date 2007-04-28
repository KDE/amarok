/***************************************************************************
                        loader.cpp  -  loader application for Amarok
                           -------------------
  begin                : 2004/02/19
  copyright            : (C) 2004 by Mark Kretschmann
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

#include <cstdlib>
#include <iostream>
#include "loader.h"
#include <QFile>
#include <QMessageBox>
#include <Q3Process>
#include <QString>
#include <kglobal.h>
#include <KSplashScreen>
#include <kstandarddirs.h>
#include <kcomponentdata.h>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

extern "C"
{
    #include <unistd.h> //::usleep
}

int
main( int argc, char *argv[] )
{
    //NOTE this list doesn't include argv[0] ("amarok")
    QStringList args;
    for( int i = 1; i < argc; i++ )
        args += QString::fromLocal8Bit(argv[i]);

    const bool isRunning = amarokIsRunning();

    // first check the arguments, we don't need a splashscreen
    // for arguments like --help, --version, etc.

    if( !args.isEmpty() )
    {
        // These arguments cannot be passed to Amarok, or Amarok will exit
        // after processing them.
        QStringList longs; longs
                << "-help" << "-help-qt" << "-help-kde" << "-help-all" << "-author" << "-version" << "-license" << "-v";

        // both --arg and -arg are valid
        {
            QStringList longlongs;
            foreach( QString str, longs )
                longlongs += QChar('-') + str;

            longs += longlongs;
        }

        foreach( QString str, args ) {
            const QString arg = str;
            foreach( QString str1, longs )
                if( arg == str1 ) {
                    // this argument cannot be passed to the running amarokapp
                    // or KCmdLineArgs would exit the application

                    Q3Process proc( QString("amarokapp") );
                    proc.setCommunication( 0 ); //show everything
		    proc.addArgument( arg );
                    proc.start();

                    while( proc.isRunning() )
                        ::usleep( 100 );

                    return 0; //exit success!
                }
        }

        // these arguments are deemed safe for dcop, but if
        // there is no amarokapp running, we'll start a new
        // instance and the above checks were not necessary
    }

    if ( isRunning ) {
        QStringList dbus_args;
        dbus_args << "qdbus" << "org.kde.amarok" << "/Player" << "transferCliArgs" << "[";

        // We transmit our DESKTOP_STARTUP_ID, so amarokapp can stop the startup animation
        dbus_args += std::getenv( "DESKTOP_STARTUP_ID" ); //will be interptreted as latin1

        // relative URLs should be interpreted correctly by amarokapp
        // so we need to pass the current working directory
        dbus_args << "--cwd" << QDir::currentPath();

        dbus_args += args;
        dbus_args += "]"; // krazy:exclude=doublequote_chars

        Q3Process proc( dbus_args );
        proc.start();
        while( proc.isRunning() )
            ::usleep( 100 );

        return 0;
    }
    else {
        // no amarokapp is running, start one, show
        // a splashscreen and pass it the arguments

        return Loader( args ).exec();
    }
}

bool
amarokIsRunning()
{
    //TODO verify service name
    if(QDBusConnection::sessionBus().interface()->isServiceRegistered("amarok.kde.org"))
	    return true;
    return false;
}



static int _argc = 0;

Loader::Loader( QStringList args )
        : QApplication( _argc, 0 )
        , m_counter( 0 )
        , m_splash( 0 )
{
    // we transmit the startup_id, so amarokapp can stop the startup animation
    //FIXME QCString str( ::getenv( "DESKTOP_STARTUP_ID" ) );

     if( !QApplication::isSessionRestored() && isSplashEnabled() )
     {
        QPixmap splashpix( KStandardDirs().findResource("data", "amarok/images/splash_screen.jpg") );
        m_splash = new KSplashScreen( splashpix, Qt::WindowStaysOnTopHint );
        m_splash->show();
     }

    args.prepend( "amarokapp" );

    m_proc = new Q3Process( args, this );
    m_proc->setCommunication( Q3Process::Stdout );

    std::cout << "Amarok: [Loader] Starting amarokapp..\n";
    std::cout << "Amarok: [Loader] Don't run gdb, valgrind, etc. against this binary! Use amarokapp.\n";

    if( !m_proc->start() )
    {
        delete m_splash; // hide the splash

        QMessageBox::critical( 0, "Amarok",
                "Amarok could not be started!\n" //FIXME this needs to be translated
                    "This may be because the amarokapp binary is not in your PATH.\n"
                    "Try locating and running amarokapp from a terminal.",
                QMessageBox::Ok, 0 );

        std::exit( 1 ); //event-loop is not yet being processed
    }

    startTimer( INTERVAL );
}

Loader::~Loader()
{
    // must be deleted before QApplication closes our Xserver connection
    // thus we cannot make it a child of the QApplication and must
    // delete it manually
    delete m_splash;
}

void
Loader::timerEvent( QTimerEvent* )
{
    if( m_proc->isRunning() )
    {
         if( ++m_counter == (30000 / INTERVAL) )
             // 30 seconds have passed
             std::cerr << "Amarok: [Loader] Amarok is taking a long time to load! Perhaps something has gone wrong?\n";

        while( m_proc->canReadLineStdout() )
            if( m_proc->readLineStdout() == "STARTUP" )
                QApplication::exit( 0 );
    }
    else if( !m_proc->normalExit() ) {
        // no reason to show messagebox, as amarokapp should start drkonqi
        std::cerr << "Amarok: [Loader] amarokapp probably crashed!\n";

        QApplication::exit( 3 );
    }
    else
        // if we get here, then either we didn't receive STARTUP through
        // the pipe, or amarokapp exited normally before the STARTUP was
        // written to stdout (possibly possible)
        QApplication::exit( 0 );
}

bool
isSplashEnabled()
{
    //determine whether splash-screen is enabled in amarokrc
    KComponentData instance("amarok"); // KGlobal::dirs() crashes without
    //(void)KGlobal::config(); // the kubuntu special directory is not present without this
    QStringList dirs = KGlobal::dirs()->findAllResources( "config", "amarokrc" );

    for( QStringList::iterator path = dirs.begin();
            path != dirs.end();
            ++path )
    {
        QFile file( *path );
        if ( file.open( QIODevice::ReadOnly ) )
        {
	    while (!file.atEnd()) {
		    QByteArray line = file.readLine();
		    if( line.contains( "false" ) )
			    return false;
		    else
			    return true;
	    }
        }
    }

    //if we fail to open anything, just show the splash
    return true;
}
