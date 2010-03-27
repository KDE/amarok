/****************************************************************************************
 * Copyright (c) 2003 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "baranalyzer.h"

#include "core/support/Debug.h"
#include <cmath>     //log10(), etc.
#include <QPainter>

BarAnalyzer::BarAnalyzer( QWidget *parent )
    : Analyzer::Base2D( parent, 30, 8 )
//     , m_bands( BAND_COUNT )
//     , barVector( BAND_COUNT, 0 )
//     , roofVector( BAND_COUNT, 50 )
//     , roofVelocityVector( BAND_COUNT, ROOF_VELOCITY_REDUCTION_FACTOR )
{
    //roof pixmaps don't depend on size() so we do in the ctor

    m_bg = backgroundRole();

    QColor fg( 0xff, 0x50, 0x70 );

    for ( uint i = 0; i < NUM_ROOFS; ++i )
    {
        m_pixRoof[i] = QPixmap( COLUMN_WIDTH, 1 );
        fg.setAlpha( 256 - ( 256 / NUM_ROOFS * i ) );
        m_pixRoof[i].fill( fg );
    }
}

void BarAnalyzer::resizeEvent( QResizeEvent * e )
{
    debug() << "Baranalyzer Resized(" << width() << "x" << height() << ")";
    Analyzer::Base2D::resizeEvent( e );
    init();
}

// METHODS =====================================================

void BarAnalyzer::init()
{
    const double MAX_AMPLITUDE = 1.0;
    const double F = double(height() - 2) / (log10( static_cast<double>(255) ) * MAX_AMPLITUDE );

    setBackgroundRole( m_bg );

    BAND_COUNT = width() / 5;
    MAX_DOWN = int(0 -((height() / 50)));
    MAX_UP = int((height() / 25));

    debug() << "BAND_COUNT = " << BAND_COUNT << " MAX_UP = " << MAX_UP << "MAX_DOWN = " << MAX_DOWN;

    barVector.resize( BAND_COUNT, 0 );
    roofVector.resize( BAND_COUNT, height() -5 );
    roofVelocityVector.resize( BAND_COUNT, ROOF_VELOCITY_REDUCTION_FACTOR );
    m_roofMem.resize(BAND_COUNT);
    m_scope.resize(BAND_COUNT);

    //generate a list of values that express amplitudes in range 0-MAX_AMP as ints from 0-height() on log scale
    for ( uint x = 0; x < 256; ++x )
    {
        m_lvlMapper[x] = static_cast<uint>( ( F * log10( static_cast<float>(x+1) ) ) );
    }

    m_pixBarGradient = QPixmap( height()*COLUMN_WIDTH, height() );

    QPainter p( &m_pixBarGradient );
    for ( int x=0, r=0x40, g=0x30, b=0xff, r2=255-r; x < height(); ++x )
    {
        for ( int y = x; y > 0; --y )
        {
            const double fraction = (double)y / height();

//          p.setPen( QColor( r + (int)(r2 * fraction), g, b - (int)(255 * fraction) ) );
            p.setPen( QColor( r + (int)(r2 * fraction), g, b ) );
            p.drawLine( x*COLUMN_WIDTH, height() - y, (x+1)*COLUMN_WIDTH, height() - y );
        }
    }

    setMinimumSize( QSize( BAND_COUNT * COLUMN_WIDTH, 10 ) );
}


void BarAnalyzer::analyze( const Scope &s )
{
    Analyzer::interpolate( s, m_scope );
    update();
}


void BarAnalyzer::paintEvent( QPaintEvent* )
{
    QPainter p( this );

    for ( uint i = 0, x = 0, y2; i < m_scope.size(); ++i, x+=COLUMN_WIDTH+1 )
    {
        //assign pre[log10]'d value
        y2 = uint(m_scope[i] * 256); //256 will be optimised to a bitshift //no, it's a float
        y2 = m_lvlMapper[ (y2 > 255) ? 255 : y2 ]; //lvlMapper is array of ints with values 0 to height()

        int change = y2 - barVector[i];

        //using the best of Markey's, piggz and Max's ideas on the way to shift the bars
        //we have the following:
        // 1. don't adjust shift when doing small up movements
        // 2. shift large upwards with a bias towards last value
        // 3. fall downwards at a constant pace

        /*if ( change > MAX_UP ) //anything too much greater than 2 gives "jitter"
           //add some dynamics - makes the value slightly closer to what it was last time
           y2 = ( barVector[i] + MAX_UP );
           //y2 = ( barVector[i] * 2 + y2 ) / 3;
        else*/ if ( change < MAX_DOWN )
           y2 = barVector[i] + MAX_DOWN;


        if ( (int)y2 > roofVector[i] )
        {
            roofVector[i] = (int)y2;
            roofVelocityVector[i] = 1;
        }

        //remember where we are
        barVector[i] = y2;

        if ( m_roofMem[i].size() > NUM_ROOFS )
            m_roofMem[i].erase( m_roofMem[i].begin() );

        //Draw last n roofs, a.k.a motion blur
        for ( uint c = 0; c < m_roofMem[i].size(); ++c )
            p.drawPixmap( x, m_roofMem[i][c], m_pixRoof[ NUM_ROOFS - 1 - c ] );

        //Draw the bar
        p.drawPixmap( x, height() - y2, *gradient(), y2 * COLUMN_WIDTH, height() - y2, COLUMN_WIDTH, y2 );

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
            else roofVelocityVector[i] += 2;
        }
    }
}
