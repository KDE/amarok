/****************************************************************************************
 * Copyright (c) 2003-2005 Max Howell <max.howell@methylblue.com>                       *
 * Copyright (c) 2005-2013 Mark Kretschmann <kretschmann@kde.org>                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "BlockAnalyzer.h"

#include "PaletteHandler.h"

#include <cmath>

#include <QPainter>
#include <QResizeEvent>


static inline uint myMax( uint v1, uint v2 )
{
    return v1 > v2 ? v1 : v2;
}

BlockAnalyzer::BlockAnalyzer( QWidget *parent )
    : Analyzer::Base2D( parent )
    , m_columns( 0 )         //uint
    , m_rows( 0 )            //uint
    , m_y( 0 )               //uint
    , m_barPixmap( 1, 1 )    //null qpixmaps cause crashes
    , m_topBarPixmap( WIDTH, HEIGHT )
    , m_scope( MIN_COLUMNS ) //Scope
    , m_store( 1 << 8, 0 )   //vector<uint>
    , m_fade_bars( FADE_SIZE ) //vector<QPixmap>
    , m_fade_pos( 1 << 8, 50 ) //vector<uint>
    , m_fade_intensity( 1 << 8, 32 ) //vector<uint>
{
    setObjectName( "Blocky" );
    setMaximumWidth( MAX_COLUMNS * ( WIDTH + 1 ) - 1 );
}

BlockAnalyzer::~BlockAnalyzer()
{}

void
BlockAnalyzer::resizeEvent( QResizeEvent *e )
{
    Analyzer::Base2D::resizeEvent( e );

    m_background = QPixmap( size() );

    const uint oldRows = m_rows;

    //all is explained in analyze()..
    //+1 to counter -1 in maxSizes, trust me we need this!
    m_columns = qMin<uint>( uint( double( width() + 1 ) / ( WIDTH + 1 ) ), MAX_COLUMNS );
    m_rows    = uint( double( height() + 1 ) / ( HEIGHT + 1 ) );

    //this is the y-offset for drawing from the top of the widget
    m_y = ( height() - ( m_rows * ( HEIGHT + 1 ) ) + 2 ) / 2;

    m_scope.resize( m_columns );

    if( m_rows != oldRows )
    {
        m_barPixmap = QPixmap( WIDTH, m_rows * ( HEIGHT + 1 ) );

        for( int i = 0; i < FADE_SIZE; ++i )
            m_fade_bars[i] = QPixmap( WIDTH, m_rows * ( HEIGHT + 1 ) );

        m_yscale.resize( m_rows + 1 );

        const float PRE = 1, PRO = 1; //PRE and PRO allow us to restrict the range somewhat

        for( uint z = 0; z < m_rows; ++z )
            m_yscale[z] = 1 - ( log10( PRE + z ) / log10( PRE + m_rows + PRO ) );

        m_yscale[m_rows] = 0;

        determineStep();
        paletteChange( palette() );
    }

    drawBackground();
    analyze( m_scope );
}

void
BlockAnalyzer::determineStep()
{
    // falltime is dependent on rowcount due to our digital resolution (ie we have boxes/blocks of pixels)
    // I calculated the value 30 based on some trial and error

    const double fallTime = 30 * m_rows;
    m_step = double( m_rows * 80 ) / fallTime; //80 = ~milliseconds between signals with audio data
}

void
BlockAnalyzer::transform( QVector<float> &s ) //pure virtual
{
    for( int x = 0; x < s.size(); ++x )
        s[x] *= 2;

    float *front = static_cast<float*>( &s.front() );

    m_fht->spectrum( front );
    m_fht->scale( front, 1.0 / 20 );

    //the second half is pretty dull, so only show it if the user has a large analyzer
    //by setting to m_scope.size() if large we prevent interpolation of large analyzers, this is good!
    s.resize( m_scope.size() <= MAX_COLUMNS / 2 ? MAX_COLUMNS / 2 : m_scope.size() );
}

void
BlockAnalyzer::analyze( const QVector<float> &s )
{
    interpolate( s, m_scope );
}

void
BlockAnalyzer::paintEvent( QPaintEvent* )
{
    // y = 2 3 2 1 0 2
    //     . . . . # .
    //     . . . # # .
    //     # . # # # #
    //     # # # # # #
    //
    // visual aid for how this analyzer works.
    // y represents the number of blanks
    // y starts from the top and increases in units of blocks

    // m_yscale looks similar to: { 0.7, 0.5, 0.25, 0.15, 0.1, 0 }
    // if it contains 6 elements there are 5 rows in the analyzer

    QPainter p( this );

    // Paint the background
    p.drawPixmap( 0, 0, m_background );

    for( uint y, x = 0; x < (uint)m_scope.size(); ++x )
    {
        // determine y
        for( y = 0; m_scope[x] < m_yscale[y]; ++y )
            ;

        // this is opposite to what you'd think, higher than y
        // means the bar is lower than y (physically)
        if( ( float )y > m_store[x] )
            y = uint( m_store[x] += m_step );
        else
            m_store[x] = y;

        // if y is lower than m_fade_pos, then the bar has exceeded the height of the fadeout
        // if the fadeout is quite faded now, then display the new one
        if( y <= m_fade_pos[x] /*|| m_fade_intensity[x] < FADE_SIZE / 3*/ )
        {
            m_fade_pos[x] = y;
            m_fade_intensity[x] = FADE_SIZE;
        }

        if( m_fade_intensity[x] > 0 )
        {
            const uint offset = --m_fade_intensity[x];
            const uint y = m_y + ( m_fade_pos[x] * ( HEIGHT + 1 ) );
            if( y < (uint)height() )
                p.drawPixmap( x * ( WIDTH + 1 ), y, m_fade_bars[offset], 0, 0, WIDTH, height() - y );
        }

        if( m_fade_intensity[x] == 0 )
            m_fade_pos[x] = m_rows;

        // REMEMBER: y is a number from 0 to m_rows, 0 means all blocks are glowing, m_rows means none are
        p.drawPixmap( x * ( WIDTH + 1 ), y * ( HEIGHT + 1 ) + m_y, *bar(), 0, y * ( HEIGHT + 1 ), -1, -1 );
    }

    for( uint x = 0; x < m_store.size(); ++x )
        p.drawPixmap( x * ( WIDTH + 1 ), int( m_store[x] ) * ( HEIGHT + 1 ) + m_y, m_topBarPixmap );
}

static inline void
adjustToLimits( int &b, int &f, uint &amount )
{
    // with a range of 0-255 and maximum adjustment of amount,
    // maximise the difference between f and b

    if( b < f )
    {
        if( b > 255 - f )
        {
            amount -= f;
            f = 0;
        }
        else
        {
            amount -= ( 255 - f );
            f = 255;
        }
    }
    else
    {
        if( f > 255 - b )
        {
            amount -= f;
            f = 0;
        }
        else
        {
            amount -= ( 255 - f );
            f = 255;
        }
    }
}

void
BlockAnalyzer::paletteChange( const QPalette& ) //virtual
{
    QPainter p( bar() );

    const QColor bg = The::paletteHandler()->backgroundColor();
    const QColor fg = The::paletteHandler()->palette().color( QPalette::Highlight );

    m_topBarPixmap.fill( fg );

    const double dr = 15 * double( bg.red()   - fg.red() )   / ( m_rows * 16 );
    const double dg = 15 * double( bg.green() - fg.green() ) / ( m_rows * 16 );
    const double db = 15 * double( bg.blue()  - fg.blue() )  / ( m_rows * 16 );
    const int r = fg.red(), g = fg.green(), b = fg.blue();

    bar()->fill( bg );

    for( int y = 0; ( uint )y < m_rows; ++y )
        //graduate the fg color
        p.fillRect( 0, y * ( HEIGHT + 1 ), WIDTH, HEIGHT, QColor( r + int( dr * y ), g + int( dg * y ), b + int( db * y ) ) );

    {
        const QColor bg = palette().color( QPalette::Active, QPalette::Background ).dark( 112 );

        //make a complimentary fadebar colour
        //TODO dark is not always correct, dumbo!
        int h, s, v; palette().color( QPalette::Active, QPalette::Background ).dark( 150 ).getHsv( &h, &s, &v );
        const QColor fg = QColor::fromHsv( h + 60, s, v );

        const double dr = fg.red() - bg.red();
        const double dg = fg.green() - bg.green();
        const double db = fg.blue() - bg.blue();
        const int r = bg.red(), g = bg.green(), b = bg.blue();

        // Precalculate all fade-bar pixmaps
        for( int y = 0; y < FADE_SIZE; ++y )
        {
            m_fade_bars[y].fill( palette().color( QPalette::Active, QPalette::Background ) );
            const double Y = 1.0 - ( log10( ( FADE_SIZE ) - y ) / log10( ( FADE_SIZE ) ) );
            QPainter f( &m_fade_bars[y] );
            for( int z = 0; ( uint )z < m_rows; ++z )
                f.fillRect( 0, z * ( HEIGHT + 1 ), WIDTH, HEIGHT, QColor( r + int( dr * Y ), g + int( dg * Y ), b + int( db * Y ) ) );
        }
    }

    drawBackground();
}

void
BlockAnalyzer::drawBackground()
{
    const QColor bg = palette().color( QPalette::Active, QPalette::Background );
    const QColor bgdark = bg.dark( 112 );

    m_background.fill( bg );

    QPainter p( &m_background );
    for( int x = 0; ( uint )x < m_columns; ++x )
        for( int y = 0; ( uint )y < m_rows; ++y )
            p.fillRect( x * ( WIDTH + 1 ), y * ( HEIGHT + 1 ) + m_y, WIDTH, HEIGHT, bgdark );

}
