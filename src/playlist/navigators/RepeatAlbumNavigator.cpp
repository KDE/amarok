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

#define DEBUG_PREFIX "Playlist::RepeatAlbumNavigator"

#include "RepeatAlbumNavigator.h"

#include "core/support/Debug.h"
#include "core/meta/Meta.h"


Playlist::RepeatAlbumNavigator::RepeatAlbumNavigator()
{
    loadFromSourceModel();
}

void
Playlist::RepeatAlbumNavigator::planOne()
{
    DEBUG_BLOCK

    if ( m_plannedItems.isEmpty() )
    {
        ItemList itemsInAlbum = m_itemsPerAlbum.value( currentAlbum() );    // May be default-constructed empty list.

        int currentRow = itemsInAlbum.indexOf( currentItem() );
        if ( currentRow != -1 )
        {
            int nextRow = (currentRow + 1) % itemsInAlbum.size();    // Circulate within same album
            m_plannedItems.append( itemsInAlbum.at( nextRow ) );
        }
        else    // This happens if 'currentItem() == 0'.
        {
            quint64 item = bestFallbackItem();
            if ( item )
                m_plannedItems.append( item );
        }
    }
}
