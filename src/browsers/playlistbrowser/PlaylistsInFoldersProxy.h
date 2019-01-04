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

#ifndef PLAYLISTSINFOLDERSPROXY_H
#define PLAYLISTSINFOLDERSPROXY_H

#include "QtGroupingProxy.h"
#include "PlaylistBrowserModel.h"

#include <QAction>

class QAction;

typedef QList<QPersistentModelIndex> QPersistentModelIndexList;

class PlaylistsInFoldersProxy : public QtGroupingProxy
{
    Q_OBJECT
    public:
        explicit PlaylistsInFoldersProxy( QAbstractItemModel *model );
        ~PlaylistsInFoldersProxy() override;

        /* PlaylistInGroupsProxy methods */
        QModelIndex createNewFolder( const QString &groupName );

        /* QtGroupingProxy methods */
        //re-implemented to make folder name (== label) editable.
        Qt::ItemFlags flags(const QModelIndex &idx) const override;
        QVariant data( const QModelIndex &idx, int role ) const override;

        /* QAbstractModel methods */
        bool removeRows( int row, int count,
                                 const QModelIndex &parent = QModelIndex() ) override;
        QStringList mimeTypes() const override;
        QMimeData *mimeData( const QModelIndexList &indexes ) const override;
        bool dropMimeData( const QMimeData *data, Qt::DropAction action,
                                   int row, int column, const QModelIndex &parent ) override;

        Qt::DropActions supportedDropActions() const override;
        Qt::DropActions supportedDragActions() const override;

        // re-implement to connect renameIndex signal
        void setSourceModel( QAbstractItemModel *sourceModel ) override;

    private Q_SLOTS:
        void slotRenameIndex( const QModelIndex &idx );

        void slotDeleteFolder();
        void slotRenameFolder();

    private:
        QList<QAction *> createGroupActions();
        void deleteFolder( const QModelIndex &groupIdx );

        QAction *m_renameFolderAction;
        QAction *m_deleteFolderAction;
};

Q_DECLARE_METATYPE(QPersistentModelIndexList);

#endif //PLAYLISTSINFOLDERSPROXY_H
