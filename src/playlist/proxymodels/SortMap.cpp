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

namespace Playlist
{

SortMap::SortMap( qint64 rowCount )
    : m_sorted( 0 )
{
    DEBUG_BLOCK
    m_rowCount = rowCount;
    m_map = new QMap< qint64, qint64 >();
    for( qint64 i = 0; i < m_rowCount; i++ )
        m_map->insert( i, i ); //identical function

}

SortMap::~SortMap()
{
    delete m_map;
}

qint64
SortMap::inv( qint64 proxyRow )
{
    return m_map->key( proxyRow );
}

qint64
SortMap::map( qint64 sourceRow )
{
    return m_map->value( sourceRow );   //note that if sourceRow>= size(), bad things will happen.
}

void
SortMap::sort( const SortScheme &scheme )
{
    //sorting
    m_sorted = 1;
}

void
SortMap::insertRows( qint64 startRowInSource, qint64 endRowInSource )
{

    m_sorted = 0;   //inserting rows surely invalidates the sorting.
}

void
SortMap::deleteRows( qint64 stareRowInSource, qint64 endRowInSource )
{

}

}   //namespace Playlist
