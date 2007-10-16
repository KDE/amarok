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
    DEBUG_BLOCK
}


AlbumGroup::~AlbumGroup()
{
    DEBUG_BLOCK
}

}

void Playlist::AlbumGroup::addRow(int row)
{
    DEBUG_BLOCK

    //Does this row fit in any of our existing groups?
    bool inGroup = false;
    for ( int i = 0; i < m_groups.count(); i++ ) {

        if ( m_groups[i].rows.last() == row - 1 ) {
            m_groups[i].rows.append( row );
            inGroup = true;
            break;
        }
    }

    //no group found, create new one:
    if ( !inGroup ) {
        Group newGroup;
        newGroup.collapsed = false;
        newGroup.rows.append( row );
        m_groups.append( newGroup );
    }
}

int Playlist::AlbumGroup::groupMode( int row )
{
    DEBUG_BLOCK

    foreach( Group group, m_groups ) {
        if ( group.rows.contains( row ) ) {

            debug() << "row " << row << " is collapsed= " << group.collapsed;

            if ( group.rows.count() < 2 )
                return None;
            else if ( group.rows.first() == row ) {
                if ( !group.collapsed )
                    return Head;
                else 
                    return Head_Collapsed;
            } else if ( group.rows.last() == row ) {
                if ( !group.collapsed )
                    return End;
                else
                return Collapsed;
            } else {
                if ( !group.collapsed )
                    return Body;
                else
                    return Collapsed;
            }
        }
    }

    return None;

}

void Playlist::AlbumGroup::setCollapsed(int row, bool collapsed)
{
    DEBUG_BLOCK
    for (int i = 0; i < m_groups.count(); i++ ) {
        if ( m_groups[ i ].rows.contains( row ) ) {
            m_groups[ i ].collapsed = collapsed;
            debug() << "row " << row << " collapsed = " << m_groups[ i ].collapsed;
        }
    }
}

int Playlist::AlbumGroup::elementsInGroup(int row)
{
    DEBUG_BLOCK
    foreach( Group group, m_groups ) {
        if ( group.rows.contains( row ) ) {
            return group.rows.count();
        }
    }

    return 0;
}


int Playlist::AlbumGroup::firstInGroup(int row)
{

    foreach( Group group, m_groups ) {
        if ( group.rows.contains( row ) ) {
            return group.rows.first();
        }
    }

}

int Playlist::AlbumGroup::lastInGroup(int row)
{

    foreach( Group group, m_groups ) {
        if ( group.rows.contains( row ) ) {
            return group.rows.last();
        }
    }
}

void Playlist::AlbumGroup::removeGroup(int row)
{

    for (int i = 0; i < m_groups.count(); i++ ) {
        if ( m_groups[ i ].rows.contains( row ) ) {
            m_groups.removeAt( i );
            return;
        }
    }


}