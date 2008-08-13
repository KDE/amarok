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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "PlaylistAlbumGroup.h"

#include "Debug.h"

namespace Playlist {

AlbumGroup::AlbumGroup()
{
    //DEBUG_BLOCK
}


AlbumGroup::~AlbumGroup()
{
    //DEBUG_BLOCK
}

}

void Playlist::AlbumGroup::addRow(int row)
{
    //DEBUG_BLOCK

    //Does this row fit in any of our existing groups?
    bool inGroup = false;
    for ( int i = 0; i < m_groups.count(); i++ ) {

        if ( m_groups[i].rows.contains( row ) ) {
            inGroup = true;
            break;
        }
        else if ( m_groups[i].rows.last() == row - 1 ) {
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
    foreach( const Group &group, m_groups ) {
        if ( group.rows.contains( row ) ) {

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


bool Playlist::AlbumGroup::alternate( int row )
{
    if( m_groups.count() > 0) {
        foreach( const Group &group, m_groups ) {
            int index = group.rows.indexOf( row );
         if ( index != -1 ) {
                return ( index % 2 ) == 1;
            }
        }
    }
    return false;
}

void Playlist::AlbumGroup::setCollapsed(int row, bool collapsed)
{
    //DEBUG_BLOCK
    for (int i = 0; i < m_groups.count(); i++ ) {
        if ( m_groups[ i ].rows.contains( row ) ) {
            m_groups[ i ].collapsed = collapsed;
            //debug() << "row " << row << " collapsed = " << m_groups[ i ].collapsed;
        }
    }
}

int Playlist::AlbumGroup::elementsInGroup(int row)
{
    //DEBUG_BLOCK
    foreach( const Group &group, m_groups ) {
        if ( group.rows.contains( row ) ) {
            return group.rows.count();
        }
    }

    return 0;
}


int Playlist::AlbumGroup::firstInGroup(int row)
{
    DEBUG_BLOCK

    foreach( const Group &group, m_groups ) {
        if ( group.rows.contains( row ) ) {
            return group.rows.first();
        }
    }

    return -1;

}

int Playlist::AlbumGroup::lastInGroup(int row)
{
    DEBUG_BLOCK

    foreach( const Group &group, m_groups ) {
        if ( group.rows.contains( row ) ) {
            return group.rows.last();
        }
    }

    return -1;
}

void Playlist::AlbumGroup::removeGroup(int row)
{
    DEBUG_BLOCK

    for (int i = 0; i < m_groups.count(); i++ ) {
        if ( m_groups[ i ].rows.contains( row ) ) {
            m_groups.removeAt( i );
            return;
        }
    }


}

int Playlist::AlbumGroup::subgroupCount()
{
    return m_groups.count();
}

void Playlist::AlbumGroup::printGroupRows()
{
   foreach( const Group &group, m_groups ) {
        debug() << "Subgroup: " << group.rows;
  }

}

void Playlist::AlbumGroup::removeBetween(int first, int last)
{
    DEBUG_BLOCK
   debug() << "first: " << first << ", last: " << last;
    for ( int i = first; i <= last; i++ ) {
        for (int j = 0; j < m_groups.count(); j++ ) {
            if ( m_groups[ j ].rows.contains( i ) ) {
                    m_groups.removeAt( j );
            }
        }
    }

}


//when something is inserted or removed, all following indexes must be moved to match their actual new position.
void Playlist::AlbumGroup::offsetBetween(int first, int last, int offset)
{

    DEBUG_BLOCK
   debug() << "first: " << first << ", last: " << last;
    for (int j = 0; j < m_groups.count(); j++ ) {
        for ( int i = first; i <= last; i++ ) {
            if ( m_groups[ j ].rows.contains( i ) ) {
                //offset all in this group (so we don't break any groups)
                for ( int k = 0; k < m_groups[ j ].rows.count(); k++ ) {
                    m_groups[ j ].rows.append( m_groups[ j ].rows.takeFirst() + offset );
                }
                break;
            }
        }
    }
}



