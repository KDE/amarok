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

#include "analyzerbase2d.h"

#include <math.h>
#include <vector>

#include <qimage.h>
#include <qpainter.h>
#include <qpen.h>
#include <qpixmap.h>


// INSTRUCTIONS
// 1. inherit AnalyzerBase2d( first parameter to AnalyzerBase2d is the frequency (in milliseconds) that drawAnalyser will be called)
// 2. do anything that depends on height() in init()
// 3. otherwise you can use the constructor
// 4. blt to this at the end of your implementation of drawAnalyser()


AnalyzerBase2d::AnalyzerBase2d( uint timeout, QWidget *parent, const char *name )
   : QWidget( parent, name )
   , AnalyzerBase( timeout )
{}


// METHODS =====================================================

void AnalyzerBase2d::polish()
{
    QWidget::polish();

    m_height = QWidget::height();

    //we use polish for initialzing (instead of ctor), because we need to know the widget's final size

    m_background.resize( size() );
#ifdef DRAW_GRID
    QPainter painterGrid( &m_background );
    painterGrid.setPen( QPen( QColor( 0x20, 0x20, 0x50 ) ) );

    for( int x = 0, w = m_background.width(), h = m_background.height()-1;
         x < w;
         x += 3 )
    {
        painterGrid.drawLine( x, 0, x, h );
    }

    for( int y = 0, w = m_background.width()-1 , h = m_background.height();
         y < h;
         y += 3 )
    {
        painterGrid.drawLine( 0, y, w, y );
    }
#else
    m_background.fill( parentWidget()->backgroundColor() );
#endif

    init(); //virtual
}


void AnalyzerBase2d::initSin( std::vector<float> &v ) const
{
    double step = ( M_PI * 2 ) / SINVEC_SIZE;
    double radian = 0;

    for ( int i = 0; i < SINVEC_SIZE; i++ )
    {
        v.push_back( sin( radian ) );
        radian += step;
    }
}

#include "analyzerbase2d.moc"
