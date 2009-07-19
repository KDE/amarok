/****************************************************************************************
 * Copyright (c) 2009 Bart Cerneels <bart.cerneels@kde.org>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_PLAYLISTSINGROUPSPROXY_H
#define AMAROK_PLAYLISTSINGROUPSPROXY_H

#include "MetaPlaylistModel.h"

#include "context/popupdropper/libpud/PopupDropperAction.h"

#include <QAbstractProxyModel>
#include <QModelIndex>
#include <QMultiHash>
#include <QStringList>

class PopupDropperAction;

class PlaylistsInGroupsProxy :  public QAbstractProxyModel,
                                public PlaylistBrowserNS::MetaPlaylistModel
{
    Q_OBJECT
    public:
        PlaylistsInGroupsProxy( QAbstractItemModel *model );
        ~PlaylistsInGroupsProxy();

        // functions from QAbstractProxyModel
        QModelIndex index( int, int c = 0, const QModelIndex& parent = QModelIndex() ) const;
        Qt::ItemFlags flags( const QModelIndex &index ) const;
        QModelIndex parent( const QModelIndex& ) const;
        int rowCount( const QModelIndex& idx = QModelIndex() ) const;
        int columnCount( const QModelIndex& ) const;
        QModelIndex mapToSource( const QModelIndex& ) const;
        QModelIndex mapFromSource( const QModelIndex& ) const;
        QVariant data( const QModelIndex &index, int role ) const;
        virtual bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );
        virtual bool removeRows( int row, int count, const QModelIndex & parent = QModelIndex() );
        virtual QStringList mimeTypes() const;
        virtual QMimeData *mimeData( const QModelIndexList &indexes ) const;
        virtual bool dropMimeData( const QMimeData *data, Qt::DropAction action,
                                   int row, int column, const QModelIndex &parent );

        virtual Qt::DropActions supportedDropActions() const;
        virtual Qt::DropActions supportedDragActions() const;

        QList<PopupDropperAction *> actionsFor( const QModelIndexList &indexes );

        void loadItems( QModelIndexList list, Playlist::AddOptions insertMode );

        QModelIndex createNewGroup( const QString &groupName );

    signals:
        void rowsInserted( const QModelIndex&, int, int );
        void rowsRemoved( const QModelIndex&, int, int );
        void layoutAboutToBeChanged();
        void layoutChanged();
        void renameIndex( QModelIndex idx );

    private slots:
        void modelDataChanged( const QModelIndex&, const QModelIndex& );
        void modelRowsInserted( const QModelIndex&, int, int );
        void modelRowsRemoved( const QModelIndex&, int, int );
        void modelRowsAboutToBeRemoved( const QModelIndex&, int, int );
        void slotRename( QModelIndex idx );
        void buildTree();

        void slotDeleteGroup();
        void slotRenameGroup();
        void slotAddToGroup();

    private:
        bool isGroup( const QModelIndex &index ) const;
        QModelIndexList mapToSource( const QModelIndexList& list ) const;
        QList<PopupDropperAction *> createGroupActions();
        bool isAGroupSelected( const QModelIndexList& list ) const;
        bool isAPlaylistSelected( const QModelIndexList& list ) const;
        bool changeGroupName( const QString &from, const QString &to );

        void deleteGroup( const QModelIndex &groupIdx );

        QAbstractItemModel *m_model;
        PopupDropperAction *m_renameAction;
        PopupDropperAction *m_deleteAction;

        QMultiHash<quint32, int> m_groupHash;
        QStringList m_groupNames;

        /** "instuctions" how to create a item in the tree.
        This is used by parent( QModelIndex )
        */
        struct ParentCreate
        {
            int parentCreateIndex;
            int row;
        };
        mutable QList<struct ParentCreate> m_parentCreateList;
        /** @returns index of the "instructions" to recreate the parent. Will create new if it doesn't exist yet.
        */
        int indexOfParentCreate( const QModelIndex &parent ) const;

        QModelIndexList m_selectedGroups;
        QModelIndexList m_selectedPlaylists;
};

#endif //AMAROK_PLAYLISTSINGROUPSPROXY_H
