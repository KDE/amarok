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
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

Loader::Loader( int& argc, char** argv )
        : QApplication( argc, argv )
        , m_argc ( argc )
        , m_argv ( argv )
        , m_pProc( NULL )
        , m_pOsd ( NULL )
{
    m_sockfd = tryConnect();

    //determine whether an amaroK instance is already running (LoaderServer)
    if ( m_sockfd == -1 ) {
        //no, amaroK is not running -> show splash, start new instance
        qDebug( "[amaroK loader] amaroK not running. Trying to start it..\n" );

        if ( splashEnabled() )
            showSplash();

        m_pProc = new QProcess( this );

        QString path = argv[ 0 ];
        path.replace( "amarok", "amarokapp" );   //FIXME!!! don't replace in path
        m_pProc->addArgument( path );

        //hand arguments through to amaroK
        for ( int i = 1; i < m_argc; i++ )
            m_pProc->addArgument( m_argv[ i ] );

        connect( m_pProc, SIGNAL( readyReadStdout() ), this, SLOT( stdoutActive() ) );
        connect( m_pProc, SIGNAL( processExited() ), this, SLOT( doExit() ) );
        m_pProc->setCommunication( QProcess::Stdin || QProcess::Stdout );
        m_pProc->start();

        //wait until LoaderServer starts (== amaroK is up and running)
        while ( ( m_sockfd = tryConnect() ) == -1 ) {
            processEvents();
            ::usleep( 200 * 1000 );    //== 200ms
        }
        QCString str = "STARTUP";
        ::send( m_sockfd, str, str.length(), 0 );
        std::cout << "[amaroK loader] amaroK startup successful.\n";
    } else {
        //yes, amaroK is running -> transmit new command line args to the LoaderServer and exit
        std::cout << "[amaroK loader] amaroK is already running. Transmitting command line arguments..\n";

        //put all arguments into one string
        QCString str;

        //we transmit the startup_id, so amarokapp can stop the startup animation
        str.append( getenv( "DESKTOP_STARTUP_ID" ) );
        str.append( "|" );

        for ( int i = 0; i < m_argc; i++ ) {
            str.append( m_argv[ i ] );
            str.append( "|" );    //"|" cannot occur in unix filenames, so we use it as a separator
        }
        ::send( m_sockfd, str, str.length(), 0 );
    }

    doExit();
}


Loader::~Loader()
{}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
////////////////////////////////////////////////////////////////////////////////

bool Loader::splashEnabled() const
{
    //determine whether splash-screen is enabled in amarokrc
    QString path( ::getenv( "HOME" ) );
    path += "/.kde/share/config/amarokrc";

    QFile file( path );
    file.open( IO_ReadOnly );
    QString line;
    QString found;

    while ( file.readLine( line, 2000 ) != -1 ) {
        if ( line.contains( "Show Splashscreen" ) && line.contains( "false" ) )
            return false;
    }

    //default:
    return true;
}


//SLOT
void Loader::showSplash()
{
    //get KDE prefix from kde-config
    QProcess * proc = new QProcess( this );
    proc->addArgument( "kde-config" );
    proc->addArgument( "--prefix" );
    proc->start();

    //wait until process has finished
    while ( proc->isRunning() );

    //read output from kde-config
    QString path = proc->readStdout();
    path.remove( "\n" );
    path += "/share/apps/amarok/images/logo_splash.png";
    //     qDebug( path.latin1() );

    m_pOsd = new OSDWidget;
    m_pOsd->showSplash( path );
    processEvents();
}

// getpwuid/getuid/gethostname/lstat/readlink
#include <pwd.h>
#include <unistd.h>

int Loader::tryConnect()
{
    //try to connect to the LoaderServer
    int fd = ::socket( AF_UNIX, SOCK_STREAM, 0 );
    if ( fd == -1 ) {
        qDebug( "[Loader::tryConnect()] socket() error" );
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
    int uid = getuid();
    QCString home_dir = getenv( "HOME" );
    QCString kde_home = uid ? getenv( "KDEHOME" ) : getenv( "KDEROOTHOME" );
    QCString path;

    struct passwd *pw_ent = getpwuid( uid );
    if ( !pw_ent ) {
        qFatal( "[Loader::tryConnect()] Current user does not exist?!?" );
        return -1;
    }

    if ( !kde_home || !kde_home[ 0 ] )
        kde_home = "~/.kde/";

    if ( kde_home[ 0 ] == '~' ) {
        if ( uid == 0 ) {
            home_dir = pw_ent->pw_dir ? pw_ent->pw_dir : "/root";
        }
        if ( !home_dir || !home_dir[ 0 ] ) {
            qFatal( "Aborting. $HOME not set!" );
            return -1;
        }
        if ( home_dir.length() > ( PATH_MAX - 100 ) ) {
            qFatal( "Aborting. Home directory path too long!" );
            return -1;
        }
        path = home_dir;
    }
    path += kde_home;
    path += "/socket-";

    char hostname[100];
    if ( gethostname( &hostname[0], 100 ) != 0 ) {
        qFatal( "Aborting. Could not determine hostname." );
        return -1;
    }
    path += QCString( &hostname[0] );
    
    QFileInfo inf( path );
    if ( !inf.isDir() && !inf.isSymLink() ) {
        qWarning( path );
        qFatal( "Error: Path is not a link or a directory.\n" );
    }
        
    path += "/amarok.loader_socket";
    ::strcpy( &local.sun_path[ 0 ], path );

    std::cerr << "[Loader::tryConnect()] connecting to " << local.sun_path << '\n';
    int len = sizeof( local );

    if ( ::connect( fd, ( struct sockaddr* ) & local, len ) == -1 ) {
        //         qDebug( "[Loader::tryConnect()] connect() failed" );
        ::close ( fd );
        return -1;
    }

    return fd;
}

//SLOT
void Loader::doExit()
{
    std::cout << "[amaroK loader] Loader exiting.\n";

    delete m_pOsd;
    delete m_pProc;

    if ( m_sockfd != -1 )
        ::close( m_sockfd );

    ::exit( 0 );
}

//SLOT
void Loader::stdoutActive()
{
    qDebug( "[Loader::stdoutActive()]" );

    //hand amarokapp's stdout messages through to the cli
    std::cout << m_pProc->readStdout() << "\n";
}


#include "loader.moc"


