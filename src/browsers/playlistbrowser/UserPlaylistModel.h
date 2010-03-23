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
class UserModel : public QAbstractItemModel, public MetaPlaylistModel,
                  public Meta::PlaylistObserver
{
    Q_OBJECT
    public:
        static UserModel * instance();
        static void destroy();

        ~UserModel();

        virtual QVariant data( const QModelIndex &index, int role ) const;
        virtual Qt::ItemFlags flags( const QModelIndex &index ) const;
        virtual QVariant headerData( int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole ) const;
        virtual QModelIndex index( int row, int column,
                        const QModelIndex &parent = QModelIndex() ) const;
        virtual QModelIndex parent( const QModelIndex &index ) const;
        virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;
        virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const;
        virtual bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );
        virtual bool removeRows( int row, int count, const QModelIndex & parent = QModelIndex() );

        virtual Qt::DropActions supportedDropActions() const {
            return Qt::CopyAction | Qt::MoveAction;
        }

        virtual Qt::DropActions supportedDragActions() const {
            return Qt::MoveAction | Qt::CopyAction;
        }

        virtual QStringList mimeTypes() const;
        QMimeData* mimeData( const QModelIndexList &indexes ) const;
        bool dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );

        void loadItems( QModelIndexList list, Playlist::AddOptions insertMode );

        /* Meta::PlaylistObserver methods */
        virtual void trackAdded( Meta::PlaylistPtr playlist, Meta::TrackPtr track,
                                 int position );
        virtual void trackRemoved( Meta::PlaylistPtr playlist, int position );

    public slots:
        void slotLoad();
        void slotAppend();

        void slotUpdate();
        void slotRenamePlaylist( Meta::PlaylistPtr playlist );

    signals:
        void renameIndex( const QModelIndex &index );
        void rowsInserted( const QModelIndex &parent, int start, int end );

    private:
        UserModel();
        QList<QAction *> actionsFor( const QModelIndex &idx ) const;
        Meta::TrackPtr trackFromIndex( const QModelIndex &index ) const;
        Meta::PlaylistPtr playlistFromIndex( const QModelIndex &index ) const;
        Meta::TrackList tracksFromIndexes( const QModelIndexList &list ) const;

        void loadPlaylists();

        static UserModel * s_instance;

        Meta::PlaylistList m_playlists;
        QAction *m_appendAction;
        QAction *m_loadAction;
};

}

Q_DECLARE_METATYPE( QModelIndexList )

namespace The {
    AMAROK_EXPORT PlaylistBrowserNS::UserModel* userPlaylistModel();
}
#endif
