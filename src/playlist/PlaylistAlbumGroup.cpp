/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

#include "PlaylistAlbumGroup.h"

#include "debug.h"

namespace Playlist {

AlbumGroup::AlbumGroup()
{
}


AlbumGroup::~AlbumGroup()
{
}

}

void Playlist::AlbumGroup::addRow(int row)
{

    //Does this row fit in any of our existing groups?
    bool inGroup = false;
    for ( int i = 0; i < m_groups.count(); i++ ) {

        if ( m_groups[i].last() == row - 1 ) {
            m_groups[i].append( row );
            inGroup = true;
            break;
        }
    }

    //no group found, create new one:
    if ( !inGroup ) {
        QList< int > newGroup;
        newGroup.append( row );
        m_groups.append( newGroup );
    }
}

int Playlist::AlbumGroup::groupMode( int row )
{

    foreach( QList< int > group, m_groups ) {
        if ( group.contains( row ) ) {
            if ( group.count() < 2 )
                return None;
            else if ( group.first() == row )
                return Head;
            else if ( group.last() == row )
                return End;
            else
                return Body;
        }
    }

    return None;

}
