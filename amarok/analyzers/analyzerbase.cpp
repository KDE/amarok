/***************************************************************************
                          viswidget.cpp  -  description
                             -------------------
    begin                : Die Jan 7 2003
    copyright            : (C) 2003 by Mark Kretschmann
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

#include "analyzerbase.h"

#include <math.h>
#include <vector>

#include <qevent.h>
#include <qframe.h>
#include <qimage.h>
#include <qpainter.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qwidget.h>

#include <kdebug.h>
#include <kstandarddirs.h>


// INSTRUCTIONS
// 1. inherit AnalyzerBase, first parameter to AnalyzerBase is the frequency (in seconds) that drawAnalyser will be called
// 2. do anything that depends on height() in init()
// 3. otherwise you can use the constructor
// 4. blt to this in drawAnalyser()


AnalyzerBase::AnalyzerBase( uint timeout, QWidget *parent, const char *name ) :
   QFrame( parent, name ),
   m_pGridPixmap( 0 ),
   m_timeout( timeout )
{}


AnalyzerBase::~AnalyzerBase()
{
    delete m_pGridPixmap;
}


// METHODS =====================================================

void AnalyzerBase::polish()
{
    QWidget::polish();

    m_iVisHeight = QWidget::height();

    //we use polish for initialzing (instead of ctor), because we need to know the widget's final size
    initGrid();
    init(); //virtual
}


void AnalyzerBase::init()
{}


void AnalyzerBase::initGrid()
{
    m_pGridPixmap = new QPixmap( parentWidget()->paletteBackgroundPixmap()->convertToImage()
        .copy( this->x(), this->y(), width(), height() ) );

    QPainter painterGrid( m_pGridPixmap );
    painterGrid.setPen( QPen( QColor( 0x20, 0x20, 0x50 ) ) );

    for ( int x = 0; x < m_pGridPixmap->width(); x += 3 )
    {
        painterGrid.drawLine( x, 0, x, m_pGridPixmap->height()-1 );
    }

    for (int y = 0; y < m_pGridPixmap->height(); y += 3 )
    {
        painterGrid.drawLine( 0, y, m_pGridPixmap->width()-1, y );
    }
}


void AnalyzerBase::initSin( std::vector<float> &v ) const
{
    double step = ( M_PI * 2 ) / SINVEC_SIZE;
    double radian = 0;

    for ( int i = 0; i < SINVEC_SIZE; i++ )
    {
        v.push_back( sin( radian ) );
        radian += step;
    }
}


void AnalyzerBase::mouseReleaseEvent( QMouseEvent * )
{
    emit clicked();
}


void AnalyzerBase::interpolate( std::vector<float> *oldVec, std::vector<float> &newVec ) const
{
    uint newSize = newVec.size(); //vector::size() is O(1)

    double pos = 0.0;
    double step = static_cast<double>( oldVec->size() ) / newSize;

    for ( uint i = 0; i < newSize; ++i, pos += step )
    {
        double error = pos - floor( pos );
        unsigned long offset = static_cast<unsigned long>( pos );

        unsigned long indexLeft = offset + 0;

        if ( indexLeft >= oldVec->size() )
            indexLeft = oldVec->size() - 1;

        unsigned long indexRight = offset + 1;

        if ( indexRight >= oldVec->size() )
            indexRight = oldVec->size() - 1;

        newVec[i] = oldVec->at( indexLeft ) * ( 1.0 - error ) +
                    oldVec->at( indexRight ) * error;
    }
}

#include "analyzerbase.moc"
