/***************************************************************************
 * copyright        : (C) 2008 Daniel Caleb Jones <danielcjones@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/


#include "PlaylistRowList.h"
#include "PlaylistModel.h"

Playlist::RowList::RowList()
{
    Playlist::Model* model = The::playlistModel();

    connect( model, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( rowsInserted( const QModelIndex &, int, int ) ) );
    connect( model, SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), this, SLOT( rowsRemoved( const QModelIndex&, int, int ) ) );
    connect( model, SIGNAL( rowMoved( int, int ) ), this, SLOT( rowMoved( int, int ) ) );
}


void 
Playlist::RowList::rowsInserted( const QModelIndex & parent, int start, int end )
{
    Q_UNUSED( parent );

    int span = end - start + 1;

    RowList::iterator i;
    for( i = this->begin(); i != this->end(); ++i )
    {
        if( *i >= start ) *i += span;
    }
}

void
Playlist::RowList::rowsRemoved( const QModelIndex & parent, int start, int end )
{
    Q_UNUSED( parent );

    int span = end - start + 1;
    QMutableListIterator<int> i( *this );


    while( i.hasNext() )
    {
        i.next();

        if( i.value() >= start )
        {
            if( i.value() <= end ) i.remove();
            else                   i.value() -= span;
        }
    }
}

void 
Playlist::RowList::rowMoved( int from, int to )
{
    RowList::iterator i;
    for( i = begin(); i != end(); ++i )
    {
        if( *i == from ) *i = to;
        else if( from < *i && *i <= to ) *i -= 1;
        else if( to <= *i && *i < from ) *i += 1;
    }
}

