//
// Amarok BarAnalyzer 3 - Jet Turbine: Symmetric version of analyzer 1
//
// Author: Stanislav Karchebny <berkus@users.sf.net>, (C) 2003
//
// Copyright: like rest of amaroK
//

#include "turbine.h"

void TurbineAnalyzer::analyze( const Scope &s )
{
    eraseCanvas();

//    Analyzer::interpolate( s, m_bands );

    std::vector<float>::const_iterator it( s.begin() );
    for ( uint i = 0, x = 10, y2; i < BAND_COUNT; ++i, ++it, x+=5 )
    {
        //assign pre[log10]'d value
        y2 = uint((*it) * 255);
        y2 = m_lvlMapper[ (y2 > 255) ? 255 : y2 ] / 2; //lvlMapper is array of ints from 0 to height()

        int change = y2 - barVector[i];

        //using the best of Markey's, piggz and Max's ideas on the way to shift the bars
        //we have the following:
        // 1. don't adjust shift when doing small up movements
        // 2. shift large upwards with a bias towards last value
        // 3. fall downwards at a constant pace

        if ( change > 4 )
           //add some dynamics - makes the value slightly closer to what it was last time
           y2 = ( barVector[i] * 2 + y2 ) / 3;
        else if( change < 0 )
           y2 = barVector[i] - 1;


        if ( (int)y2 > roofVector[i] )
        {
            roofVector[i] = (int)y2;
            roofVelocityVector[i] = 1;
        }

        //remember where we are
        barVector[i] = y2;

        //blt the coloured bar
        bitBlt( canvas(), x, height()/2 - y2,
                gradient(), y2 * 4, height() - y2, 4, y2, Qt::CopyROP );
        bitBlt( canvas(), x, height()/2,
                gradient(), y2 * 4, height() - y2, 4, y2, Qt::CopyROP );
        //blt the roof bar
        bitBlt( canvas(), x, height()/2 - roofVector[i] - 2, &m_pixRoof[0] );
        bitBlt( canvas(), x, height()/2 + roofVector[i] + 2, &m_pixRoof[0] );

        //set roof parameters for the NEXT draw
        if ( roofVelocityVector[i] != 0 )
        {
            if ( roofVelocityVector[i] > ROOF_HOLD_TIME )
            {
               roofVector[i] -= roofVelocityVector[i] / ROOF_VELOCITY_REDUCTION_FACTOR;
            }

            if ( roofVector[i] < 0 )
            {
               roofVector[i] = 0; //not strictly necessary
               roofVelocityVector[i] = 0;
            }
            else ++roofVelocityVector[i];
        }
    }
}
