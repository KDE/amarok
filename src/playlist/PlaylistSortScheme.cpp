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

#include "PlaylistSortScheme.h"

#include "Debug.h"

namespace Playlist
{

SortLevel::SortLevel( int sortCategory, Qt::SortOrder sortOrder )
    : m_category( sortCategory )
    , m_order( sortOrder )
{
    if( m_category == PlaceHolder )
        debug() << "Warning: Playlist::SortLevel: for some reason somebody has created a SortLevel with a placeholder as column.";
    if( m_category >= NUM_COLUMNS )
        debug() << "Error:   Playlist::SortLevel: column number overflow.";
}

int
SortLevel::category()
{
    return m_category;
}

Qt::SortOrder
SortLevel::order()
{
    return m_order;
}

void
SortLevel::setCategory(int sortCategory)
{
    m_category = sortCategory;
}

void
SortLevel::setOrder( Qt::SortOrder sortOrder )
{
    m_order = sortOrder;
}


// BEGIN SortScheme

SortScheme::SortScheme()
    : m_scheme( new QStack< SortLevel > )
{

}

SortLevel &
SortScheme::level( int i )
{
    return m_scheme->value( i, SortLevel( PlaceHolder ) );    //SortLevel( 0 ) is a dummy, as in PlaylistDefines.h 0=PlaceHolder
}

SortLevel &
SortScheme::operator[]( int i )
{
    return m_scheme->value( i, SortLevel( PlaceHolder ) );    //SortLevel( 0 ) is a dummy, as in PlaylistDefines.h 0=PlaceHolder
}

const SortLevel &
SortScheme::level( int i ) const
{
    return m_scheme->value( i, SortLevel( PlaceHolder ) );    //SortLevel( 0 ) is a dummy, as in PlaylistDefines.h 0=PlaceHolder
}

const SortLevel &
SortScheme::operator[]( int i ) const
{
    return m_scheme->value( i, SortLevel( PlaceHolder ) );    //SortLevel( 0 ) is a dummy, as in PlaylistDefines.h 0=PlaceHolder
}

void
SortScheme::addLevel( const Playlist::SortLevel& level )
{
    m_scheme->push( level );
}

int
SortScheme::length()
{
    return m_scheme->size();
}

void
SortScheme::trimToLevel( int lastLevel )
{
    for( int i = length - 1; i < lastLevel; i--)
        m_scheme->pop();
}

}   //namespace Playlist