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
    for( int i = startRowInSource; i <= endRowInSource; i++ )
    {
        m_map->removeAt( i );
        debug() << "          Removing from map row=" << i;
        // this is no good, I need to keep the values an uninterrupted sequence of integers between 0 and m_map->length() too!!!
        // the complexity of this is going to hell :(
    }
}


bool
MultilevelLessThan::operator()(int rowA, int rowB)
{
    quint8 verdict;  //0 = false  1 = true  2 = nextIteration
    for( int i = 0; i < m_scheme->length(); i++ )
    {
        int currentCategory = m_scheme->level( i ).category();  //see enum Column in PlaylistDefines.h
        Qt::SortOrder currentOrder = m_scheme->level( i ).order();
        QVariant dataA = m_sourceProxy->index( rowA, currentCategory ).data();  //FIXME: are you sure you need to do comparisons on sourceProxy indexes?
        QVariant dataB = m_sourceProxy->index( rowB, currentCategory ).data();  //or better, are you sure those rowA and rowB don't need a rowToSource around them?
        if( m_scheme->level( i ).isString() )
        {
            if( dataA.toString() < dataB.toString() )
            {
                verdict = 1;
            }
            else if( dataA.toString() > dataB.toString() )
            {
                verdict = 0;
            }
            verdict = 2;
        }
        else //if it's not a string ==> it's a number
        {
            if( dataA.toInt() < dataB.toInt() )
            {
                verdict = 1;
            }
            else if( dataA.toInt() > dataB.toInt() )
            {
                verdict = 0;
            }
            verdict = 2;
        }
        if( verdict != 2 )
            break;
    }
    return static_cast<bool>( verdict );
}

}   //namespace Playlist
