//
//
// C++ Implementation: $MODULE$
//
// Description:
//
//
// Author: Mark Kretschmann <markey@web.de>, (C) 2003
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "baranalyzer.h"
#include <vector>
#include <math.h>
#include <qpainter.h>
#include <qpixmap.h>


BarAnalyzer::BarAnalyzer( QWidget *parent, const char *name )
    : AnalyzerBase2d( 10, parent, name )
    , m_pSrcPixmap( 0 )
    , m_pComposePixmap( 0 )
    , m_roofPixmap( 4, 1 )
{}


BarAnalyzer::~BarAnalyzer()
{
    delete m_pSrcPixmap;
    delete m_pComposePixmap;
}


// METHODS =====================================================
#include <kdebug.h>
void BarAnalyzer::init()
{
    double F = double(height() - 2) / (log10( 255 ) * MAX_AMPLITUDE);

    //generate a list of values that express amplitudes in range 0-MAX_AMP as ints from 0-height() on log scale
    for( uint x = 0; x < 256; ++x )
    {
        m_lvlMapper[x] = uint( F * log10( x+1 ) );
    }

    m_pSrcPixmap = new QPixmap( height()*4, height() );
    m_pComposePixmap = new QPixmap( width(), height() );

    //FIXME obsolete
    m_roofPixmap.fill( QColor( 0xff, 0x50, 0x70 ) );

    QColor col( 0xff, 0x50, 0x70 );

    for ( int i = 0; i < NUM_ROOFS; ++i )
    {
        m_roofPixmaps[i].resize( 4, 1 );
        m_roofPixmaps[i].fill( col.dark( i + 100 + i * 5  ) );
    }

    QPainter p( m_pSrcPixmap );
    p.setBackgroundColor( Qt::black );
    p.eraseRect( 0, 0, m_pSrcPixmap->width(), m_pSrcPixmap->height() );

    for ( uint x=0, r=0x40, g=0x30, b=0xff, r2=255-r;
          x < height();
          ++x )
    {
        for ( int y = x; y > 0; --y )
        {
            double fraction = (double)y / (double)height();

//          p.setPen( QColor( r + (int)(r2  * fraction), g, b - (int)(255 * fraction) ) );
            p.setPen( QColor( r + (int)(r2  * fraction), g, b ) );
            p.drawLine( x * 4, height() - y, x * 4 + 4, height() - y );
        }
    }
}


void BarAnalyzer::drawAnalyzer( std::vector<float> *s )
{
    static std::vector<uint> barVector( BAND_COUNT, 0 );
    static std::vector<int>  roofVector( BAND_COUNT, 50 ); //can't risk uint //FIXME 50 is arbituary!
    static std::vector<uint> roofVelocityVector( BAND_COUNT, ROOF_VELOCITY_REDUCTION_FACTOR );

    bitBlt( m_pComposePixmap, 0, 0, grid() ); //start with a blank canvas

    std::vector<float> bands( BAND_COUNT, 0 );
    std::vector<float>::const_iterator it( bands.begin() );

    if ( s )
       interpolate( s, bands ); //if no s then there is no playback, let the bars fall to base

    for ( uint i = 0, x = 10, y2; i < bands.size(); ++i, ++it, x+=5 )
    {
        //assign pre[log10]'d value
        y2 = uint((*it) * 256); //256 is optimised to bitshift
        y2 = m_lvlMapper[ (y2 > 255) ? 255 : y2 ]; //lvlMapper is array of ints with values 0 to height()

        int change = y2 - barVector[i];

        //using the best of Markey's, piggz and Max's ideas on the way to shift the bars
        //we have the following:
        // 1. don't adjust shift when doing small up movements
        // 2. shift large upwards with a bias towards last value
        // 3. fall downwards at a constant pace

        if ( change > 2 ) //anything too much greater than 2 gives "jitter"
           //add some dynamics - makes the value slightly closer to what it was last time
           y2 = ( barVector[i] * 2 + y2 ) / 3;
        else if( change < -1 )
           y2 = barVector[i] - 1;


        if ( (int)y2 > roofVector[i] )
        {
            roofVector[i] = (int)y2;
            roofVelocityVector[i] = 1;
        }

        //remember where we are
        barVector[i] = y2;

        if ( m_roofMem[i].size() > NUM_ROOFS )
            m_roofMem[i].erase( m_roofMem[i].begin() );

        //blt last n roofs, a.k.a motion blur
        for ( uint c = 0; c < m_roofMem[i].size(); ++c )
            //bitBlt( m_pComposePixmap, x, m_roofMem[i]->at( c ), m_roofPixmaps[ c ] );
            bitBlt( m_pComposePixmap, x, m_roofMem[i].at( c ), &m_roofPixmaps[ NUM_ROOFS - 1 - c ] );

        //blt the coloured bar
        bitBlt( m_pComposePixmap, x, height() - y2,
                m_pSrcPixmap, y2 * 4, height() - y2, 4, y2, Qt::CopyROP );

        m_roofMem[i].push_back( height() - roofVector[i] - 2 );

        //set roof parameters for the NEXT draw
        if ( roofVelocityVector[i] != 0 )
        {
            if ( roofVelocityVector[i] > 32 ) //no reason to do == 32
                roofVector[i] -= (roofVelocityVector[i] - 32) / 20; //trivial calculation

            if ( roofVector[i] < 0 )
            {
               roofVector[i] = 0; //not strictly necessary
               roofVelocityVector[i] = 0;
            }
            else ++roofVelocityVector[i];
        }
    }

    bitBlt( this, 0, 0, m_pComposePixmap );
}

#include "baranalyzer.moc"
