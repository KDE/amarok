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
//     qDebug( "[Loader::Loader()]" );

    m_sockfd = tryConnect();

    //determine whether an amaroK instance is already running (LoaderServer)
    if ( m_sockfd == -1 ) {
        //no, amaroK is not running -> show splash, start new instance
        qDebug( "[amaroK loader] amaroK not running. Trying to start it..\n" );

        if ( splashEnabled() )
            showSplash();

        m_pProc = new QProcess( this );
        m_pProc->addArgument( "amarokapp" );
        //hand arguments through to amaroK
        //NOTE the first argument is /path/to/amarok but this is not /path/to/amarokapp so we ignore it
        //FIXME it should be converted to /path/top/amarokapp instead, I think, or there might be new bugs
        for ( int i = 1; i < m_argc; i++ )
            m_pProc->addArgument( m_argv[i] );

        connect( m_pProc, SIGNAL( readyReadStdout() ), this, SLOT( stdoutActive() ) );
        connect( m_pProc, SIGNAL( processExited() ),   this, SLOT( doExit() ) );
        m_pProc->setCommunication( QProcess::Stdin || QProcess::Stdout );
        m_pProc->start();

        //wait until LoaderServer starts (== amaroK is up and running)
        while( ( m_sockfd = tryConnect() ) == -1 ) {
            processEvents();
            ::usleep( 200 * 1000 );    //== 200ms
        }
        QCString str = "STARTUP";
        ::send( m_sockfd, str, str.length(), 0 );
        std::cout << "[amaroK loader] amaroK startup successful.\n";
    }
    else {
        //yes, amaroK is running -> transmit new command line args to the LoaderServer and exit
        std::cout << "[amaroK loader] amaroK is already running. Transmitting command line arguments..\n";

        //put all arguments into one string
        QCString str;
        for ( int i = 0; i < m_argc; i++ ) {
            str.append( m_argv[i] );
            str.append( " " );
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
    QProcess* proc = new QProcess( this );
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
    QCString path( ::getenv( "HOME" ) );
    path += "/.kde/share/apps/amarok/.loader_socket";
    ::strcpy( &local.sun_path[0], path );

    int len = ::strlen( local.sun_path ) + sizeof( local.sun_family );

    if ( ::connect( fd, (struct sockaddr*) &local, len ) == -1 ) {
//         qDebug( "[Loader::tryConnect()] connect() failed" );
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




////////////////////////////////////////////////////////////////////////////////
// TODO
////////////////////////////////////////////////////////////////////////////////

//* IPC. When amaroK is already loaded, find the instance and send the command args to it, without
//  loading a new instance (much faster). This way we can also get rid of KUniqueApplication
//* For a challenge, make this work without Qt! This would reduce loading time even more, especially
//  when Qt is not already in memory
//* Eventually rename loader to "amarok", and amaroK to "amarok-app"

