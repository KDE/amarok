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
#undef  BAND_COUNT
#define BAND_COUNT 33

#include "blockanalyzer.h"

//static float lvlMapper[ROWS] = { 0.155, 0.456, 0.632, 0.757, 0.854, 0.933, 10.0 };
static float lvlMapper[ROWS] = { 0.100, 0.200, 0.300, 0.450, 0.625, 0.800, 10.0 };

BlockAnalyzer::BlockAnalyzer( QWidget *parent, const char *name )
 : AnalyzerBase2d( 16, parent, name  )
 , m_block1( WIDTH, HEIGHT )
 , m_block2( WIDTH, HEIGHT )
{}


BlockAnalyzer::~BlockAnalyzer()
{}


void BlockAnalyzer::init()
{
    m_block1.fill( QColor( 172, 149, 156 ) );
    m_block2.fill( QColor( 82, 68, 106 ) );
}


void BlockAnalyzer::drawAnalyzer( std::vector<float> *s )
{
    std::vector<float> v( BAND_COUNT, 0 );

    if ( s ) interpolate( s, v );

    QPixmap m_pix( v.size() * (WIDTH + 1), (HEIGHT + 1) * ROWS );
    m_pix.fill( QColor( 31, 32, 82 ) ); //FIXME get from settings struct.

    for ( uint x = 0; x < v.size(); ++x )
    {
        uint z;
        for( z = 0; v[x] > lvlMapper[z]; ++z );
        //uint z = uint( ROWS - v[x] * ROWS );
        z = ROWS - 1 - z;
        for( uint y = 0; y < ROWS; ++y )
        {
            if( y > z )
                bitBlt( &m_pix, x * (WIDTH + 1), y * (HEIGHT + 1), &m_block1 );
            else
                bitBlt( &m_pix, x * (WIDTH + 1), y * (HEIGHT + 1), &m_block2 );
        }
    
    }
    
    bitBlt( this, 0, height() - m_pix.height(), &m_pix ); 
}

#include "blockanalyzer.moc"
