/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "Playlist::RandomAlbumNavigator"

#include "RandomAlbumNavigator.h"

#include "core/support/Debug.h"
#include "core/meta/Meta.h"

#include <algorithm> // For std::random_shuffle


Playlist::RandomAlbumNavigator::RandomAlbumNavigator()
{
    loadFromSourceModel();
}


void
Playlist::RandomAlbumNavigator::planOne()
{
    DEBUG_BLOCK

    // Try to find next item in same album
    if ( m_plannedItems.isEmpty() )
    {
        ItemList itemsInAlbum = m_itemsPerAlbum.value( currentAlbum() );    // May be default-constructed empty list.

        int currentRow = itemsInAlbum.indexOf( currentItem() );    // -1 if currentItem() == 0.
        if ( currentRow != -1 )
        {
            int nextRow = currentRow + 1;
            if ( nextRow < itemsInAlbum.size() )
                m_plannedItems.append( itemsInAlbum.at( nextRow ) );
        }
    }

    // Try to find first item in next album
    if ( m_plannedItems.isEmpty() )
    {
        if ( m_plannedAlbums.isEmpty() )    // Handle end of planned album list
            notifyAlbumsInserted( m_itemsPerAlbum.uniqueKeys() );

        if ( !m_plannedAlbums.isEmpty() )
        {
            AlbumId newAlbum = m_plannedAlbums.takeFirst();
            quint64 newCurrentItem = m_itemsPerAlbum.value( newAlbum ).first();
            m_plannedItems.append( newCurrentItem );
        }
    }
}

void
Playlist::RandomAlbumNavigator::notifyAlbumsInserted( QList<AlbumId> insertedAlbums )
{
    DEBUG_BLOCK

    m_plannedAlbums.append( insertedAlbums );
    std::random_shuffle( m_plannedAlbums.begin(), m_plannedAlbums.end() );
    if ( !m_plannedAlbums.isEmpty() )
        if ( m_plannedAlbums.first() == currentAlbum() )
            m_plannedAlbums.append( m_plannedAlbums.takeFirst() );    // Try to avoid playing same album twice.
}
