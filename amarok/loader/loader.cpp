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

#include <stdlib.h>
#include <qprocess.h>
#include <qstring.h>


Loader::Loader( int& argc, char** argv )
    : QApplication( argc, argv )
    , m_pOsd( new OSDWidget )
{
    qDebug( "[Loader::Loader()]" );

    QString path( getenv( "KDEDIR" ) );
    path += "/share/apps/amarok/images/logo_splash.png";
    m_pOsd->showSplash( path );
        
    QProcess* proc = new QProcess( this );
    proc->addArgument( "amarok" );
    proc->start();
    
    connect( proc, SIGNAL( processExited() ), this, SLOT( loaded() ) );    
}


Loader::~Loader()
{
    delete m_pOsd;
}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
////////////////////////////////////////////////////////////////////////////////
    
//SLOT
void Loader::loaded()
{
    qDebug( "[Loader::loaded()]" );
    quit();
}


#include "loader.moc"
