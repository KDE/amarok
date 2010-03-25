/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef USERPLAYLISTMODEL_H
#define USERPLAYLISTMODEL_H

#include "MetaPlaylistModel.h"
#include "core/meta/Meta.h"
#include "core/playlists/Playlist.h"

#include <QAction>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

#define PLAYLIST_DB_VERSION 1

namespace PlaylistBrowserNS {

/**
        @author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class UserModel : public MetaPlaylistModel
{
    Q_OBJECT
    public:
        static UserModel * instance();
        static void destroy();

        ~UserModel();

        virtual bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );
        virtual bool removeRows( int row, int count, const QModelIndex & parent = QModelIndex() );

        virtual Qt::DropActions supportedDropActions() const {
            return Qt::CopyAction | Qt::MoveAction;
        }

        virtual Qt::DropActions supportedDragActions() const {
            return Qt::MoveAction | Qt::CopyAction;
        }

        bool dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );

    private:
        UserModel();
        static UserModel * s_instance;
};

}

namespace The {
    AMAROK_EXPORT PlaylistBrowserNS::UserModel* userPlaylistModel();
}
#endif
