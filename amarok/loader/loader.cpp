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

#include <qprocess.h>
#include <qstring.h>


Loader::Loader( int& argc, char** argv )
    : QApplication( argc, argv )
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
    
    QProcess* proc = new QProcess( this );
    proc->addArgument( "amarok" );
    proc->start();
    
    connect( proc, SIGNAL( processExited() ), this, SLOT( loaded() ) );    
}
    
//SLOT
void Loader::loaded()
{
    qDebug( "[Loader::loaded()]" );
    quit();
}


#include "loader.moc"
