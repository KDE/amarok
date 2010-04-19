/****************************************************************************************
 * Copyright (c) 2010 Daniel Dewald <Daniel.Dewald@time-shift.de>                       *
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

#include "SplineInterpolation.h"

double neville( QVector<double> x, QVector<double> y, double t )
{
    QVector<double> f;

     f.fill( 0.0, x.size() );
     
     for( int a = 0; a < x.size(); a++ )
     {
        f[a] = y[a];
     }

     for ( int z = 1; z < x.size(); z++ )
     {
        for ( int i = x.size() - 1; i >= z; i-- )
        {
            f[i] = ( ( t - x[i - z] ) * f[i] - ( t - x[i] ) * f[i - 1] ) / ( x[i] - x[i - z] );
        }
     }

     return f[x.size() - 1];
}


QVector<double> divDiff( QVector<double> x, QVector<double> y )
{
    QVector<double> f;

    f.fill( 0, x.size() );
     
    for ( int i = 0; i < x.size(); i++ )
    {
        f[i] = y[i];
    }

    for ( int j = 1; j < x.size(); j++ )
    {
        for ( int i = x.size() - 1; i >= j; i-- )
        {
           f[i] = ( f[i] - f[i - 1] ) / ( x[i] - x[i - j] );
        }
    }

    return f;
}

double nfEval( QVector<double> x, QVector<double> nf, double t )
{
    double temp;

    temp = nf[x.size() - 1];

    for ( int j = x.size() - 2; j >= 0; j-- )
    {
        temp = temp * ( t - x[j] ) + nf[j];
    }

    return temp;
}

void tridiagonal ( int n, QVector<double> c, QVector<double> &a, QVector<double> &b, QVector<double> &r )
{
    for ( int i = 0; i < n - 1; i++ )
    {
         b[i] /= a[i];
         a[i + 1] -= c[i] * b[i];
     }

     r[0] /= a[0];
     
     for ( int i = 1; i < n; i++ )
     {
        r[i] = ( r[i] - c[i - 1] * r[i - 1] ) / a[i];
     }

     for ( int i = n - 2; i >= 0; i-- )
     {
        r[i] -= r[i+1] * b[i];
     }
}

void cubicNak( QVector<double> x, QVector<double> f, QVector<double> &b, QVector<double> &c, QVector<double> &d )
{
    QVector<double> h, dl, dd, du;

    h.fill( 0.0, x.size() );
    dl.fill( 0.0, x.size() );
    dd.fill( 0.0, x.size() );
    du.fill( 0.0, x.size() );

    for( int i = 0; i < x.size() - 1; i++ )
    {
        h[i] = x[i + 1] - x[i];
    }

    for ( int i = 0; i < x.size() - 3; i++ )
    {
        dl[i] = du[i] = h[i+1];
    }

    for ( int i = 0; i < x.size() - 2; i++ )
    {
        dd[i] = 2.0 * ( h[i] + h[i + 1] );
        c[i]  = ( 3.0 / h[i + 1] ) * ( f[i + 2] - f[i + 1] ) - ( 3.0 / h[i] ) * ( f[i + 1] - f[i] );
    }
    
    dd[0] += ( h[0] + h[0] * h[0] / h[1] );
    dd[x.size() - 3] += ( h[x.size() - 2] + h[x.size() - 2] * h[x.size() - 2] / h[x.size() - 3] );
    du[0] -= ( h[0] * h[0] / h[1] );
    dl[x.size() - 4] -= ( h[x.size() - 2] * h[x.size() - 2] / h[x.size() - 3] );

    tridiagonal( x.size() - 2, dl, dd, du, c );

    for ( int i = x.size() - 3; i >= 0; i-- )
    {
       c[i + 1] = c[i];
    }
     
    c[0] = ( 1.0 + h[0] / h[1] ) * c[1] - h[0] / h[1] * c[2];
    c[x.size() - 1] = ( 1.0 + h[x.size() - 2] / h[x.size() - 3] ) * c[x.size() - 2] - h[x.size() - 2] / h[x.size() - 3] * c[x.size() - 3];

    for ( int i = 0; i < x.size() - 1; i++ )
    {
        d[i] = ( c[i + 1] - c[i] ) / ( 3.0 * h[i] );
        b[i] = ( f[i + 1] - f[i] ) / h[i] - h[i] * ( c[i + 1] + 2.0*c[i] ) / 3.0;
    }
}

void cubicClamped ( QVector<double> x, QVector<double> f, QVector<double> &b, QVector<double> &c, QVector<double> &d, double fpa, double fpb )
{
    QVector<double> h, dl, dd, du;

    h.fill( 0.0, x.size() );
    dl.fill( 0.0, x.size() );
    dd.fill( 0.0, x.size() );
    du.fill( 0.0, x.size() );

    for ( int i = 0; i < x.size() - 1; i++ )
    {
        h[i] = x[i + 1] - x[i];
        dl[i] = du[i] = h[i];
    }

    dd[0] = 2.0 * h[0];
    dd[x.size() - 1] = 2.0 * h[x.size() - 2];
    c[0] = ( 3.0 / h[0] ) * ( f[1] - f[0] ) - 3.0 * fpa;
    c[x.size() - 1] = 3.0 * fpb - ( 3.0 / h[x.size() - 2] ) * ( f[x.size() - 1] - f[x.size() - 2] );

    for ( int i = 0; i < x.size() - 2; i++ )
    {
        dd[i + 1] = 2.0 * ( h[i] + h[i + 1] );
        c[i + 1] = ( 3.0 / h[i + 1] ) * ( f[i + 2] - f[i + 1] ) - ( 3.0 / h[i] ) * ( f[i + 1] - f[i] );
    }

    tridiagonal( x.size(), dl, dd, du, c );

     for ( int i = 0; i < x.size() - 1; i++ )
     {
        d[i] = ( c[i + 1] - c[i] ) / ( 3.0 * h[i] );
        b[i] = ( f[i + 1] - f[i] ) / h[i] - h[i] * ( c[i + 1] + 2.0*c[i] ) / 3.0;
     }
}

double splineEval ( QVector<double> x, QVector<double> f, QVector<double> b, QVector<double> c, QVector<double> d, double t )
{
    int i = 1;
    bool found = false;

    while ( !found && ( i < x.size() - 1 ) )
    {
        if ( t < x[i] )
        {
            found = true;
        }
        else
        {
            i++;
        }
    }

    t = f[i - 1] + ( t - x[i - 1] ) * ( b[i - 1] + ( t - x[i - 1] ) * ( c[i - 1] + ( t - x[i - 1] ) * d[i - 1] ) );
    return t;
}


