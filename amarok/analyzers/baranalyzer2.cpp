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
#include <qpainter.h>

#define MAX_INCREASE 8
#define MAX_DECREASE -3


BarAnalyzer2::BarAnalyzer2( QWidget *parent ) :
    Analyzer::Base2D( parent, 30 ),
    m_pBgPixmap( 0 ),
    m_pSrcPixmap( 0 ),
    m_pRoofPixmap( 0 )
{}


BarAnalyzer2::~BarAnalyzer2()
{
  delete m_pBgPixmap;
  delete m_pSrcPixmap;
  delete m_pRoofPixmap;
}


// METHODS =====================================================

void BarAnalyzer2::init()
{
  m_pSrcPixmap  = new QPixmap( height()*4, height() );
  m_pRoofPixmap = new QPixmap( 1, 1 );
  m_pRoofPixmap->fill( QColor( 0xff, 0x50, 0x70 ) );

  QPainter p( m_pSrcPixmap );
  p.setBackgroundColor( Qt::black );
  p.eraseRect( 0, 0, m_pSrcPixmap->width(), m_pSrcPixmap->height() );

  m_lvlMap.resize(height() , 0);
  m_barArray.resize(width() - 20, 0);
  m_bands.resize(width() - 20, 0);
  m_peakArray.resize(width() - 20);

  //Maybe use this in the future
  //A frequency level mapper to boost frequencies for display purposes
  /*m_freqMap.resize(width() - 20, 0);

  for (uint i = 0; i < uint(width() - 20); i++)
  {
  	m_freqMap[i] = (((0/(width() - 20)) * i) + 1);
  }*/

  //generate a list of values that express amplitudes in range 0-MAX_AMP as ints from 0-height() on square scale
  m_lvlMap[0] = 0;
  for( uint x = 1; x < height(); x++ )
  {
    m_lvlMap[x] = uint(-(((x-height())*(x-height()))/height())+height()-1);
  }

  for ( uint x=0, r=0x40, g=0x30, b=0xff; x < height(); ++x )
  {
    for ( int y = x; y > 0; --y )
    {
      p.setPen( QColor( r + (+y*4), g+ (+y*0), b+ (-y*4) ) );
      p.drawLine( x * 4, height() - y, x * 4, height() - y );
    }
  }
}


// --------------------------------------------------------------------------------

void BarAnalyzer2::drawAnalyzer( std::vector<float> *s )
{
  uint newval;
  int change;

  if ( s )
  {
    Analyzer::interpolate( s, m_bands ); //if no s then we are paused/stopped
  }

  eraseCanvas();

  for ( uint i = 0, x = 10; i < m_bands.size(); ++i, x++ )
  {
    //note: values in bands can be greater than 1!
    newval = uint((m_bands[i] * (height()-1)) /** m_freqMap[i]*/);
    if (newval > height()-1)
      newval = height() - 1;

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
    bitBlt( canvas(), x, height() - m_barArray[i], m_pSrcPixmap, m_barArray[i] * 4, height() - m_barArray[i], 4, m_barArray[i], Qt::CopyROP );
    //blt the roof bar
    bitBlt( canvas(), x, height() - m_peakArray[i].level, m_pRoofPixmap );
  }
}
