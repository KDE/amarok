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

#include <QVector>

#ifndef SPLINEINTERPOLATION_H
#define SPLINEINTERPOLATION_H

    /**
    *   Evaluates the polynomial which interpolates a given set of data at a single value of the independent variable
    *   @returns the value of interpolating polynomial
    *   @arg x vector containing interpolating points
    *   @arg y vector containing function values of x
    *   @arg t independent value at which the interpolating polynomial is to be evaluated
    */
    double neville( QVector<double> x, QVector<double> y, double t );

    /**
    *   Computes the coefficients of the Newton form of the polynomial which interpolates a given set of data
    *   @returns a vector containing the coefficients of the Newton form of the interpolating polynomial
    *   @arg x vector containing interpolating points
    *   @arg y vector containing function values of x
    */
    QVector<double> divDiff( QVector<double> x, QVector<double> y );

    /**
    *   Evaluates the Newton form of the polynomial which interpolates a given set of data at a single value of the independent variable
    *   @returns a value of interpolating polynomial at the specified value of the independent variable
    *   @arg x vector containing interpolating points
    *   @arg nf vector containing the coefficients of the Newton form of the interpolating polynomial
    *   @arg t independent value at which the interpolating polynomial is to be evaluated
    */
    double nfEval( QVector<double> x, QVector<double> nf, double t );

    /**
    *   Determinates the coefficients for the 'not-a-knot' cubic spline for a given set of data
    *   @returns nothing
    *   @arg x vector containing interpolating points
    *   @arg f vector containing function values to be interpolated
    *   @arg b output for coefficients of linear terms in cubic spline
    *   @arg c output for coefficients of quadratic terms in cubic spline
    *   @arg d output for coefficients of cubic terms in cubic spline
    */
    void cubicNak( QVector<double> x, QVector<double> f, QVector<double> &b, QVector<double> &c, QVector<double> &d );

    /**
    *   Determinates the coefficients for the clamped cubic spline for a given set of data
    *   @returns nothing
    *   @arg x vector containing interpolating points
    *   @arg f vector containing function values to be interpolated
    *   @arg b output for coefficients of linear terms in cubic spline
    *   @arg c output for coefficients of quadratic terms in cubic spline
    *   @arg d output for coefficients of cubic terms in cubic spline
    *   @arg fpa derivative of function at x=a
    *   @arg fpb derivative of function at x=b
    */
    void cubicClamped( QVector<double> x, QVector<double> f, QVector<double> &b, QVector<double> &c, QVector<double> d, double fpa, double fpb );

    /**
    *   Evaluates a cubic spline at a single value of the independent variable given the coefficients of the cubic spline interpolant
    *   @returns value of cubic spline at the specified value of the independent variable
    *   @arg x vector containing interpolating points
    *   @arg f vector containing the constant terms from the cubic spline
    *   @arg b vector containing the coefficients of the linear terms from the cubic spline
    *   @arg c vector containing the coefficients of the quadratic terms from the cubic spline
    *   @arg d vector containing the coefficients of the cubic terms from the cubic spline
    *   @arg t value of independent variable at which the interpolating polynomial is to be evaluated
    */
    double splineEval( QVector<double> x, QVector<double> f, QVector<double> b, QVector<double> c, QVector<double> d, double t );
    
#endif
