/***************************************************************************
                          visdistortwidget.cpp  -  description
                             -------------------
    begin                : Oct 27 2003
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

#include "distortanalyzer.h"

#include <math.h>
#include <vector>

#include <qpainter.h>
#include <qpixmap.h>
#include <qwidget.h>

#include <kdebug.h>
#include <kstandarddirs.h>


DistortAnalyzer::DistortAnalyzer( QWidget *parent, const char *name ) :
   AnalyzerBase( 30, parent, name ),
   m_pSrcPixmap( 0 ),
   m_pComposePixmap( 0 ),
   m_pComposePixmap1( 0 )
{}


DistortAnalyzer::~DistortAnalyzer()
{
    delete m_pSrcPixmap;
    delete m_pComposePixmap;
    delete m_pComposePixmap1;
}


//METHODS =====================================================

void DistortAnalyzer::init()
{
    m_pSrcPixmap = new QPixmap( locate( "data", "amarok/images/logo_web.png" ) );
    m_pComposePixmap  = new QPixmap( size() );
    m_pComposePixmap1 = new QPixmap( size() );

    initSin( m_sinVector );
}


// --------------------------------------------------------------------------------

void DistortAnalyzer::drawAnalyzer( std::vector<float> *s )
{
    bitBlt( m_pComposePixmap, 0, 0, m_pGridPixmap );
    bitBlt( m_pComposePixmap1, 0, 0, m_pGridPixmap );

    if ( s )                                          // don't bother if vector is empty
    {
        std::vector<float>::const_iterator it;
        std::vector<float> sNew( width() );
        interpolate( s, sNew );
        
        // HORIZONTAL:
                
        it = sNew.begin();
        int sinIndex;
        
        for ( int x = 0; x < width(); x++ )
        {
            sinIndex = static_cast<int>( (*it) * SINVEC_SIZE ) % SINVEC_SIZE;
            
            bitBlt( m_pComposePixmap1, x, 0, m_pSrcPixmap, x,
                    static_cast<int>( m_sinVector[sinIndex] * 20 ), 1, height(), Qt::CopyROP );

            ++it;
        }

        // VERTICAL:
        
        it = sNew.begin();

        // jump to middle of vector -> more interesting values
        for ( unsigned int i = 0; i < sNew.size() / 2; i++, ++it )
        {}

        for ( uint y = 0; y < height(); ++y )
        {
            sinIndex = static_cast<int>( (*it) * SINVEC_SIZE ) % SINVEC_SIZE;
            
            bitBlt( m_pComposePixmap, 0, y, m_pComposePixmap1,
                    static_cast<int>( m_sinVector[sinIndex] * 14 - 15 ),  y, width(), 1, Qt::CopyROP );

           ++it;
        }
    }
    bitBlt( this, 0, 0, m_pComposePixmap );
}


#include "distortanalyzer.moc"
