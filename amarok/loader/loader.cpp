/***************************************************************************
                          loader.cpp  -  description
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

#include <stdlib.h>
#include <unistd.h>  
#include <sys/socket.h>
#include <sys/un.h>    


Loader::Loader( int& argc, char** argv )
    : QApplication( argc, argv )
    , m_argc( argc )
    , m_argv( argv )
    , m_pOsd( new OSDWidget )
{
    qDebug( "[Loader::Loader()]" );

    m_pPrefixProc = new QProcess( this );
    m_pPrefixProc->addArgument( "kde-config" );
    m_pPrefixProc->addArgument( "--prefix" );
    connect( m_pPrefixProc, SIGNAL( processExited() ), this, SLOT( gotPrefix() ) );    
    
    m_pPrefixProc->start();
}


Loader::~Loader()
{
    delete m_pOsd;
}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
////////////////////////////////////////////////////////////////////////////////

//SLOT
void Loader::gotPrefix()
{
    QString path = m_pPrefixProc->readStdout();
    path.remove( "\n" );
    path += "/share/apps/amarok/images/logo_splash.png";
    qDebug( path.latin1() );
    m_pOsd->showSplash( path );
    
    int sockfd = tryConnect();
    
    if ( sockfd == -1 ) {
        QProcess* proc = new QProcess( this );
        proc->addArgument( "amarok" );
        
        //hand arguments through to amaroK
        for ( int i = 1; i < m_argc; i++ )
            proc->addArgument( m_argv[i] );
        
        proc->start();
        connect( proc, SIGNAL( processExited() ), this, SLOT( loaded() ) );    
    }
    else
    {
        qDebug( "[Loader::gotPrefix()] connected!" );
        
        QCString text = "Captain to ship: battle station all decks!";
        ::send( sockfd, text, text.length(), 0 );
        ::close(sockfd );
        
        loaded();    
    }
}
    
int Loader::tryConnect()
{
    //try to connect to the LoaderServer
    uint sockfd = ::socket( AF_UNIX, SOCK_STREAM, 0 );
    if ( sockfd == -1 ) {
        qDebug( "[Loader::tryConnect()] socket() error" );
        return -1;
    }
    sockaddr_un local;
    local.sun_family = AF_UNIX;
    QCString path( ::getenv( "HOME" ) );
    path += "/.kde/share/apps/amarok/.loader_socket";
    ::strcpy( &local.sun_path[0], path );
    
    int len = ::strlen( local.sun_path ) + sizeof( local.sun_family );
    
    if ( ::connect( sockfd, (struct sockaddr*) &local, len ) == -1 ) {
        qDebug( "[Loader::tryConnect()] connect() error" );
        return -1;
    }

    return sockfd;
}

//SLOT
void Loader::loaded()
{
    qDebug( "[Loader::loaded()]" );
    quit();
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

