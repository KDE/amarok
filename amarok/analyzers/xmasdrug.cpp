/***************************************************************************
                          xmasdrug.cpp  -  Special Xmas analyzer
                             -------------------
    begin                : Dec 20 2003
    copyright            : (C) 2003 by Stanislav Karchebny
    email                : berk@upnet.ru
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "xmasdrug.h"
#include <vector>
#include <math.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qbitmap.h>

#include <kstandarddirs.h>

// FIXME: WIP

#undef BAND_COUNT
#undef TIMEOUT
#undef MAX_AMPLITUDE

#define BAND_COUNT 7
#define TIMEOUT 60
#define MAX_AMPLITUDE 1.1


XmasAnalyzer::XmasAnalyzer( QWidget *parent, const char *name )
   : AnalyzerBase( TIMEOUT, parent, name )
   , m_pComposePixmap( 0 )
   , m_pBuckPixmap( 0 )
{
}


XmasAnalyzer::~XmasAnalyzer()
{
   delete m_pComposePixmap;
   delete m_pBuckPixmap;
}


// METHODS =====================================================

void XmasAnalyzer::init()
{
   // Fill level-to-X mapping table
   double Fx = double(width() - 2) / 2;

   for( uint x = 0; x < 256; ++x )
   {
      m_levelToX[x] = uint( Fx * sin ( double(x - 128) / 128.0 ) + Fx );
   }

   m_pComposePixmap = new QPixmap( size() );
   m_pBuckPixmap = new QBitmap( locate( "data", "amarok/images/bucky.png" ) );
//   m_pBuckPixmap->setOptimization( QPixmap::BestOptim );
//   m_pSantaPixmap = new QPixmap( locate( "data", "amarok/images/santa.png" ) );
//   m_pSantaPixmap->setOptimization( QPixmap::BestOptim );
   m_pSantaPixmap = new QPixmap( m_pBuckPixmap->width(), m_pBuckPixmap->height() );
   m_pSantaPixmap->setMask(*m_pBuckPixmap);
}


void XmasAnalyzer::drawAnalyzer( std::vector<float> *s )
{
   int x2;

   bitBlt( m_pComposePixmap, 0, 0, grid() ); //start with a blank canvas

   QPainter p( m_pSantaPixmap );
   p.setPen( Qt::red );
   p.eraseRect( p.window() );

   std::vector<float> bands( BAND_COUNT, 0 );
   std::vector<float>::const_iterator it( bands.begin() );

   if ( s )
      interpolate( s, bands );

   for ( uint i = 0; i < bands.size(); ++i, ++it )
   {
      x2 = uint((*it) * 255);
      x2 = m_levelToX[ (x2 > 255) ? 255 : x2 ];

      bitBlt( m_pComposePixmap, x2, 0,
              m_pSantaPixmap );
   }

   bitBlt( this, 0, 0, m_pComposePixmap );
}

#include "xmasdrug.moc"
