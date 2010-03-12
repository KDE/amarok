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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef PLAYLISTSINGROUPSPROXY_H
#define PLAYLISTSINGROUPSPROXY_H

#include "QtGroupingProxy.h"
#include "MetaPlaylistModel.h"

#include <QAction>

class QAction;

class PlaylistsInGroupsProxy : public QtGroupingProxy
{
    Q_OBJECT
    public:
        PlaylistsInGroupsProxy( QAbstractItemModel *model );
        ~PlaylistsInGroupsProxy();

        /* PlaylistInGroupsProxy methods */
        QModelIndex createNewGroup( const QString &groupName );

        /* QAbstractModel methods */
        virtual bool removeRows( int row, int count,
                                 const QModelIndex &parent = QModelIndex() );
        virtual QStringList mimeTypes() const;
        virtual QMimeData *mimeData( const QModelIndexList &indexes ) const;
        virtual bool dropMimeData( const QMimeData *data, Qt::DropAction action,
                                   int row, int column, const QModelIndex &parent );

        virtual Qt::DropActions supportedDropActions() const;
        virtual Qt::DropActions supportedDragActions() const;

    signals:
        void renameIndex( QModelIndex idx );

    private slots:
        void slotRename( QModelIndex idx );

        void slotDeleteFolder();
        void slotRenameFolder();

    private:
        QList<QAction *> createGroupActions();
        bool isAPlaylistSelected( const QModelIndexList& list ) const;
        void deleteFolder( const QModelIndex &groupIdx );

        QAction *m_renameFolderAction;
        QAction *m_deleteFolderAction;

        QModelIndexList m_selectedGroups;
        QModelIndexList m_selectedPlaylists;
};

#endif //PLAYLISTSINGROUPSPROXY_H
