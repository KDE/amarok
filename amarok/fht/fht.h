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

class fht {
	void		makecastable(void);
	int		exp2;
	int		num;
	double		*buf;
	double		*tab;

public:
	fht(int exp);
	~fht();
	inline int	getexp(void) const { return exp2; }
	inline int	getwidth(void) const { return num; }
	void 		__transform(double *, int, int);
	inline void	transform8(double *);
	void		transform(double *);
	inline double	_transform(double *);
	void		power(double *);
	double		_power(double *);
	double		*copy(double *, double *);
	double		*clear(double *);
	inline void	scale(double *, double);
};


