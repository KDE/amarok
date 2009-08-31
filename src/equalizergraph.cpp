/****************************************************************************************
 * Copyright (c) 1998-2000 Peter Alm <info@opensound.com>                               *
 * Copyright (c) 1998-2000 Mikael Alm <info@opensound.com>                              *
 * Copyright (c) 1998-2000 Olle Hallnas <info@opensound.com>                            *
 * Copyright (c) 1998-2000 Thomas Nilsson <info@opensound.com>                          *
 * Copyright (c) 1998-2000 4Front Technologies <info@opensound.com>                     *
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2005 Markus Brueffer <markus@brueffer.de>                              *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

/***************************************************************************
  Graphical spline display for equalizer
  Based on code from XMMS
***************************************************************************/

#include "equalizergraph.h"

#include "amarokconfig.h"

#include <QPainter>
#include <QPixmap>
#include <QVector>


EqualizerGraph::EqualizerGraph( QWidget* parent )
    : QWidget( parent, Qt::WNoAutoErase )
    , m_backgroundPixmap( new QPixmap() )
{}


EqualizerGraph::~EqualizerGraph()
{
    delete m_backgroundPixmap;
}


/////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
/////////////////////////////////////////////////////////////////////////////////////

void
EqualizerGraph::resizeEvent( QResizeEvent* )
{
    drawBackground();
}

QSize
EqualizerGraph::sizeHint() const
{
   return QSize( 100, 60 );
}

void
EqualizerGraph::paintEvent( QPaintEvent* )
{
    QPainter p( this );
    p.drawPixmap( 0, 0, *m_backgroundPixmap );

    // Draw middle line
    int middleLineY = (int) ( ( height() - 1 ) / 2.0 + AmarokConfig::equalizerPreamp() * ( height() - 1 ) / 200.0 );
    QPen pen( colorGroup().dark(), 0, Qt::DotLine);
    p.setPen( pen );
    p.drawLine( 8, middleLineY, width() - 1, middleLineY );

    QColor color( colorGroup().highlight() );
    int h, s, v;
    color.getHsv( &h, &s, &v );

    int i, y, ymin, ymax, py = 0;
    float x[NUM_BANDS], yf[NUM_BANDS];
    float gains[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0};

    // Don't calculate 0 and NUM_BANDS-1 for accuracy reasons
    for ( i = 1; i < NUM_BANDS -1 ; i++)
        x[i] = ( width() - 8 ) * i / ( NUM_BANDS -1 ) + 8;
    x[ 0 ] = 8;
    x[ NUM_BANDS - 1 ] = width() - 1;

    if ( AmarokConfig::equalizerEnabled() )
        for ( i = 0; i < NUM_BANDS; i++ )
            gains[i] = ( height() - 1 ) * AmarokConfig::equalizerGains()[i] / 200.0;

    init_spline( x, gains, NUM_BANDS, yf );

    for ( i = 8; i < width(); ++i ) {
        y = (int) ( ( height() - 1 ) / 2 - eval_spline( x, gains, yf, NUM_BANDS, i ) );

        if ( y < 0 )
            y = 0;

        if ( y > height() - 1 )
            y = height() - 1;

        if ( i == 8 )
            py = y;

        if ( y < py ) {
            ymin = y;
            ymax = py;
        } else {
            ymin = py;
            ymax = y;
        }

        py = y;
        for ( y = ymin; y <= ymax; ++y ) {
            // Absolute carthesian coordinate
            s = y - ( height() - 1 ) / 2;
            s = QABS(s);

            // Normalise to a base of 256
            // short for: s / ( ( height() / 2.0 ) * 255;
            s = (int) ( s * 510.0 / height() );

            color.setHsv( h, 255 - s, v );
            p.setPen( color );

            p.drawPoint( i, y );
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE
/////////////////////////////////////////////////////////////////////////////////////

void
EqualizerGraph::drawBackground()
{
    m_backgroundPixmap->resize( size() );

    m_backgroundPixmap->fill( colorGroup().background().dark( 105 ) );
    QPainter p( m_backgroundPixmap );

    // Erase background for scale
    p.fillRect( 0, 0, 7, height() -1, colorGroup().background());

    // Draw scale
    p.setPen( colorGroup().shadow() );
    p.drawLine( 7, 0, 7, height() - 1 );
    p.drawLine( 0, 0, 7, 0 );
    p.drawLine( 0, height() / 2 - 1, 7, height() / 2 - 1 );
    p.drawLine( 0, height() - 1, 7, height() - 1 );
}


void
EqualizerGraph::init_spline( float* x, float* y, int n, float* y2 )
{
    int i, k;
    float p, qn, sig, un;
    QVector<float> u(n * sizeof(float));

    y2[ 0 ] = u[ 0 ] = 0.0;

    for ( i = 1; i < n - 1; ++i ) {
        sig = ( (float)x[i] - x[i-1] ) / ( (float)x[i+1] - x[i-1] );
        p = sig * y2[i-1] + 2.0;
        y2[i] = ( sig - 1.0 ) / p;
        u[i] = ( ( (float)y[i+1] - y[i] ) / ( x[i+1] - x[i] ) ) - ( ( (float)y[i] - y[i-1] ) / ( x[i] - x[i-1] ) );
        u[i] = ( 6.0 * u[i] / ( x[i+1] - x[i-1] ) - sig * u[i-1] ) / p;
    }
    qn = un = 0.0;

    y2[n-1] = ( un - qn * u[n-2] ) / ( qn * y2[n-2] + 1.0 );
    for ( k = n - 2; k >= 0; --k )
        y2[k] = y2[k] * y2[k+1] + u[k];
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
        if ( xa[k] > x )
            khi = k;
        else
            klo = k;
    }
    h = xa[khi] - xa[klo];
    a = ( xa[khi] - x ) / h;
    b = ( x - xa[klo] ) / h;
    return ( a * ya[klo] + b * ya[khi] + ( ( a*a*a - a ) * y2a[klo] + ( b*b*b - b ) * y2a[khi] ) * ( h*h ) / 6.0 );
}

