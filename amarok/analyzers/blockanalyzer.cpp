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

#define HEIGHT 2
#define WIDTH  4
#define ROWS   7

#include "blockanalyzer.h"

//static const float lvlMapper[ROWS] = { 0.155, 0.456, 0.632, 0.757, 0.854, 0.933, 10.0 };
static const float lvlMapper[ROWS] = { 0.100, 0.200, 0.300, 0.450, 0.625, 0.800, 10.0 };

BlockAnalyzer::BlockAnalyzer( QWidget *parent )
 : Analyzer::Base2D( parent, 16 )
 , m_block1( WIDTH, HEIGHT )
 , m_block2( WIDTH, HEIGHT )
{
    m_block1.fill( QColor( 172, 149, 156 ) );
    m_block2.fill( QColor( 82, 68, 106 ) );
}

void
BlockAnalyzer::analyze( const Scope &s )
{
    std::vector<float> v( BAND_COUNT, 0 );
    const int offset = height() - (HEIGHT+1) * ROWS;

    Analyzer::interpolate( s, v );

//    QPixmap m_pix( v.size() * (WIDTH + 1), (HEIGHT + 1) * ROWS );
    eraseCanvas();

    for ( uint x = 0; x < v.size(); ++x )
    {
        uint z;
        for( z = 0; v[x] > lvlMapper[z]; ++z );
        //uint z = uint( ROWS - v[x] * ROWS );
        z = ROWS - 1 - z;
        for( uint y = 0; y < ROWS; ++y )
        {
            if( y > z )
                bitBlt( canvas(), x * (WIDTH + 1), y * (HEIGHT + 1) + offset, &m_block1 );
            else
                bitBlt( canvas(), x * (WIDTH + 1), y * (HEIGHT + 1) + offset, &m_block2 );
        }

    }
}
