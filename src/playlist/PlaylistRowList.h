/***************************************************************************
 * copyright            : (C) 2008 Daniel Caleb Jones <danielcjones@gmail.com>
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

#ifndef AMAROK_PLAYLISTROWLIST_H
#define AMAROK_PLAYLISTROWLIST_H

#include <QList>
#include <QModelIndex>
#include <QObject>


namespace Playlist
{
    /**
     * This is a list of row indexes that stays synced with the playlist model.
     * (i.e. the indexes change value when rows are added or removed from the
     * model.)
     */
    class RowList : public QObject, public QList<int>
    {
        Q_OBJECT

        public:
            RowList();

        private slots:
            void rowsInserted( const QModelIndex & parent, int start, int end );
            void rowsRemoved( const QModelIndex & parent, int start, int end );
            void rowMoved( int from, int to );
    };
}


#endif
