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

class FHT {
	int		m_exp2;
	int		m_num;
	float		*m_buf;
	float		*m_tab;

	void		makeCasTable();

public:
	FHT(int);
	~FHT();
	inline int	sizeExp() const { return m_exp2; }
	inline int	size() const { return m_num; }
	void 		__transform(float *, int, int);
	inline void	transform8(float *);
	void		transform(float *);
	inline float	_transform(float *);
	void		power(float *);
	float		_power(float *);
	float		*copy(float *, float *);
	float		*clear(float *);
	inline void	scale(float *, float);
};


