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


DistortAnalyzer::DistortAnalyzer( QWidget *parent, const char *name )
        : AnalyzerBase( 30, parent, name )
        , m_pComposePixmap( 0 )
        , m_pComposePixmap1( 0 )
{}


DistortAnalyzer::~DistortAnalyzer()
{
    delete m_pComposePixmap;
    delete m_pComposePixmap1;

    for ( int i = 0; i < NUM_PIXMAPS; i++ )
        delete m_srcPixmaps[i];
}


//METHODS =====================================================

void DistortAnalyzer::init()
{
    m_pComposePixmap  = new QPixmap( size() );
    m_pComposePixmap1 = new QPixmap( size() );

    initSin( m_sinVector );

    //init colored source pixmaps:
        
    m_srcPixmaps.resize( NUM_PIXMAPS );
    QPixmap srcPixmap( locate( "data", "amarok/images/logo_web.png" ) );    
            
    for ( int i = 0; i < NUM_PIXMAPS; i++ )
    {
        m_srcPixmaps[i] = new QPixmap( srcPixmap );
        m_srcPixmaps[i]->fill( QColor( i*8, i*2, 0xc0 - i*3 ) );
    }

    m_backupVector.push_back( 10 );
}


// --------------------------------------------------------------------------------

void DistortAnalyzer::drawAnalyzer( std::vector<float> *s )
{
    bitBlt( m_pComposePixmap, 0, 0, grid() );
    bitBlt( m_pComposePixmap1, 0, 0, grid() );

    if ( s ) // don't bother if vector is empty
    {
        m_backupVector = *s;
        std::vector<float>::const_iterator it, it1;
        std::vector<float> sNew( width() );
        interpolate( s, sNew );

        // VERTICAL:
                
        it = sNew.begin();
        it1 = sNew.end();
        int sinIndex, pixIndex;
        
        for ( int x = 0; x < width(); x++ )
        {
            sinIndex = static_cast<int>( ((*it)+(*it1)) * SINVEC_SIZE );
            pixIndex = static_cast<int>( ( m_sinVector[ checkIndex( sinIndex, m_sinVector.size() ) ] + 1.0 )
                                         / 2  * (NUM_PIXMAPS-1) );
            
            sinIndex = static_cast<int>( (*it) * SINVEC_SIZE );
            bitBlt( m_pComposePixmap1, x, 0,
                    m_srcPixmaps[ checkIndex( pixIndex, m_srcPixmaps.size() ) ],
                    x, static_cast<int>( m_sinVector[ checkIndex( sinIndex, m_sinVector.size() ) ] * 20 - 20 ),
                    1, height() );

            ++it;
           --it1;
        }

        // HORIZONTAL:
        
        it = sNew.begin();

        // jump to middle of vector -> more interesting values
        for ( unsigned int i = 0; i < sNew.size() / 2; i++, ++it )
        {}

        for ( uint y = 0; y < height(); ++y )
        {
            sinIndex = static_cast<int>( (*it) * SINVEC_SIZE );
            
            bitBlt( m_pComposePixmap, 0, y, m_pComposePixmap1,
                    static_cast<int>( m_sinVector[ checkIndex( sinIndex, m_sinVector.size() ) ] * 14 - 15 ),  y,
                    width(), 1 );

           ++it;
        }

        bitBlt( this, 0, 0, m_pComposePixmap );

    }
}


void DistortAnalyzer::paintEvent( QPaintEvent * )
{
    drawAnalyzer( &m_backupVector );
}


int DistortAnalyzer::checkIndex( int index, int size )
{
    if ( index < 0 )
        return 0;
    if ( index > size - 1 )
        return size - 1;
    
    return index;
}


#include "distortanalyzer.moc"
