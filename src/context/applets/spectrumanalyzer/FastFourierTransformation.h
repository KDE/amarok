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

//This class was mainly taken from Melchior FRANZ's (mfranz@kde.org) simple player example (see fht.h and fht.cpp)

#ifndef FASTFOURIERTRANSFORMATION_H
#define FASTFOURIERTRANSFORMATION_H

class FastFourierTransformation
{
    public:
        /**
        * Prepare transform for data sets with @f$2^n@f$ numbers, whereby @f$n@f$
        * should be at least 3. Values of more than 3 need a trigonometry table.
        * @see makeCasTable()
        */
        FastFourierTransformation( int n );
        ~FastFourierTransformation();

        /**
        *
        */
        void scale( float *p, float d );

        /**
        * Fourier spectrum.
        */
        void spectrum( float *p );

    private:
        int     m_num;
        float   *m_buf;
        float   *m_tab;

        /**
        * Create a table of cosine and sine values.
        * Has only to be done in the constructor and saves from
        * calculating the same values over and over while transforming.
        */
        void makeCasTable();

        /**
        * Recursive in-place Hartley transform. For internal use only!
        */
        void _transform(float *, int, int);

        /**
        * Calculates an FFT power spectrum with doubled values as a
        * result. The values need to be multiplied by 0.5 to be exact.
        * Note that you only get @f$2^{n-1}@f$ power values for a data set
        * of @f$2^n@f$ input values. This is the fastest transform.
        */
        void power2( float * );

        /**
        * Discrete Hartley transform of data sets with 8 values.
        */
        void transform8( float * );
};

#endif // FASTFOURIERTRANSFORMATION_H
