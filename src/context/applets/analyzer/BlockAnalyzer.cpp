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


BlockAnalyzer* BlockAnalyzer::instance = 0;

static inline uint myMax( uint v1, uint v2 )
{
    return v1 > v2 ? v1 : v2;
}

BlockAnalyzer::BlockAnalyzer( QWidget *parent )
    : Analyzer::Base( parent )
    , m_columns( 0 )         //int
    , m_rows( 0 )            //int
    , m_fade_bars( FADE_SIZE ) //vector<QPixmap>
    , m_fade_pos( MAX_COLUMNS, 50 ) //vector<uint>
    , m_fade_intensity( MAX_COLUMNS, 32 ) //vector<uint>
{
    instance = this;
    setObjectName( "Blocky" );

    setMaximumWidth( MAX_COLUMNS * ( BLOCK_WIDTH + 1 ) - 1 );
    setFps( 50 );
}

void
BlockAnalyzer::initializeGL()
{
    // Disable depth test (all is drawn on a 2d plane)
    glDisable( GL_DEPTH_TEST );
}

void
BlockAnalyzer::resizeGL( int w, int h )
{
    glViewport( 0, 0, (GLint)w, (GLint)h );

    // Set up a 2D projection matrix
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( 0.0, (GLdouble)w, (GLdouble)h, 0.0, 0.0, 1.0 );

    const int oldRows = m_rows;

    // Rounded up so that the last column/line is covered if partially visible
    m_columns = std::min( std::ceil( (double)width() / ( BLOCK_WIDTH + 1 ) ), (double)MAX_COLUMNS );
    m_rows    = std::ceil( (double)height() / ( BLOCK_HEIGHT + 1 ) );

    m_scope.resize( m_columns );
    m_store.resize( m_columns );

    if( m_rows != oldRows )
    {
        m_barPixmap = QPixmap( BLOCK_WIDTH, m_rows * ( BLOCK_HEIGHT + 1 ) );

        m_yscale.resize( m_rows + 1 );

        const float PRE = 1, PRO = 1; //PRE and PRO allow us to restrict the range somewhat

        for( int z = 0; z < m_rows; ++z )
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
BlockAnalyzer::paintGL()
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

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    // Paint the background
    drawTexture( m_background.data(), 0, 0, 0, 0 );

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
            const uint y = m_fade_pos[x] * ( BLOCK_HEIGHT + 1 );
            if( y < (uint)height() )
                drawTexture( m_fade_bars[offset].data(), x * ( BLOCK_WIDTH + 1 ), y, 0, 0 );
        }

        if( m_fade_intensity[x] == 0 )
            m_fade_pos[x] = m_rows;

        // REMEMBER: y is a number from 0 to m_rows, 0 means all blocks are glowing, m_rows means none are
        drawTexture( m_barTexture, x * ( BLOCK_WIDTH + 1 ), y * ( BLOCK_HEIGHT + 1 ) + m_y, 0, y * ( BLOCK_HEIGHT + 1 ) );

        // Draw top bar
        drawTexture( m_topBarTexture, x * ( BLOCK_WIDTH + 1 ), int( m_store[x] ) * ( BLOCK_HEIGHT + 1 ) + m_y, 0, 0 );
    }
}

void
BlockAnalyzer::drawTexture( Texture* texture, int x, int y, int sx, int sy )
{
    const GLfloat xf = x;
    const GLfloat yf = y;
    const GLfloat wf = texture->size.width() - sx;
    const GLfloat hf = texture->size.height() - sy;
    const GLfloat sxf = (GLfloat)sx / texture->size.width();
    const GLfloat syf = (GLfloat)sy / texture->size.height();

    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, texture->id );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    // Draw a textured quad
    glBegin(GL_QUADS);
    glTexCoord2f( sxf, syf ); glVertex2f( xf, yf );
    glTexCoord2f( sxf, 1.0 ); glVertex2f( xf, yf + hf );
    glTexCoord2f( 1.0, 1.0 ); glVertex2f( xf + wf, yf + hf );
    glTexCoord2f( 1.0, syf ); glVertex2f( xf + wf, yf );
    glEnd();

    glDisable( GL_TEXTURE_2D );
}

void
BlockAnalyzer::paletteChange( const QPalette& ) //virtual
{
    QPainter p( &m_barPixmap );

    const QColor bg = The::paletteHandler()->backgroundColor();
    const QColor fg = palette().color( QPalette::Active, QPalette::Highlight );

    QPixmap topBar( BLOCK_WIDTH, BLOCK_HEIGHT );
    topBar.fill( fg );
    m_topBarTexture = QSharedPointer<Texture>( new Texture( topBar ) );

    const double dr = 15 * double( bg.red()   - fg.red() )   / ( m_rows * 16 );
    const double dg = 15 * double( bg.green() - fg.green() ) / ( m_rows * 16 );
    const double db = 15 * double( bg.blue()  - fg.blue() )  / ( m_rows * 16 );
    const int r = fg.red(), g = fg.green(), b = fg.blue();

    m_barPixmap.fill( bg );

    for( int y = 0; y < m_rows; ++y )
        //graduate the fg color
        p.fillRect( 0, y * ( BLOCK_HEIGHT + 1 ), BLOCK_WIDTH, BLOCK_HEIGHT, QColor( r + int( dr * y ), g + int( dg * y ), b + int( db * y ) ) );

    {
        const QColor bg = palette().color( QPalette::Active, QPalette::Window ).dark( 112 );

        //make a complimentary fadebar colour
        //TODO dark is not always correct, dumbo!
        int h, s, v; palette().color( QPalette::Active, QPalette::Window ).dark( 150 ).getHsv( &h, &s, &v );
        const QColor fg = QColor::fromHsv( h + 60, s, v );

        const double dr = fg.red() - bg.red();
        const double dg = fg.green() - bg.green();
        const double db = fg.blue() - bg.blue();
        const int r = bg.red(), g = bg.green(), b = bg.blue();

        // Precalculate all fade-bar pixmaps
        for( int y = 0; y < FADE_SIZE; ++y )
        {
            QPixmap fadeBar( BLOCK_WIDTH, m_rows * ( BLOCK_HEIGHT + 1 ) );

            fadeBar.fill( palette().color( QPalette::Active, QPalette::Window ) );
            const double Y = 1.0 - ( log10( ( FADE_SIZE ) - y ) / log10( ( FADE_SIZE ) ) );
            QPainter f( &fadeBar );
            for( int z = 0; z < m_rows; ++z )
                f.fillRect( 0, z * ( BLOCK_HEIGHT + 1 ), BLOCK_WIDTH, BLOCK_HEIGHT, QColor( r + int( dr * Y ), g + int( dg * Y ), b + int( db * Y ) ) );

            m_fade_bars[y] = QSharedPointer<Texture>( new Texture( fadeBar ) );
        }
    }

    m_barTexture = QSharedPointer<Texture>( new Texture( m_barPixmap ) );
    drawBackground();
}

void
BlockAnalyzer::drawBackground()
{
    const QColor bg = palette().color( QPalette::Active, QPalette::Window );
    const QColor bgdark = bg.dark( 112 );

    QPixmap background( size() );
    background.fill( bg );

    QPainter p( &background );
    for( int x = 0; x < m_columns; ++x )
        for( int y = 0; y < m_rows; ++y )
            p.fillRect( x * ( BLOCK_WIDTH + 1 ), y * ( BLOCK_HEIGHT + 1 ), BLOCK_WIDTH, BLOCK_HEIGHT, bgdark );

    m_background = QSharedPointer<Texture>( new Texture( background ) );
}
