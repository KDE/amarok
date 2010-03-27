/****************************************************************************************
 * Copyright (c) 2003 Stanislav Karchebny <berkus@users.sf.net>                         *
 * Copyright (c) 2003 Max Howell <max.howell@methylblue.com>                            *
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

// Amarok BarAnalyzer 3 - Jet Turbine: Symmetric version of analyzer 1

#include "turbine.h"

#include "core/support/Amarok.h"

#include <QPainter>

#include <cmath>



void TurbineAnalyzer::analyze( const Scope &scope )
{
    eraseCanvas();

    QPainter p( canvas() );
    float h;
    const uint hd2 = height() / 2;
    const uint MAX_HEIGHT = hd2 - 1;

    for( uint i = 0, x = 0, y; i < BAND_COUNT; ++i, x += COLUMN_WIDTH+1 )
    {
        h = log10( scope[i]*256.0 ) * F * 0.5;

        if( h > MAX_HEIGHT )
           h = MAX_HEIGHT;

        if( h > bar_height[i] )
        {
            bar_height[i] = h;

            if( h > peak_height[i] )
            {
                peak_height[i] = h;
                peak_speed[i]  = 0.01;
            }
            else goto peak_handling;
        }
        else
        {
            if( bar_height[i] > 0.0 )
            {
                bar_height[i] -= K_barHeight; //1.4
                if( bar_height[i] < 0.0 ) bar_height[i] = 0.0;
            }

        peak_handling:

            if( peak_height[i] > 0.0 )
            {
                peak_height[i] -= peak_speed[i];
                peak_speed[i]  *= F_peakSpeed; //1.12

                if( peak_height[i] < bar_height[i] ) peak_height[i] = bar_height[i];
                if( peak_height[i] < 0.0 ) peak_height[i] = 0.0;
            }
        }


        y = hd2 - uint(bar_height[i]);
        bitBlt( canvas(), x+1, y,   &barPixmap, 0, y );
        bitBlt( canvas(), x+1, hd2, &barPixmap, 0, (int)bar_height[i] );

        p.setPen( Amarok::ColorScheme::Foreground );
        p.drawRect( x, y, COLUMN_WIDTH, (int)bar_height[i]*2 );

        const uint x2 = x+COLUMN_WIDTH-1;
        p.setPen( Amarok::ColorScheme::Text );
        y = hd2 - uint(peak_height[i]);
        p.drawLine( x, y, x2, y );
        y = hd2 + uint(peak_height[i]);
        p.drawLine( x, y, x2, y );
    }
}
