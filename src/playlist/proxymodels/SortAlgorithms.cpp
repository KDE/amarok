/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#include "SortAlgorithms.h"

#include "Debug.h"

namespace Playlist
{

bool
multilevelLessThan::operator()( int rowA, int rowB)
{
    quint8 verdict = 0;  //0 = false  1 = true  2 = nextIteration
    for( int i = 0; i < m_scheme.length(); i++ )
    {
        int currentCategory = m_scheme.level( i ).category();  //see enum Column in PlaylistDefines.h
        if( currentCategory == -1 ) //random
            return static_cast<bool>( qrand() % 2 );
        QVariant dataA = m_sourceProxy->index( rowA, currentCategory ).data();  //FIXME: are you sure you need to do comparisons on sourceProxy indexes?
        QVariant dataB = m_sourceProxy->index( rowB, currentCategory ).data();  //or better, are you sure those rowA and rowB don't need a rowToSource around them?
        if( m_scheme.level( i ).isString() )
        {
            if( dataA.toString() < dataB.toString() )
                verdict = 1;
            else if( dataA.toString() > dataB.toString() )
                verdict = 0;
            else
                verdict = 2;
        }
        else //if it's not a string ==> it's a number
        {
            if( dataA.toInt() < dataB.toInt() )
                verdict = 1;
            else if( dataA.toInt() > dataB.toInt() )
                verdict = 0;
            else
                verdict = 2;
        }
        if( verdict != 2 )
        {
            if( m_scheme.level( i ).order() == Qt::DescendingOrder )
                verdict = verdict ? 0 : 1;
            break;
        }
        else
        {
            if( m_scheme.level( i ).order() == Qt::DescendingOrder )
                verdict = verdict ? 0 : 1;
        }
    }
    if( verdict == 2 )
    {
        if( m_scheme.level( m_scheme.length() - 1 ).order() == Qt::DescendingOrder )
            verdict = 1;
        else
            verdict = 0;
    }
    return static_cast<bool>( verdict );
}

}   //namespace Playlist
