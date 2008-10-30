/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef LASTFM_WEIGHTED_STRING_H
#define LASTFM_WEIGHTED_STRING_H

#include <lastfm/DllExportMacro.h>
#include <QString>


class WeightedString : public QString
{
    float m_weighting;

public:
    WeightedString()
	{
		m_weighting = -1.0f;
	}
    
	explicit WeightedString( const QString& name, float w = -1.0f ) : QString( name ), m_weighting( w ) 
	{}

    int weighting() const { return m_weighting; }

	bool operator<( const WeightedString& that ) const
	{
		return this->weighting() < that.weighting();
	}
};


#endif
