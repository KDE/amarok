// FHT - Fast Hartley Transform Class
//
// Copyright (C) 2004  Melchior FRANZ - mfranz@kde.org
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
// Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
//
// $Id$

#include <stdio.h>
#include <stdlib.h>
#include "fht.h"

int main(void)
{
	FHT f(5);  // 2^5 input values  =>  2^4 spectrum values
	int num = f.size(), i;
	
	double *s1 = new double[num];	// start values
	double *s2 = new double[num];	// Hartley coefficients
	double *s3 = new double[num];	// Fourier power spektrum

	for (i = 0; i < num; i++)
		s1[i] = 100.0 * rand() / (RAND_MAX + 1.0);
	
	f.copy(s2, s1);
	f.transform(s2);

	f.copy(s3, s1);
	f.power(s3);

	printf("Input\t\tFHT\t\tPower\n");
	for (i = 0; i < num; i++)
		if (i < num / 2)
			printf("%f\t%f\t%f\n", s1[i], s2[i], s3[i]);
		else
			printf("%f\t%f\n", s1[i], s2[i]);
	
	delete[] s1;
	delete[] s2;
	delete[] s3;
	return 0;
}


