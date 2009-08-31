/****************************************************************************************
 * Copyright (c) 2005-2008 Last.fm Ltd. <copyright@last.fm>                             *
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

#ifndef LASTFM_WEIGHTED_STRING_LIST_H
#define LASTFM_WEIGHTED_STRING_LIST_H

#include "WeightedString.h"

#include <QtAlgorithms>
#include <QStringList>


class WeightedStringList : public QList<WeightedString>
{
	void reverse()
	{
		int const N = count();
		int const n = N/2;
		for (int i = 0; i < n; ++i)
			swap( i, N-i-1 );
	}
	
public:
    WeightedStringList()
	{}

    WeightedStringList( QList<WeightedString> list ) : QList<WeightedString>( list )
	{}

    operator QStringList()
    {
        QStringList strings;
        QListIterator<WeightedString> i( *this );
        while (i.hasNext())
            strings += i.next();
        return strings;
    }

    void weightedSort( Qt::SortOrder order = Qt::AscendingOrder ) 
    {
        qSort( begin(), end() );
		if (order == Qt::DescendingOrder)
			reverse();
    }
    
    void sort( Qt::SortOrder order = Qt::AscendingOrder )
    {
        qSort( begin(), end(), qLess<QString>() );
		if (order == Qt::DescendingOrder)
			reverse();
    }
};

#endif
