// FHT - Fast Hartley Transform Class
//
// Copyright (C) 2004  Melchior FRANZ - mfranz@aon.at
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$

#include <math.h>
#include <string.h>
#include "fht.h"


fht::fht(int e)
{
	if (e < 4) {
		buf = tab = 0;
		num = 0;
		exp2 = -1;
		return;
	}
	exp2 = e;
	num = 1 << e;
	buf = new double[num];
	tab = new double[num * 2];
	makecastable();
}


fht::~fht()
{
	delete[] buf;
	delete[] tab;
}


void fht::makecastable(void)
{	
	double d, *costab, *sintab;
	int ul, ndiv2 = num / 2;
	
	for (costab = tab, sintab = tab + num / 2 + 1, ul = 0; ul < num; ul++) {
		d = M_PI * ul / ndiv2;
		*costab = *sintab = cos(d);
		
		costab += 2, sintab += 2;
		if (sintab > tab + num * 2)
			sintab = tab + 1;
	}
}


double* fht::copy(double *d, double *s)
{
	return (double *)memcpy(d, s, num * sizeof(double));
}


double* fht::clear(double *d)
{
	return (double *)memset(d, 0, num * sizeof(double));
}


inline void fht::scale(double *p, double d)
{
	for (int i = 0; i < num; i++)
		*p++ *= d;
}


void fht::transform(double *p)
{
	if (num == 8)
		transform8(p);
	else
		_transform(p);		// doesn't need scaling
	return;
	
}


inline void fht::transform8(double *p)
{
	double a, b, c, d, e, f, g, h, b_f2, d_h2;
	double a_c_eg, a_ce_g, ac_e_g, aceg, b_df_h, bdfh;

	a = *p++, b = *p++, c = *p++, d = *p++;
	e = *p++, f = *p++, g = *p++, h = *p;
	b_f2 = (b - f) * M_SQRT2;
	d_h2 = (d - h) * M_SQRT2;

	a_c_eg = a - c - e + g;
	a_ce_g = a - c + e - g;
	ac_e_g = a + c - e - g;
	aceg = a + c + e + g;

	b_df_h = b - d + f - h;
	bdfh = b + d + f + h;

	*p = a_c_eg - d_h2;
	*--p = a_ce_g - b_df_h;
	*--p = ac_e_g - b_f2;
	*--p = aceg - bdfh;
	*--p = a_c_eg + d_h2;
	*--p = a_ce_g + b_df_h;
	*--p = ac_e_g + b_f2;
	*--p = aceg + bdfh;
}


inline double fht::_transform(double *p)
{
	__transform(p, num, 0);
	return 0.0;
}


void fht::power(double *p)
{
	_power(p);
	
	for (int i = 0; i < (num / 2); i++)
		*p++ *= .5;
}


double fht::_power(double *p)
{
	int i;
	double *q;
	_transform(p);
	
	*p = (*p * *p), *p += *p, p++;
	
	for (i = 1, q = p + num - 2; i < (num / 2); i++, --q)
		*p++ = (*p * *p) + (*q * *q);

	return .5;
}


void fht::__transform(double *p, int n, int k)
{
	if (n == 8) {
		transform8(p + k);
		return;
	}

	int i, j, ndiv2 = n / 2;
	double a, *t1, *t2, *t3, *t4, *ptab, *pp;
	
	for (i = 0, t1 = buf, t2 = buf + ndiv2, pp = &p[k]; i < ndiv2; i++)
		*t1++ = *pp++, *t2++ = *pp++;
	
	memcpy(p + k, buf, sizeof(double) * n);
	
	__transform(p, ndiv2, k);
	__transform(p, ndiv2, k + ndiv2);
	
	j = num / ndiv2 - 1;
	t1 = buf;
	t2 = t1 + ndiv2;
	t3 = p + k + ndiv2;
	ptab = tab;
	pp = p + k;
	
	a = *ptab++ * *t3++;
	a += *ptab * *pp;
	ptab += j;
	
	*t1++ = *pp + a;
	*t2++ = *pp++ - a;
	
	for (i = 1, t4 = p + k + n; i < ndiv2; i++, ptab += j) {
		a = *ptab++ * *t3++;
		a += *ptab * *--t4;
		
		*t1++ = *pp + a;
		*t2++ = *pp++ - a;
	}
	
	memcpy(p + k, buf, sizeof(double) * n);
}


