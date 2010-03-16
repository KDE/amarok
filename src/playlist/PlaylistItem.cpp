/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
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

#define DEBUG_PREFIX "Playlist::Item"

#include "PlaylistItem.h"
#include "meta/capabilities/SourceInfoCapability.h"

#include <KRandom>

Playlist::Item::Item( Meta::TrackPtr track )
        : m_track( track ), m_state( NewlyAdded )
{
    m_id = ( static_cast<quint64>( KRandom::random() ) << 32 ) | static_cast<quint64>( KRandom::random() );
}

Playlist::Item::~Item()
{ }


// Does the same thing as:
//     foreach( quint64 val, set )
//         target.removeAll( val )
// but with O(n * log n) performance instead of O(n^2) if 'target' and 'set' are similar-sized.
void
Playlist::Item::listRemove( QList<quint64> &target, QSet<quint64> &removeSet )
{
    QMutableListIterator<quint64> iter( target );
    while ( iter.hasNext() )
        if ( removeSet.contains( iter.next() ) )
            iter.remove();
}
