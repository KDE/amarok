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
#include <qfileinfo.h>
#include <qprocess.h>
#include <qstring.h>

#include <iostream>
#include <pwd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>


#define TIMEOUT 50


Loader::Loader( int& argc, char** argv )
        : QApplication( argc, argv )
        , m_pOsd ( 0 )
{
    m_sockfd = tryConnect( true );

    //determine whether an amaroK instance is already running (LoaderServer)
    if ( m_sockfd == -1 )
    {
        //no, amaroK is not running -> start new instance, show splash
        std::cout << "[amaroK loader] amaroK not running. Trying to start it..\n";

        QStringList args( "amarokapp" );
        for( int i = 1; i < argc; i++ ) args << argv[i]; //start at 1 to avoid appName

        QProcess proc( args );
        connect( &proc, SIGNAL(processExited()), SLOT(doExit()) );
        proc.setCommunication( QProcess::Stdout );
        proc.start();

        //show splash after starting process so we don't cause a delay
        if( splashEnabled() ) showSplash();

        //periodically check for amaroK startup completion
        startTimer( TIMEOUT );

    } else {

        //yes, amaroK is running -> transmit new command line args to the LoaderServer and exit
        std::cout << "[amaroK loader] amaroK is already running. Transmitting command line arguments..\n";

        //put all arguments into one string
        //we transmit the startup_id, so amarokapp can stop the startup animation
        QCString str = ::getenv( "DESKTOP_STARTUP_ID" );

        for ( int i = 0; i < argc; i++ )
        {
            str += '|'; //"|" cannot occur in unix filenames, so we use it as a separator
            str += argv[ i ];
        }
        ::send( m_sockfd, str, str.length() + 1, 0 ); //+1 = /0 termination

        doExit();
    }
}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
////////////////////////////////////////////////////////////////////////////////

bool Loader::splashEnabled() const
{
    //determine whether splash-screen is enabled in amarokrc
    //FIXME we need to use kde-config to get the path
    QString path( ::getenv( "HOME" ) );
    path += "/.kde/share/config/amarokrc";

    QFile file( path ); //close() is called in the dtor

    if( file.open( IO_ReadOnly ) )
    {
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
    QProcess proc( QString("kde-config") ); //"--install data" doesn't work! should do..
    proc.addArgument( "--prefix" );
    proc.start();

    //wait until process has finished
    while( proc.isRunning() ) ::usleep( 100 );

    //read output from kde-config
    QString path = proc.readStdout();
    path.remove( '\n' );
    path += "/share/apps/amarok/images/logo_splash.png";

    m_pOsd = new OSDWidget;
    m_pOsd->showSplash( path );
}


int Loader::tryConnect( bool verbose )
{
    static QCString path;

    //try to connect to the LoaderServer
    int fd = ::socket( AF_UNIX, SOCK_STREAM, 0 );
    if ( fd == -1 ) {
        qDebug( "[amaroK loader] socket() error" );
        return -1;
    }
    sockaddr_un local;
    local.sun_family = AF_UNIX;

    // find out current user's kde sockets location
    // this is usually $KDEHOME/socket-$HOST/
    // or $HOME/.kde/socket-$HOST/
    // NOTE: /tmp/ksocket-$USER/ doesn't work because it may be located on a separate
    // filesystem from user's home (think NFS).

    // find out current user home dir
    // (code bits from kdelibs/kinit/lnusertemp.c)
    if( path.isEmpty() )
    {
        int uid = getuid();
        QCString home_dir = getenv( "HOME" );
        QCString kde_home = uid ? getenv( "KDEHOME" ) : getenv( "KDEROOTHOME" );

        struct passwd *pw_ent = getpwuid( uid );
        if ( !pw_ent )
            qFatal( "[Loader::tryConnect()] Current user does not exist?!" );

        if ( !kde_home || !kde_home[ 0 ] )
            kde_home = "~/.kde/";

        if ( kde_home[ 0 ] == '~' ) {
            if ( uid == 0 )
                home_dir = pw_ent->pw_dir ? pw_ent->pw_dir : "/root";

            if ( !home_dir || !home_dir[ 0 ] )
                qFatal( "Aborting. $HOME not set!" );

            if ( home_dir.length() > ( PATH_MAX - 100 ) )
                qFatal( "Aborting. Home directory path too long!" );

            path = home_dir;
            kde_home = kde_home.mid(1);
        }
        path += kde_home;
        path += "/socket-";

        char hostname[100];
        if ( gethostname( &hostname[0], 100 ) != 0 )
            qFatal( "Aborting. Could not determine hostname." );

        path += hostname;


        QFileInfo inf( path );
        if ( !inf.isDir() && !inf.isSymLink() ) {
            qWarning( path );
            qFatal( "Error: Path is not a link or a directory.\n" );
        }

        path += "/amarok.loader_socket";
    }

    ::strcpy( &local.sun_path[ 0 ], path );

    if ( verbose )
        std::cerr << "[amaroK loader] connecting to " << path << '\n';

    if ( ::connect( fd, (sockaddr*)&local, sizeof(local) ) == -1 ) {
        ::close ( fd );
        return -1;
    }

    return fd;
}

//SLOT
void Loader::doExit()
{
    std::cout << "[amaroK loader] exiting.\n\n";

    delete m_pOsd;

    if ( m_sockfd != -1 )
        ::close( m_sockfd );

    ::exit( 0 );
}


void Loader::timerEvent( QTimerEvent* )
{
    static uint delay;

    delay   += TIMEOUT;
    m_sockfd = tryConnect();

    if( m_sockfd != -1 )
    {
        killTimers();

        std::cout << "[amaroK loader] startup successful.\n";
        ::send( m_sockfd, "STARTUP", 8, 0 );
        doExit();
    }
    else if( delay >= 30*1000 )
    {
        std::cout << "[amaroK loader] timed out trying to contact to amaroK.\n";
        doExit();
    }
}

#include "loader.moc"
