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
/*
static uint levelMap50[256] = {
                                0,1,2,3,4,8,9,10,12,14,18,18,20,20,20,20,
                                21,21,21,21,21,21,21,22,22,22,22,22,22,22,22,22,
                                23,23,23,23,23,23,23,24,24,24,24,24,24,24,24,24,
                                25,25,25,25,25,25,25,26,26,26,26,26,26,26,26,26,
                                27,27,27,27,27,27,27,28,28,28,28,28,28,28,28,28,
                                29,29,29,29,29,29,29,30,30,30,30,30,30,30,30,30,
                                31,31,31,31,31,31,31,32,32,32,32,32,32,32,32,32,
                                33,33,33,33,33,33,33,34,34,34,34,34,34,34,34,34,
                                35,35,35,35,35,35,35,36,36,36,36,36,36,36,36,36,
                                37,37,37,37,37,37,37,38,38,38,38,38,38,38,38,38,
                                39,39,39,39,39,39,39,40,40,40,40,40,40,40,40,40,
                                41,41,41,41,41,41,41,41,42,42,42,42,42,42,42,42,
                                43,43,43,43,43,43,43,43,44,44,44,44,44,44,44,44,
                                45,45,45,45,45,45,45,45,46,46,46,46,46,46,46,46,
                                47,47,47,47,47,47,47,47,48,48,48,48,48,48,48,48,
                                49,49,49,49,49,49,49,49,50,50,40,50,50,50,50,50,
                              };
*/

static uint linearLevelMap[50] = {0,1,2,3,4,5,6,7,8,9,
			      10,11,12,13,14,15,16,17,18,19,
			      20,21,22,23,24,25,26,27,28,29,
			      30,31,32,33,34,35,36,37,38,39,
			      40,41,42,43,44,45,46,47,48,49
			      };

static uint logLevelMap[50] = {0,3,9,12,15,18,20,22,24,26,
			      28,30,31,32,33,34,35,36,37,38,
			      38,39,39,40,40,41,41,42,42,43,
			      43,44,44,44,45,45,45,46,46,46,
			      47,47,47,48,48,48,49,49,49,49
			      };

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

  m_pRoofPixmap = new QPixmap( 4, 1 );
  m_pRoofPixmap->fill( QColor( 0xff, 0x50, 0x70 ) );

  QPainter p( m_pSrcPixmap );
  p.setBackgroundColor( Qt::black );
  p.eraseRect( 0, 0, m_pSrcPixmap->width(), m_pSrcPixmap->height() );

  for ( uint x=0, r=0x40, g=0x30, b=0xff; x < height(); ++x )
  {
    for ( int y = x; y > 0; --y )
    {
      p.setPen( QPen( QColor( r + (+y*4), g+ (+y*0), b+ (-y*4) ) ) );
      p.drawLine( x * 4, height() - y, x * 4 + 4, height() - y );
    }
  }

  for (uint i = 0; i < BAND_COUNT; i++)
  {
    barArray[i] = 0;
    peakArray[i].level = 0;
    peakArray[i].delay = 0;
  }
}


// --------------------------------------------------------------------------------

void BarAnalyzer2::drawAnalyzer( std::vector<float> *s )
{
  uint newval;
  int change;
  bool demo;
  
  std::vector<float> bands(BAND_COUNT, 0);

  if ( s )
  {
    demo = false;
    interpolate( s, bands ); //if no s then we are paused/stopped
  }
  else
  {
    demo = true;
    bands = demoData();
  }
  
  bitBlt( m_pComposePixmap, 0, 0, grid() ); //start with a blank canvas

  for ( uint i = 0, x = 10; i < BAND_COUNT; ++i, x+=5 )
  {
    //note: values in bands can be greater than 1!
    newval = uint(bands[i] * 49);
    newval = logLevelMap[ (newval > 49) ? 49 : newval ];
    change = newval - barArray[i];

    if ( change < 0) //Going Down
    {
      if (change < MAX_DECREASE)
      {
        change = MAX_DECREASE;
      }
      barArray[i] += change;
    }
    else //Going Up
    {
      if(change > MAX_INCREASE)
      {
        change = MAX_INCREASE;
      }

      //A bit of a hack for demo mode
      if (demo)
      {
      	change = newval - barArray[i];
      }

      barArray[i] += change;
      if (barArray[i] > height() - 2)
      {
        barArray[i] = height() - 2;
      }
    }

    //Peak Code
    if (barArray[i] > peakArray[i].level)
    {
      peakArray[i].level = barArray[i];
      if(!demo)
      	peakArray[i].delay = 15;
      else
        peakArray[i].delay = 5;
    }

    if (peakArray[i].delay > 0)
    {
      peakArray[i].delay--;
    }

    if (peakArray[i].level > 1)
    {
      if (!peakArray[i].delay)
      {
        peakArray[i].level--;
      }
    }

    //blt the coloured bar
    bitBlt( m_pComposePixmap, x, height() - barArray[i], m_pSrcPixmap, barArray[i] * 4, height() - barArray[i], 4, barArray[i], Qt::CopyROP );
    //blt the roof bar
    bitBlt( m_pComposePixmap, x, height() - peakArray[i].level, m_pRoofPixmap );
  }

  bitBlt( this, 0, 0, m_pComposePixmap );
}


//A cool little demo mode when no music is playing
//Maybe make configurable/make a way to switch it off
std::vector<float> BarAnalyzer2::demoData()
{
static std::vector<float> dd;
static bool init;
static uint count;
static bool reverse;
static bool update;

if (!init)
{
	dd.resize(BAND_COUNT, 0);
	count = 0;
	init = true;
	reverse = false;
	update = true;
}

if (update)
{
dd[count] = 0;
dd[BAND_COUNT - count] = 0;

if (!reverse)
{
	dd[count + 1] = 0.1;
	dd[BAND_COUNT - count - 1] = 0.1;
	count++;
}
else
{

	dd[count - 1] = 0.1;
	dd[BAND_COUNT - count + 1] = 0.1;
	count--;
}	

if (!reverse && count >= BAND_COUNT-1)
	reverse = true;
else if (reverse && count <= 1)
	reverse = false;

update = false;
}
else
{
	update = true;
}		
return dd;
}
#include "baranalyzer2.moc"
