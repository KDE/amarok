/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
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
#ifndef PLAYLISTSBYPROVIDERPROXY_H
#define PLAYLISTSBYPROVIDERPROXY_H

#include "QtGroupingProxy.h"
#include "PlaylistBrowserModel.h"

#include <QAction>

class PlaylistsByProviderProxy : public QtGroupingProxy
{
    Q_OBJECT
    public:
        explicit PlaylistsByProviderProxy( int playlistCategory, QObject *parent = nullptr );
        PlaylistsByProviderProxy( QAbstractItemModel *model, int column, int playlistCategory );
        ~PlaylistsByProviderProxy() {}

        /* QtGroupingProxy methods */
        /* reimplement to handle tracks with multiple providers (synced) */
        QVariant data( const QModelIndex &idx, int role ) const override;

        /* reimplemented to prevent changing providers name */
        Qt::ItemFlags flags( const QModelIndex &idx ) const override;

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

    protected Q_SLOTS:
        //re-implemented to add empty providers
        void buildTree() override;

    private Q_SLOTS:
        void slotRenameIndex( const QModelIndex &index );
        void slotProviderAdded( Playlists::PlaylistProvider *provider, int category );
        void slotProviderRemoved( Playlists::PlaylistProvider *provider, int category );

    private:
        int m_playlistCategory;
};

#endif // PLAYLISTSBYPROVIDERPROXY_H
