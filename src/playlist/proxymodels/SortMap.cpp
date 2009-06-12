/***************************************************************************
 *   Copyright © 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "SortMap.h"

#include "Debug.h"
#include "FilterProxy.h"
#include "SortProxy.h"

namespace Playlist
{

SortMap::SortMap( FilterProxy *sourceProxy )
    : m_sorted( 0 )
{
    DEBUG_BLOCK
    m_sourceProxy = sourceProxy;    //FilterProxy::instance();
    m_rowCount = m_sourceProxy->rowCount();
    m_map = new QList< int >();
    for( int i = 0; i < m_rowCount; i++ )
        m_map->insert( i, i ); //identical function

}

SortMap::~SortMap()
{
    delete m_map;
}

int
SortMap::inv( int proxyRow )
{
    return m_map->indexOf( proxyRow );
}

int
SortMap::map( int sourceRow )
{
    debug() << "SortMap: map sourceRow=" << sourceRow;
    return m_map->value( sourceRow );   //note that if sourceRow>= size(), bad things will happen.
}

void
SortMap::sort( SortScheme *scheme )
{
    debug()<< "about to call qStableSort()";
    //MultilevelLessThan multilevelLessThan( m_sourceProxy, scheme );
    qStableSort( m_map->begin(), m_map->end(), MultilevelLessThan( m_sourceProxy, scheme) );
    m_sorted = 1;

    debug() << "Behold the mighty sorting map spamming your terminal:";
    debug() << "  source  sortProxy";
    for( int i = 0; i < m_map->length(); i++ )
    {
        debug() << "   " << i << "   " << m_map->value( i );
    }
}

void
SortMap::insertRows( int startRowInSource, int endRowInSource )
{
    //TODO: implement adding
    debug() << "Here I should be adding to the map sourceRows from " << startRowInSource << " to " << endRowInSource;
    m_sorted = 0;   //inserting rows surely invalidates the sorting.
}

void
SortMap::deleteRows( int startRowInSource, int endRowInSource )
{
    debug() << "Removing from the map sourceRows from " << startRowInSource << " to " << endRowInSource;
    QList< int >::iterator startIterator = m_map->begin();
    QList< int >::iterator endIterator = m_map->begin();
    startIterator += startRowInSource;
    endIterator += endRowInSource + 1;  //erase() wants the first item that *won't* be deleted as the end row
    m_map->erase( startIterator, endIterator ); //yay this should be O(1)
    // Let's now restore codomain elements consistency.
    // We have to make it so that the set of keys has the same elements as the set of values,
    // and the existing order has to be preserved.  -- Téo
    for( startIterator = m_map->begin(); startIterator != m_map->end(); ++startIterator )
    {
        if( *startIterator >= *endIterator )    //if the codomain index is 
        {
/*
A A'
0 1
1 3
2 5
3 2
4 0
5 4
what if I remove [3..4]?
A A'   should be:
0 1     0   (-1)
1 3     1   (-2)
2 5     3   (-2)
3 4     2   (-2)
*/
        }
    }
        // this is no good, I need to keep the values an uninterrupted sequence of integers between 0 and m_map->length() too!!!
        //NOTE TO SELF: and if I just use a sorting algorithm that does well with partially sorted lists (adaptive sort)? Could be almost O(n).
}

bool
MultilevelLessThan::operator()(int rowA, int rowB)
{
    quint8 verdict;  //0 = false  1 = true  2 = nextIteration
    for( int i = 0; i < m_scheme->length(); i++ )
    {
        int currentCategory = m_scheme->level( i ).category();  //see enum Column in PlaylistDefines.h
        QVariant dataA = m_sourceProxy->index( rowA, currentCategory ).data();  //FIXME: are you sure you need to do comparisons on sourceProxy indexes?
        QVariant dataB = m_sourceProxy->index( rowB, currentCategory ).data();  //or better, are you sure those rowA and rowB don't need a rowToSource around them?
        if( m_scheme->level( i ).isString() )
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
            if( m_scheme->level( i ).order() == Qt::AscendingOrder )
                verdict = verdict ? 0 : 1;
            break;
        }
    }
    return static_cast<bool>( verdict );
}

}   //namespace Playlist
