/* This file is part of the KDE project
   Copyright (C) 2009 Bart Cerneels <bart.cerneels@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#ifndef AMAROK_PLAYLISTSINGROUPSPROXY_H
#define AMAROK_PLAYLISTSINGROUPSPROXY_H

#include "MetaPlaylistModel.h"

#include "context/popupdropper/libpud/PopupDropperAction.h"

#include <QAbstractProxyModel>
#include <QModelIndex>
#include <QMultiHash>
#include <QStringList>

class PopupDropperAction;

class PlaylistsInGroupsProxy : public PlaylistBrowserNS::MetaPlaylistModel
{
    Q_OBJECT
    public:
        PlaylistsInGroupsProxy( PlaylistBrowserNS::MetaPlaylistModel *model );
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

        QList<PopupDropperAction *> actionsFor( const QModelIndexList &indexes );

        void loadItems( QModelIndexList list, Playlist::AddOptions insertMode );

    signals:
        void rowsInserted( const QModelIndex&, int, int );
        void rowsRemoved( const QModelIndex&, int, int );

    private slots:
        void modelDataChanged( const QModelIndex&, const QModelIndex& );
        void modelRowsInserted( const QModelIndex&, int, int );
        void modelRowsRemoved( const QModelIndex&, int, int );

        void slotDeleteGroup();
        void slotRenameGroup();

    private:
        void buildTree();
        QModelIndexList mapToSource( const QModelIndexList& list ) const;
        QList<PopupDropperAction *> createGroupActions();
        bool isAGroupSelected( const QModelIndexList& list ) const;
        bool isAPlaylistSelected( const QModelIndexList& list ) const;

        MetaPlaylistModel *m_model;
        PopupDropperAction *m_renameAction;
        PopupDropperAction *m_deleteAction;

        QMultiHash<qint64, int> m_groupHash;
        QStringList m_groupNames;
        QModelIndexList m_selectedGroups;
};

#endif //AMAROK_PLAYLISTSINGROUPSPROXY_H