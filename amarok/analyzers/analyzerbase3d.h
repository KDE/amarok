/***************************************************************************
                          anaylyzerbase3d.h  -  description
                             -------------------
    begin                : Die Jan 7 2003
    copyright            : (C) 2003 by Adam Pigg
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ANALYZERBASE3D_H
#define ANALYZERBASE3D_H

#include <config.h>

#ifdef HAVE_QGLWIDGET

#include <qgl.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <vector>
#include "analyzerbase.h"

class QMouseEvent;
class QQLWidget;


/**
 *@author PiggZ
 */

class AnalyzerBase3d : public QGLWidget, public AnalyzerBase
{
    Q_OBJECT

    public:
        AnalyzerBase3d( uint, QWidget *parent=0, const char *name=0 );
        virtual ~AnalyzerBase3d();

    signals:
        void clicked();

    protected:
        virtual void mousePressEvent( QMouseEvent* );

    private:

};

#endif

#endif
