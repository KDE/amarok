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

#include <kapplication.h>
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

#define NDOTS 15
#define WAVESIZE 15

#define KRAND(x) (kapp->random() % (x))


void XmasAnalyzer::drawStar( QPainter &p, int x, int y, QColor startColor )
{
   p.setPen(startColor);
   p.drawPoint(x,y);
   p.setPen( QColor(startColor.red() - 15, startColor.green() - 15, startColor.blue() - 15) );
   p.drawPoint(x-1,y);
   p.drawPoint(x,y-1);
   p.drawPoint(x+1,y);
   p.drawPoint(x,y+1);
   p.setPen( QColor(startColor.red() - 35, startColor.green() - 35, startColor.blue() - 35) );
   p.drawPoint(x-2,y);
   p.drawPoint(x,y-2);
   p.drawPoint(x+2,y);
   p.drawPoint(x,y+2);
}

void XmasAnalyzer::drawAnalyzer( std::vector<float> *s )
{
   int x2;
   static int wave1pos = -WAVESIZE, wave2pos = -WAVESIZE, wavecounter = 0;
   static bool waving = false;
   static QColor wave1col, wave2col;

   bitBlt( m_pComposePixmap, 0, 0, grid() ); //start with a blank canvas

   QPainter p( m_pSantaPixmap );
   QPainter q( m_pComposePixmap );

   std::vector<float> bands( BAND_COUNT, 0 );
   std::vector<float>::const_iterator it( bands.begin() );

   if ( s )
      interpolate( s, bands );

   // splash some waves
   if ((++wavecounter > 300) && !waving)
   {
      wave1pos = m_pComposePixmap->width() - 1;
      wave2pos = wave1pos + 45;
      wave1col = QColor(0xff - KRAND(0x80), 0xff - KRAND(0x80), 0xff - KRAND(0x80));
      wave2col = QColor(0xff - KRAND(0x80), 0xff - KRAND(0x80), 0xff - KRAND(0x80));
      waving = true;
   }

   if ((wave1pos < m_pComposePixmap->width()) && (wave1pos > -WAVESIZE))
   {
      for ( int i = 0; (i < WAVESIZE) && (i < m_pComposePixmap->width() - wave1pos); i++ )
      {
         QColor c = QColor( wave1col.red() - i * 2, wave1col.green() - i * 3, wave1col.blue() - i * 2 );
         q.setPen( c );
         q.drawLine( wave1pos + i, 0, wave1pos + i, m_pComposePixmap->height() );
      }
   }

   if ((wave2pos < m_pComposePixmap->width()) && (wave2pos > -WAVESIZE))
   {
      for ( int i = 0; (i < WAVESIZE) && (i < m_pComposePixmap->width() - wave2pos); i++ )
      {
         QColor c = QColor( wave2col.red() - i * 3, wave2col.green() - i * 2, wave2col.blue() - i * 2 );
         q.setPen( c );
         q.drawLine( wave2pos + i, 0, wave2pos + i, m_pComposePixmap->height() );
      }
   }

   if (wave2pos <= -WAVESIZE)
   {
      waving = false;
      wavecounter = 0;
   }

   if (waving)
   {
      wave1pos--;
      wave2pos--;
   }

   // paint some random disco stars too =)
   if (waving)
      for ( uint i = 0; i < NDOTS; i++)
      {
         drawStar( q, KRAND(m_pComposePixmap->width() - 5), KRAND(m_pComposePixmap->height() -5), QColor(0xff - KRAND(0x80), 0xff - KRAND(0x80), 0xff - KRAND(0x80)) );
      }

   // and let deers run...
   for ( uint i = 0; i < bands.size(); ++i, ++it )
   {
      x2  = uint((*it) * 255);
      x2 = m_levelToX[ (x2 > 255) ? 255 : x2 ];

      p.fillRect( p.window(), QColor(0x40 + (+x2*4), 0x30 + x2, 0xff + (-x2*3)) );

      bitBlt( m_pComposePixmap, x2, 0,
              m_pSantaPixmap );
   }

   // let santa jump

   bitBlt( this, 0, 0, m_pComposePixmap );
}

#include "xmasdrug.moc"
