/***************************************************************************
                          spectralshine.cpp  -  SpectralShine analyzer
                             -------------------
    begin                : Dec 4 2003
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

#include "spectralshine.h"
#include <vector>
#include <math.h>
#include <qpainter.h>
#include <qpixmap.h>

// FIXME: WIP

#undef BAND_COUNT
#undef MAX_AMPLITUDE

#define BAND_COUNT 31
#define MAX_AMPLITUDE 1.1


SpectralShineAnalyzer::SpectralShineAnalyzer( QWidget *parent )
   : Analyzer::Base2d( parent, 8 )
   , m_pSrcPixmap( 0 )
{
}


SpectralShineAnalyzer::~SpectralShineAnalyzer()
{
   delete m_pSrcPixmap;
}


// METHODS =====================================================

void SpectralShineAnalyzer::init()
{
   // Fill level-to-X mapping table
   const double Fx = double(width() - 2) / 2;

   for( uint x = 0; x < 256; ++x )
   {
      m_levelToX[x] = uint( Fx * sin ( double(x - 128) / 128.0 ) + Fx );
   }

   // Fill level-to-Y mapping table
   const double Fy = double(height() - 2) / (log10( 255 ) * MAX_AMPLITUDE);

   for( uint y = 0; y < 256; ++y )
   {
      m_levelToY[y] = uint( Fy * log10( y+1 ) );
   }

   m_pSrcPixmap = new QPixmap( width(), height() );

   QPainter p( m_pSrcPixmap );
   p.setBackgroundColor( Qt::black );
   p.eraseRect( 0, 0, m_pSrcPixmap->width(), m_pSrcPixmap->height() );

   for ( uint x=0, r=0x40, g=0x30, b=0xff; x < height(); ++x )
   {
      p.setPen( QPen( QColor( r + (+x*4), g+ (+x*0), b+ (-x*4) ) ) );
      p.drawLine( 0, x, width(), x );
   }
}


void SpectralShineAnalyzer::drawAnalyzer( std::vector<float> *s )
{
   eraseBackground();

   std::vector<float> bands( BAND_COUNT, 0 );
   std::vector<float>::const_iterator it( bands.begin() );

   if ( s )
      interpolate( s, bands );

   for ( uint i = 0, x = 10, x2, y2; i < bands.size(); ++i, ++it, x+=5 )
   {
      x2 = uint((*it) * 255);
      x2 = m_levelToX[ (x2 > 255) ? 255 : x2 ];
      y2 = uint((*it) * 255);
      y2 = m_levelToY[ (y2 > 255) ? 255 : y2 ];

      bitBlt( canvas(), x2, y2,
               m_pSrcPixmap, x2, y2, 2, 2, Qt::CopyROP );
      bitBlt( canvas(), width() - x2, y2,
               m_pSrcPixmap, width() - x2, y2, 2, 2, Qt::CopyROP );
      bitBlt( canvas(), x2, height() - y2,
               m_pSrcPixmap, x2, height() - y2, 2, 2, Qt::CopyROP );
      bitBlt( canvas(), width() - x2, height() - y2,
               m_pSrcPixmap, width() - x2, height() - y2, 2, 2, Qt::CopyROP );
   }
}

#include "spectralshine.moc"
