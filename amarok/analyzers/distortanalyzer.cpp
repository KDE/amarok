/***************************************************************************
                          visdistortwidget.cpp  -  description
                             -------------------
    begin                : Oct 27 2003
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

#include "distortanalyzer.h"

#include <math.h>
#include <vector>

#include <qbitmap.h>
#include <qcolor.h>
#include <qimage.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qwidget.h>

#include <kdebug.h>
#include <kstandarddirs.h>

#define NUM_PIXMAPS 32


DistortAnalyzer::DistortAnalyzer( QWidget *parent )
        : Analyzer::Base2D( parent, 30 )
        , m_pComposePixmap1( 0 )
{}


DistortAnalyzer::~DistortAnalyzer()
{
    delete m_pComposePixmap1;

    for ( int i = 0; i < NUM_PIXMAPS; i++ )
        delete m_srcPixmaps[i];
}


//METHODS =====================================================

void DistortAnalyzer::init()
{
    m_pComposePixmap1 = new QPixmap( size() );

    Analyzer::initSin( m_sinVector );

    //init colored source pixmaps:

    m_srcPixmaps.resize( NUM_PIXMAPS );
    QPixmap srcPixmap( locate( "data", "amarok/images/logo_web.png" ), "PNG" );

    for ( int i = 0; i < NUM_PIXMAPS; i++ )
    {
        m_srcPixmaps[i] = new QPixmap( srcPixmap );
        m_srcPixmaps[i]->setOptimization( QPixmap::BestOptim );
        m_srcPixmaps[i]->fill( QColor( i*8, i*2, 0xc0 - i*3 ) );
    }
}


// --------------------------------------------------------------------------------

void DistortAnalyzer::analyze( const Scope &s )
{
    //start with a blank canvas
    eraseCanvas();
    bitBlt( m_pComposePixmap1, 0, 0, background() );

    std::vector<float>::const_iterator it, it1;
    std::vector<float> sNew( width() );
    Analyzer::interpolate( s, sNew );

    // VERTICAL:

    it =  sNew.begin();
    it1 = sNew.end();
    int sinIndex, pixIndex;

    for ( int x = 0; x < width(); x++ )
    {
        sinIndex = static_cast<int>( ((*it)+(*it1)) * m_sinVector.size() );
        pixIndex = static_cast<int>( ( m_sinVector[ checkIndex( sinIndex, m_sinVector.size() ) ] + 1.0 )
                                        / 2  * (NUM_PIXMAPS-1) );

        sinIndex = static_cast<int>( (*it) * m_sinVector.size() );
        bitBlt( m_pComposePixmap1, x, 0,
                m_srcPixmaps[ checkIndex( pixIndex, m_srcPixmaps.size() ) ],
                x, static_cast<int>( m_sinVector[ checkIndex( sinIndex, m_sinVector.size() ) ] * 21 - 21 ),
                1, height() );

        ++it;
        --it1;
    }

    // HORIZONTAL:

    it = sNew.begin();

//  // jump to middle of vector -> more interesting values
//  for ( unsigned int i = 0; i < sNew.size() / 2; i++, ++it )
//  {}

    for ( uint y = 0; y < height(); ++y )
    {
        sinIndex = static_cast<int>( ( (*it)/3.0 ) * m_sinVector.size() );

        bitBlt( canvas(), 0, y, m_pComposePixmap1,
                static_cast<int>( m_sinVector[ checkIndex( sinIndex, m_sinVector.size() ) ] * 11 - 12 ), y,
                width(), 1 );

        ++it;
    }
}

int DistortAnalyzer::checkIndex( int index, int size )
{
    if ( index < 0 )
        return 0;
    if ( index > size - 1 )
        return size - 1;

    return index;
}
