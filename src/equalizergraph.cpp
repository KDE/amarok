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
#include <qvaluelist.h>


EqualizerGraph::EqualizerGraph( QWidget * parent )
    : QWidget( parent )
{}


/////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
/////////////////////////////////////////////////////////////////////////////////////

void
EqualizerGraph::paintEvent( QPaintEvent* e )
{
    QPainter p( this );
    p.setPen( Qt::black );

    int cols[ 19 ];
    int i, y, ymin, ymax, py = 0;
    float x[] = {0, 11, 23, 35, 47, 59, 71, 83, 97, 109}, yf[ 10 ];

    float gains[NUM_BANDS];

    for ( int count = 0; count < NUM_BANDS; count++ )
        gains[count] = (float) *AmarokConfig::equalizerGains().at( count ) * 0.01;

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
}


/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE
/////////////////////////////////////////////////////////////////////////////////////

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

