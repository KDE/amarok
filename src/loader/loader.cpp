/***************************************************************************
                        loader.cpp  -  loader application for amaroK
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

#include <loader.h>
#include <splash.h>

#include <qcstring.h>
#include <qdir.h>
#include <qfileinfo.h>

#include <qmessagebox.h>
#include <qprocess.h>
#include <qstring.h>
#include <qurl.h>

#include <iostream>
#include <pwd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>


Loader::Loader( int& argc, char** argv )
        : QApplication( argc, argv )
        , m_pOsd ( 0 )
{
    if( argc > 1 )
    {
        if( strcmp( argv[1], "--vis-socket-path" ) == 0 )
        {
            std::cout << socketPath( Vis ) << '\0';
            ::exit( 0 );
        }

        //put all arguments into one string
        //we transmit the startup_id, so amarokapp can stop the startup animation
        QCString str( ::getenv( "DESKTOP_STARTUP_ID" ) );

        //TODO support compacted short options eg "-ne"

        //these arguments require a running amaroK
        //the rest should be done on a new instance (eg --help)
        QStringList sockArgs;
        sockArgs << "--next" << "--previous" << "--stop" << "--pause" << "--play" << "--play-pause"
                 << "--enqueue" << "--append" << "--queue"
                 << "-caption" << "-f" << "-r" << "-s" << "-p" << "-e" << "-a" << "-m" << "-t";

        for ( int i = 0; i < argc; i++ )
        {
            QString arg = argv[ i ];

            str += '|';

            if ( sockArgs.contains( arg ) )
            {
                //pass this argument to amaroK
                str += argv[i];

            } else if ( arg[0] != '-' ) {

                //pass this URL to amaroK
                QFileInfo info( arg );

                if ( info.exists() && info.isRelative() )
                {
                    str += info.absFilePath().local8Bit();
                }
                else str += argv[i];

            } else {

                //we don't need to pass any of these arguments to amaroK
                //and if we did it KCmdLineArgs would make us exit!
                //TODO add all arguments or something

                QProcess proc( QString("amarokapp") );
                proc.setCommunication( 0 );
                proc.addArgument( arg );
                proc.start();

                while ( proc.isRunning() ) ::usleep( 100 );

                //now exit as help only prints usage info
                //no need to doExit(), as nothing is started
                ::exit( 0 );
            }
        }

        //lets see if amaroK is running
        m_sockfd = tryConnect( true );

        if ( m_sockfd != -1 )
        {
            ::send( m_sockfd, str, str.length() + 1, 0 ); //+1 = /0 termination

            doExit();
        }

        //if it isn't running we need to show the splash and start a new instance
        //we verified above that no arguments like --help are being passed
    }
    else m_sockfd = tryConnect( true );


    if ( m_sockfd == -1 )
    {
        //no, amaroK is not running -> start new instance, show splash
        std::cout << "[amaroK loader] amaroK not running. Trying to start it..\n";

        QStringList args( "amarokapp" );
        for ( int i = 1; i < argc; i++ ) args << argv[ i ]; //start at 1 to avoid appName

        QProcess proc( args );
        connect( &proc, SIGNAL( processExited() ), SLOT( doExit() ) );
        proc.setCommunication( 0 ); //ouputs everything straight to stdout/err
        proc.start();

        //show splash after starting process so we don't cause a delay
        //don't show splash on session restore
        if ( !QApplication::isSessionRestored() && splashEnabled() ) showSplash();

        //periodically check for amaroK startup completion
        startTimer( TIMER_INTERVAL );
    }
    else doExit();
    //TODO else the user just typed amarok with no arguments, I spose we should raise amaroK?
}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
////////////////////////////////////////////////////////////////////////////////

bool Loader::splashEnabled() const
{
    //determine whether splash-screen is enabled in amarokrc

    // Use $KDEHOME for kde config path if available; otherwise use $HOME
    QString path( ::getenv( "KDEHOME" ) );
    if ( path.isEmpty() )
        path = ::getenv( "HOME" ) + QString( "/.kde" );
    path += "/share/config/amarokrc";

    QFile file( path );

    if ( file.open( IO_ReadOnly ) ) {
        QString line;

        while ( file.readLine( line, 2000 ) != -1 )
            if ( line.contains( "Show Splashscreen" ) && line.contains( "false" ) )
                return false;

    } //if we fail to open it, just show the splash

    //default:
    return true;
}


//SLOT
void Loader::showSplash()
{
    //get KDE prefix from kde-config
    QProcess proc( QString( "kde-config" ) ); //"--install data" doesn't work! should do..
    proc.addArgument( "--prefix" );
    proc.start();

    //wait until process has finished
    while ( proc.isRunning() ) ::usleep( 100 );

    //read output from kde-config
    QString path = proc.readStdout();
    path.remove( '\n' );
    path += "/share/apps/amarok/images/logo_splash.png";

    m_pOsd = new OSDWidget;
    m_pOsd->showSplash( path );
}


QCString Loader::socketPath( SocketType type )
{
    static QCString path;

    // find out current user's kde sockets location
    // this is usually $KDEHOME/socket-$HOST/
    // or $HOME/.kde/socket-$HOST/
    // NOTE: /tmp/ksocket-$USER/ doesn't work because it may be located on a separate
    // filesystem from user's home (think NFS).

    // find out current user home dir
    // (code bits from kdelibs/kinit/lnusertemp.c)

    if( path.isEmpty() )
    {
        const int uid      = getuid();
        QCString  home_dir = getenv( "HOME" );
        QCString  kde_home = uid ? getenv( "KDEHOME" ) : getenv( "KDEROOTHOME" );
        passwd   *pw_ent   = getpwuid( uid );

        if ( !pw_ent ) qFatal( "Current user does not exist?!" );

        if ( !kde_home || !kde_home[0] ) kde_home = "~/.kde";

        if ( kde_home[0] == '~' )
        {
            if ( uid == 0 )
                home_dir = pw_ent->pw_dir ? pw_ent->pw_dir : "/root";

            if ( !home_dir || !home_dir[0] )
                qFatal( "$HOME not set!" );

            if ( home_dir.length() > ( PATH_MAX - 100 ) )
                qFatal( "Home directory path is too long!" );

            path = home_dir;
            kde_home = kde_home.mid( 1 );
        }

        path += kde_home;
        path += "/socket-";

        char hostname[ 100 ];
        if ( gethostname( &hostname[ 0 ], 100 ) != 0 )
            qFatal( "Could not determine this computer's hostname." );

        path += hostname;


        QFileInfo inf( path );
        if ( !inf.isDir() && !inf.isSymLink() )
            qFatal( "Socket-path is not a link or a directory: %s\n", (const char*)path );
    }

    QCString retpath = path;
    retpath += ((type == loader) ? "/amarok.loader_socket" : "/amarok.visualization_socket");

    return retpath;
}


int Loader::tryConnect( bool verbose )
{
    //try to connect to the LoaderServer
    int fd = ::socket( AF_UNIX, SOCK_STREAM, 0 );
    if ( fd == -1 ) {
        qDebug( "[amaroK loader] socket() error" );
        return -1;
    }
    sockaddr_un local;
    local.sun_family = AF_UNIX;

    QCString path = socketPath();

    ::strcpy( &local.sun_path[ 0 ], path );

    if ( verbose )
        std::cerr << "[amaroK loader] connecting to " << path << '\n';

    if ( ::connect( fd, ( sockaddr* ) & local, sizeof( local ) ) == -1 ) {
        ::close ( fd );
        return -1;
    }

    return fd;
}

//SLOT
void Loader::doExit()
{
    std::cout << "[amaroK loader] exiting.\n";

    delete m_pOsd;

    if ( m_sockfd != -1 )
        ::close( m_sockfd );

    ::exit( 0 );
}


void Loader::timerEvent( QTimerEvent* )
{
    static uint delay;

    delay += TIMER_INTERVAL;
    m_sockfd = tryConnect();

    if ( m_sockfd != -1 ) {
        killTimers();

        std::cout << "[amaroK loader] startup successful.\n";
        ::send( m_sockfd, "STARTUP", 8, 0 );
        doExit();
    } else if ( delay >= TIMEOUT * 1000 ) {
        killTimers();
        std::cout << "[amaroK loader] timed out trying to contact amaroK.\n";

        //NOTE these are untranslated.. nasty.

        QMessageBox::critical( 0, "amaroK",
            "amaroK could not be started, try typing \"amarok\" at "
            "a command prompt, perhaps the output can help.",
            QMessageBox::Ok, QMessageBox::NoButton );

        doExit();
    }
}

#include "loader.moc"
