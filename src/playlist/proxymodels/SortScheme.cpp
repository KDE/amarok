/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "SortScheme.h"
#include "playlist/PlaylistDefines.h"

#include "core/support/Debug.h"

namespace Playlist
{

SortLevel::SortLevel( Column sortCategory, Qt::SortOrder sortOrder )
    : m_category( sortCategory )
    , m_order( sortOrder )
{
    if( m_category == PlaceHolder )
        debug() << "Warning: Playlist::SortLevel: for some reason somebody has created a SortLevel with a placeholder as column.";
    if( m_category >= NUM_COLUMNS )
        debug() << "Error:   Playlist::SortLevel: column number overflow.";
}

Column
SortLevel::category() const
{
    return m_category;
}

Qt::SortOrder
SortLevel::order() const
{
    return m_order;
}

void
SortLevel::setCategory(Column sortCategory)
{
    m_category = sortCategory;
}

void
SortLevel::setOrder( Qt::SortOrder sortOrder )
{
    m_order = sortOrder;
}

bool
SortLevel::isComparable() const
{
    return isSortableColumn( category() );
}

bool
SortLevel::isString() const
{
    QList< Column > strCategories;
    strCategories << Album << AlbumArtist << Artist << Comment << Composer << Directory << Filename
        << Genre << LastPlayed << Source << Title << Type << Year;
    if( isComparable() && strCategories.contains( category() ) )
        return true;
    return false;
}

bool
SortLevel::isFloat() const
{
    QList< Column > floatCategories;
    floatCategories << Bpm;
    if( isComparable() && floatCategories.contains( category() ) )
        return true;
    return false;
}

QString
SortLevel::prettyName() const
{
    return columnName( m_category );
}

// BEGIN SortScheme

const SortLevel &
SortScheme::level( int i ) const
{
    //SortLevel( 0 ) is a dummy, as in PlaylistDefines.h 0=PlaceHolder
    return (*this)[ i ];
}

void
SortScheme::addLevel( const Playlist::SortLevel& level )
{
    push( level );
}

int
SortScheme::length() const
{
    return size();
}

void
SortScheme::trimToLevel( int lastLevel )
{
    while( size() > lastLevel )
        pop();
}

SortScheme::const_iterator
SortScheme::begin() const
{
    return QStack::begin();
}

SortScheme::const_iterator
SortScheme::end() const
{
    return QStack::end();
}

}   //namespace Playlist
