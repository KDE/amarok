/***************************************************************************
                          baranalyzer2.cpp  -  description
                             -------------------
    begin                : Die Jan 7 2003
    copyright            : (C) 2003 by Adam Pigg
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

#include "baranalyzer2.h"

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

#define MAX_INCREASE 8
#define MAX_DECREASE -3

BarAnalyzer2::BarAnalyzer2( QWidget *parent, const char *name ) :
    AnalyzerBase( 30, parent, name ),
    m_pBgPixmap( 0 ),
    m_pSrcPixmap( 0 ),
    m_pComposePixmap( 0 ),
    m_pRoofPixmap( 0 )
{}


BarAnalyzer2::~BarAnalyzer2()
{
  delete m_pBgPixmap;
  delete m_pSrcPixmap;
  delete m_pComposePixmap;
  delete m_pRoofPixmap;
}


// METHODS =====================================================

void BarAnalyzer2::init()
{
  m_pSrcPixmap = new QPixmap( height()*4, height() );
  m_pComposePixmap = new QPixmap( size() );

  m_pRoofPixmap = new QPixmap( 1, 1 );
  m_pRoofPixmap->fill( QColor( 0xff, 0x50, 0x70 ) );

  QPainter p( m_pSrcPixmap );
  p.setBackgroundColor( Qt::black );
  p.eraseRect( 0, 0, m_pSrcPixmap->width(), m_pSrcPixmap->height() );

  m_lvlMap.resize(50, 0);
  m_barArray.resize(width() - 20, 0);
  m_peakArray.resize(width() - 20);
  
  //generate a list of values that express amplitudes in range 0-MAX_AMP as ints from 0-height() on log scale
  m_lvlMap[0] = 0;
  for( uint x = 1; x < 50; x++ )
  {
    m_lvlMap[x] = uint(-(((x-50)*(x-50))/50)+50-1);
    kdDebug() << "X=" << x << " Y=" << m_lvlMap[x] << endl;
  }
    
  for ( uint x=0, r=0x40, g=0x30, b=0xff; x < height(); ++x )
  {
    for ( int y = x; y > 0; --y )
    {
      p.setPen( QPen( QColor( r + (+y*4), g+ (+y*0), b+ (-y*4) ) ) );
      p.drawLine( x * 4, height() - y, x * 4, height() - y );
    }
  }
}


// --------------------------------------------------------------------------------

void BarAnalyzer2::drawAnalyzer( std::vector<float> *s )
{
  uint newval;
  int change;
  
  std::vector<float> bands(width() - 20, 0);

  if ( s )
  {
    interpolate( s, bands ); //if no s then we are paused/stopped
  }
  
  bitBlt( m_pComposePixmap, 0, 0, grid() ); //start with a blank canvas

  for ( uint i = 0, x = 10; i < bands.size(); ++i, x++ )
  {
    //note: values in bands can be greater than 1!
    newval = uint(bands[i] * 49);
    newval = m_lvlMap[newval];
    change = newval - m_barArray[i];
    if (change < MAX_DECREASE)
    {
      change = MAX_DECREASE;
    }
    else if(change > MAX_INCREASE)
    {
      change = MAX_INCREASE;
    }
    m_barArray[i] += change;
    
    if (m_barArray[i] > height() - 1)
    {
      m_barArray[i] = height() - 1;
    }

    //Peak Code
    if (m_barArray[i] > m_peakArray[i].level)
    {
      m_peakArray[i].level = m_barArray[i];
        m_peakArray[i].delay = 5;
    }

    if (m_peakArray[i].delay > 0)
    {
      m_peakArray[i].delay--;
    }

    if (m_peakArray[i].level > 1)
    {
      if (!m_peakArray[i].delay)
      {
        m_peakArray[i].level--;
      }
    }

    //blt the coloured bar
    bitBlt( m_pComposePixmap, x, height() - m_barArray[i], m_pSrcPixmap, m_barArray[i] * 4, height() - m_barArray[i], 4, m_barArray[i], Qt::CopyROP );
    //blt the roof bar
    bitBlt( m_pComposePixmap, x, height() - m_peakArray[i].level, m_pRoofPixmap );
  }

  bitBlt( this, 0, 0, m_pComposePixmap );
}

#include "baranalyzer2.moc"
