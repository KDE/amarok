/***************************************************************************
                          viswidget.cpp  -  description
                             -------------------
    begin                : Die Jan 7 2003
    copyright            : (C) 2003 by Mark Kretschmann
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

#include "analyzerbase3d.h"

#include <math.h>
#include <vector>


// INSTRUCTIONS
// 1. inherit AnalyzerBase( first parameter to AnalyzerBase is the frequency (in milliseconds) that drawAnalyser will be called)
// 2. do anything that depends on height() in init()
// 3. otherwise you can use the constructor
// 4. blt to this at the end of your implementation of drawAnalyser()


AnalyzerBase3d::AnalyzerBase3d( uint timeout, QWidget *parent, const char *name ) :
   QGLWidget( parent, name ),
   AnalyzerBase( timeout )
{}

AnalyzerBase3d::~AnalyzerBase3d()
{}


// METHODS =====================================================

void AnalyzerBase3d::mousePressEvent( QMouseEvent *e )
{
    if( e->button() == Qt::LeftButton )
    {
        emit clicked();
        
        e->accept();  //don't propagate to PlayerWidget!
    }
    else e->ignore();
}

#include "analyzerbase3d.moc"
