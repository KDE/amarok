//
// C++ Implementation: blockanalyzer
//
// Description:
//
//
// Author: Max Howell <max.howell@methylblue.com>, (C) 2003
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "blockanalyzer.h"
#include "fht.h"
#include <math.h>


static float lvlMapper[BlockAnalyzer::ROWS+1];// = { 0.080, 0.140, 0.200, 0.300, 0.500, 0.700, 100 };


BlockAnalyzer::BlockAnalyzer( QWidget *parent )
 : Analyzer::Base2D( parent, 20 )
 , m_dark( WIDTH, HEIGHT )
 , m_store( 16, 0 )
{
    QColor darkColor( backgroundColor().dark( 150 ) );

    m_dark.fill( darkColor );

    double dr = 3*double(darkColor.red()   - 32) / (ROWS*4);
    double dg = 3*double(darkColor.green() - 32) / (ROWS*4);
    double db = 3*double(darkColor.blue()  - 82) / (ROWS*4);

    for( uint x = 0; x < ROWS; ++x )
    {
        m_glow[x].resize( WIDTH, HEIGHT );
        m_glow[x].fill( QColor( 32+dr*x, 32+dg*x, 82+db*x ) ); //amaroK blue, graduated
    }
}

void
BlockAnalyzer::init()
{
    //FIXME adjust height so we fit to smaller toolbars

    const uint bands = (double)width() / (WIDTH+1);
    if( bands < 16 ) m_store.resize( 16 );
    else             m_store.resize( bands );

    std::fill( m_store.begin(), m_store.end(), 0 );


    //make a set of discrete values from 0-1 that the scope amplitude must exceed for that block
    //to be rendered
    //last value is huge to ensure we don't crash by referencing beyond array size
    for( uint x = 2; x < ROWS+2; ++x )
    {
        lvlMapper[ROWS+2-x] = 1-(log10(x) / log10(ROWS+2));
    }
    lvlMapper[ROWS] = 100;

    setMinimumWidth( (m_store.size() + 2) * (WIDTH+1) ); //+2 is 2 blocks margin either side
}

void
BlockAnalyzer::transform( Scope &scope )
{
    scope.resize( scope.size() / 4 );

    float *front = static_cast<float*>( &scope.front() );

    float f[ m_fht.size() ];
    m_fht.copy( &f[0], front );
    m_fht.logSpectrum( front, &f[0] );
    m_fht.scale( front, 1.0 / 20 );
}

void
BlockAnalyzer::analyze( const Scope &s )
{
    // z = 2 3 2 1 0 2
    //     . . . . # .
    //     . . . # # .
    //     # . # # # #
    //     # # # # # #
    //
    // visual aid for how this analyzer works.
    // as can be seen the value of z is from the top in units of blocks

    const int offset = height() - (HEIGHT+1) * ROWS;
    Scope v( m_store.size() + 7 );

    Analyzer::interpolate( s, v );

    eraseCanvas();

    for( uint x = 0; x < m_store.size(); ++x )
    {
        uint z = 0;
        for( ; v[x+7] > lvlMapper[z]; ++z );
        z = ROWS - 1 - z;

        //too high is not fatal
        //higher than stored value means we are falling
        //fall gradually
        //m_store is twice size of regular units so falling is slower
        if( z * 2 > m_store[x] )
        {
            z = ++m_store[x] / 2 ;
        }
        else m_store[x] = z * 2;

        //we start bltting from the top and go down
        //so start with dark and then blt glow blocks
        //REMEMBER: z is a number from 0 to 6, y is 1 to 7 so that the method works
        for( uint y = 1; y <= ROWS; ++y )
        {
            if( y > z ) //greater than z means we blt bottom, ie glow blocks
                bitBlt( canvas(), (x+2) * (WIDTH + 1), y * (HEIGHT + 1) + offset, &m_glow[y-1] ); //x+1 = margin
            else
                bitBlt( canvas(), (x+2) * (WIDTH + 1), y * (HEIGHT + 1) + offset, &m_dark );
        }

    }
}
