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

#include <config.h>
#ifdef HAVE_QGLWIDGET

#include "analyzerbase3d.h"

AnalyzerBase3d::AnalyzerBase3d( uint timeout, QWidget *parent, const char *name )
   : QGLWidget( parent, name )
   , AnalyzerBase( timeout )
{}

#include "analyzerbase3d.moc"

#endif
