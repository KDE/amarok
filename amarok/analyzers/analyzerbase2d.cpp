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
// 1. inherit AnalyzerBase( first parameter to AnalyzerBase is the frequency (in milliseconds) that drawAnalyser will be called)
// 2. do anything that depends on height() in init()
// 3. otherwise you can use the constructor
// 4. blt to this at the end of your implementation of drawAnalyser()


AnalyzerBase2d::AnalyzerBase2d( uint timeout, QWidget *parent, const char *name ) :
   QWidget( parent, name ),
   AnalyzerBase( timeout )
{}

AnalyzerBase2d::~AnalyzerBase2d()
{}


// METHODS =====================================================

void AnalyzerBase2d::polish()
{
    QWidget::polish();

    m_iVisHeight = QWidget::height();

    //we use polish for initialzing (instead of ctor), because we need to know the widget's final size
    initGrid();
    init(); //virtual
}


void AnalyzerBase2d::initGrid()
{
    m_grid.resize( width(), height() );
    bitBlt( &m_grid, 0, 0, parentWidget()->paletteBackgroundPixmap(), x(), y(), width(), height() );

  #ifdef DRAW_GRID
    QPainter painterGrid( &m_grid );
    painterGrid.setPen( QPen( QColor( 0x20, 0x20, 0x50 ) ) );

    for( int x = 0, w = m_grid.width(), h = m_grid.height()-1;
         x < w;
         x += 3 )
    {
        painterGrid.drawLine( x, 0, x, h );
    }

    for( int y = 0, w = m_grid.width()-1 , h = m_grid.height();
         y < h;
         y += 3 )
    {
        painterGrid.drawLine( 0, y, w, y );
    }
  #endif    
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

void AnalyzerBase2d::mousePressEvent( QMouseEvent *e )
{
    if( e->button() == Qt::LeftButton )
    {
        emit clicked();
        
        e->accept();  //don't propagate to PlayerWidget!
    }
    else e->ignore();
}

#include "analyzerbase2d.moc"
