/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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

#ifndef USERPLAYLISTMODEL_H
#define USERPLAYLISTMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

#include "Meta.h"
#include "meta/Playlist.h"
// #include "meta/PlaylistGroup.h"

// class SqlPlaylistGroup;
// 
// class SqlPlaylistViewItem;
// typedef KSharedPtr<SqlPlaylistViewItem> SqlPlaylistViewItemPtr;
// 
// class SqlPlaylistGroup;
// typedef KSharedPtr<SqlPlaylistGroup> SqlPlaylistGroupPtr;
// typedef QList<SqlPlaylistGroupPtr> SqlPlaylistGroupList;


#define PLAYLIST_DB_VERSION 1

namespace PlaylistBrowserNS {

/**
	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class UserModel : public QAbstractItemModel
{
    Q_OBJECT
    public:
        static UserModel * instance();

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

        virtual Qt::DropActions supportedDropActions() const{
            return Qt::MoveAction;
        }

        virtual QStringList mimeTypes() const;
        QMimeData* mimeData( const QModelIndexList &indexes ) const;
        bool dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );

        void reloadFromDb();
        void editPlaylist( int id );
//         void createNewStream( const QString& streamName, const Meta::TrackPtr& streamTrack );
//         QModelIndex createIndex( int row, int column, SqlPlaylistViewItemPtr item ) const;
        //only use the above method
//         QModelIndex createIndex( int, int, void * ptr = 0) const { Q_UNUSED( ptr ); Q_ASSERT( 0 );  return QModelIndex(); }
//         QModelIndex createIndex( int, int, quint32 ) const { Q_ASSERT( 0 ); return QModelIndex(); }
    public slots:
//         void createNewGroup();

    signals:
        void editIndex( const QModelIndex & index );

    private:
        UserModel();

        static UserModel * s_instance;

//         SqlPlaylistGroupPtr m_root;
//        mutable QHash<quint32, SqlPlaylistViewItemPtr> m_viewItems; ///the hash of the pointer mapped to the KSharedPtr

        Meta::PlaylistList m_playlists;

};

}

#endif
