/***************************************************************************
 Graphical spline display for equalizer

 (c) 2004 Mark Kretschmann <markey@web.de>
 Based on code from XMMS
 (c) 1998-2000 Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarokconfig.h"
#include "equalizergraph.h"

#include <qpainter.h>
#include <qpixmap.h>
#include <qvaluelist.h>

#include <kapplication.h>


EqualizerGraph::EqualizerGraph( QWidget* parent )
    : QWidget( parent, 0, Qt::WNoAutoErase )
    , m_backgroundPixmap( new QPixmap() )
    , m_composePixmap( new QPixmap() )
{
    drawBackground();
}


EqualizerGraph::~EqualizerGraph()
{
    delete m_backgroundPixmap;
    delete m_composePixmap;
}


/////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
/////////////////////////////////////////////////////////////////////////////////////

void
EqualizerGraph::resizeEvent( QResizeEvent* )
{
    drawBackground();
}


void
EqualizerGraph::paintEvent( QPaintEvent* )
{
    bitBlt( m_composePixmap, 0, 0, m_backgroundPixmap );

    QPainter p( m_composePixmap );
    p.setPen( Qt::black );

//     int cols[ 19 ];
    int i, y, ymin, ymax, py = 0;
    float x[] = {0, 11, 23, 35, 47, 59, 71, 83, 97, 109}, yf[ 10 ];

    float gains[NUM_BANDS];

    for ( int count = 0; count < NUM_BANDS; count++ )
        gains[count] = (float) *AmarokConfig::equalizerGains().at( count ) * 0.3;

    init_spline( x, gains, NUM_BANDS, yf );

    for ( i = 0; i < 109; i++ ) {
        y = 9 - ( int ) ( ( eval_spline( x, gains, yf, NUM_BANDS, i ) * 9.0 ) / 20.0 );
        if ( y < 0 )
            y = 0;
        if ( y > 18 )
            y = 18;
        if ( !i )
            py = y;
        if ( y < py ) {
            ymin = y;
            ymax = py;
        } else {
            ymin = py;
            ymax = y;
        }

        py = y;
        for ( y = ymin; y <= ymax; y++ ) {
            p.drawPoint( i, y );
        }
    }

    p.end();
    bitBlt( this, 0, 0, m_composePixmap );
}


/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE
/////////////////////////////////////////////////////////////////////////////////////

void
EqualizerGraph::drawBackground()
{
    m_backgroundPixmap->resize( size() );
    m_composePixmap->resize( size() );

    m_backgroundPixmap->fill( kapp->palette().active().background() );
    QPainter p( m_backgroundPixmap );

    // Draw frame
    p.setPen( kapp->palette().active().highlight() );
    p.drawRect( 0, 0, width() - 1, height() - 1 );

    // Draw middle line
    QPen pen( kapp->palette().active().shadow(), 0, Qt::DotLine);
    p.setPen( pen );
    p.drawLine( 0, height() / 2 - 1, width() - 1, height() / 2 - 1 );
}


void
EqualizerGraph::init_spline( float* x, float* y, int n, float* y2 )
{
    int i, k;
    float p, qn, sig, un;
    float u[n * sizeof(float)];

    y2[ 0 ] = u[ 0 ] = 0.0;

    for ( i = 1; i < n - 1; i++ ) {
        sig = ( ( float ) x[ i ] - x[ i - 1 ] ) / ( ( float ) x[ i + 1 ] - x[ i - 1 ] );
        p = sig * y2[ i - 1 ] + 2.0;
        y2[ i ] = ( sig - 1.0 ) / p;
        u[ i ] = ( ( ( float ) y[ i + 1 ] - y[ i ] ) / ( x[ i + 1 ] - x[ i ] ) ) - ( ( ( float ) y[ i ] - y[ i - 1 ] ) / ( x[ i ] - x[ i - 1 ] ) );
        u[ i ] = ( 6.0 * u[ i ] / ( x[ i + 1 ] - x[ i - 1 ] ) - sig * u[ i - 1 ] ) / p;
    }
    qn = un = 0.0;

    y2[ n - 1 ] = ( un - qn * u[ n - 2 ] ) / ( qn * y2[ n - 2 ] + 1.0 );
    for ( k = n - 2; k >= 0; k-- )
        y2[ k ] = y2[ k ] * y2[ k + 1 ] + u[ k ];
}


float
EqualizerGraph::eval_spline( float xa[], float ya[], float y2a[], int n, float x )
{
    int klo, khi, k;
    float h, b, a;

    klo = 0;
    khi = n - 1;
    while ( khi - klo > 1 ) {
        k = ( khi + klo ) >> 1;
        if ( xa[ k ] > x )
            khi = k;
        else
            klo = k;
    }
    h = xa[ khi ] - xa[ klo ];
    a = ( xa[ khi ] - x ) / h;
    b = ( x - xa[ klo ] ) / h;
    return ( a * ya[ klo ] + b * ya[ khi ] + ( ( a * a * a - a ) * y2a[ klo ] + ( b * b * b - b ) * y2a[ khi ] ) * ( h * h ) / 6.0 );
}

