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

#ifndef PLAYLISTPLAYLISTALBUMGROUP_H
#define PLAYLISTPLAYLISTALBUMGROUP_H

#include <QList>


namespace Playlist {


enum GroupMode
{
    None = 1,
    Head,
    Head_Collapsed,
    Body,
    End,
    Collapsed
};

struct Group {

    QList < int > rows;
    bool collapsed;

};

enum OffsetMode
{
    OffsetNone = 0,
    OffsetBetween,
    OffsetAfter
};



/**
A helper class representing the group(s) of tracks for a specific album. For each album, the rows
of all tracks are added and this class keeps tracks of what kind of GroupMode each row should use

	@author 
*/
class AlbumGroup{
public:
    AlbumGroup( );

    ~AlbumGroup();

    void addRow( int row );
    int groupMode( int row );

    void setCollapsed( int row, bool collapsed );
    int elementsInGroup( int row );

    int firstInGroup( int row );
    int lastInGroup( int row );
    void removeGroup( int row );

    void removeBetween( int first, int last );
    void offsetBetween( int first, int last, int offset );

    int subgroupCount();

    void printGroupRows();

private:

    QList< Group > m_groups;

};

}

#endif
