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
#include <math.h>
#include <qpainter.h>
#include <qbitmap.h>

#include <kapplication.h>
#include <kstandarddirs.h>

// FIXME: WIP


XmasAnalyzer::XmasAnalyzer( QWidget *parent )
   : Analyzer::Base2D( parent, 60 )
   , m_pBuckPixmap( 0 )
   , m_pSantaPixmap( 0 )
{
}


XmasAnalyzer::~XmasAnalyzer()
{
   delete m_pBuckPixmap;
   delete m_pSantaPixmap;
}


// METHODS =====================================================

void XmasAnalyzer::init()
{
   // Fill level-to-X mapping table
   const double Fx = double(width() - 2) / 2;

   for( uint x = 0; x < 256; ++x )
   {
      m_levelToX[x] = uint( Fx * sin ( double(x - 128) / 128.0 ) + Fx );
   }

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

class QSaturatedColor : public QColor
{
   public:
      QSaturatedColor() : QColor() {}
      QSaturatedColor(int r, int g, int b) : QColor() { setRgb(r, g, b); }
      void setRgb(int r, int g, int b );
};

void QSaturatedColor::setRgb( int r, int g, int b )
{
   r = (r < 0) ? 0 : ((r > 255) ? 255 : r);
   g = (g < 0) ? 0 : ((g > 255) ? 255 : g);
   b = (b < 0) ? 0 : ((b > 255) ? 255 : b);
   QColor::setRgb(r, g, b);
}

void XmasAnalyzer::drawStar( QPainter &p, int x, int y, QColor startColor )
{
   p.setPen(startColor);
   p.drawPoint(x,y);
   p.setPen( QSaturatedColor(startColor.red() - 15, startColor.green() - 15, startColor.blue() - 15) );
   p.drawPoint(x-1,y);
   p.drawPoint(x,y-1);
   p.drawPoint(x+1,y);
   p.drawPoint(x,y+1);
   p.setPen( QSaturatedColor(startColor.red() - 35, startColor.green() - 35, startColor.blue() - 35) );
   p.drawPoint(x-2,y);
   p.drawPoint(x,y-2);
   p.drawPoint(x+2,y);
   p.drawPoint(x,y+2);
}

void XmasAnalyzer::analyze( const Scope &s )
{
   int x2;
   static int wave1pos = -WAVESIZE, wave2pos = -WAVESIZE, wavecounter = 0;
   static bool waving = false;
   static QSaturatedColor wave1col, wave2col;

   eraseCanvas();

   QPainter p( m_pSantaPixmap );
   QPainter q( canvas() );

   std::vector<float> bands( BAND_COUNT, 0 );
   std::vector<float>::const_iterator it( bands.begin() );

   Analyzer::interpolate( s, bands );

   // splash some waves
   if ((++wavecounter > 300) && !waving)
   {
      wave1pos = canvas()->width() - 1;
      wave2pos = wave1pos + 45;
      wave1col = QSaturatedColor(0xff - KRAND(0x80), 0xff - KRAND(0x80), 0xff - KRAND(0x80));
      wave2col = QSaturatedColor(0xff - KRAND(0x80), 0xff - KRAND(0x80), 0xff - KRAND(0x80));
      waving = true;
   }

   if ((wave1pos < canvas()->width()) && (wave1pos > -WAVESIZE))
   {
      for ( int i = 0; (i < WAVESIZE) && (i < canvas()->width() - wave1pos); i++ )
      {
         QSaturatedColor c = QSaturatedColor( wave1col.red() - i * 2, wave1col.green() - i * 3, wave1col.blue() - i * 2 );
         q.setPen( c );
         q.drawLine( wave1pos + i, 0, wave1pos + i, canvas()->height() );
      }
   }

   if ((wave2pos < canvas()->width()) && (wave2pos > -WAVESIZE))
   {
      for ( int i = 0; (i < WAVESIZE) && (i < canvas()->width() - wave2pos); i++ )
      {
         QSaturatedColor c = QSaturatedColor( wave2col.red() - i * 3, wave2col.green() - i * 2, wave2col.blue() - i * 2 );
         q.setPen( c );
         q.drawLine( wave2pos + i, 0, wave2pos + i, canvas()->height() );
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
         drawStar( q, KRAND(canvas()->width() - 5), KRAND(canvas()->height() -5), QSaturatedColor(0xff - KRAND(0x80), 0xff - KRAND(0x80), 0xff - KRAND(0x80)) );
      }

   // and let deers run...
   for ( uint i = 0; i < bands.size(); ++i, ++it )
   {
      x2  = uint((*it) * 255);
      x2 = m_levelToX[ (x2 > 255) ? 255 : x2 ];

      p.fillRect( p.window(), QSaturatedColor(0x40 + (+x2*4), 0x30 + x2, 0xff + (-x2*3)) );

      bitBlt( canvas(), x2, 0,
              m_pSantaPixmap );
   }
}
