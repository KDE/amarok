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
        explicit PlaylistsByProviderProxy( int playlistCategory, QObject *parent = 0 );
        PlaylistsByProviderProxy( QAbstractItemModel *model, int column, int playlistCategory );
        ~PlaylistsByProviderProxy() {}

        /* QtGroupingProxy methods */
        /* reimplement to handle tracks with multiple providers (synced) */
        virtual QVariant data( const QModelIndex &idx, int role ) const;

        /* reimplemented to prevent changing providers name */
        virtual Qt::ItemFlags flags( const QModelIndex &idx ) const;

        /* QAbstractModel methods */
        virtual bool removeRows( int row, int count,
                                 const QModelIndex &parent = QModelIndex() );
        virtual QStringList mimeTypes() const;
        virtual QMimeData *mimeData( const QModelIndexList &indexes ) const;
        virtual bool dropMimeData( const QMimeData *data, Qt::DropAction action,
                                   int row, int column, const QModelIndex &parent );

        virtual Qt::DropActions supportedDropActions() const;
        virtual Qt::DropActions supportedDragActions() const;

        // re-implement to connect renameIndex signal
        virtual void setSourceModel( QAbstractItemModel *sourceModel );

    Q_SIGNALS:
        void renameIndex( const QModelIndex &idx );

    protected Q_SLOTS:
        //re-implemented to add empty providers
        virtual void buildTree();

    private Q_SLOTS:
        void slotRenameIndex( const QModelIndex &index );
        void slotProviderAdded( Playlists::PlaylistProvider *provider, int category );
        void slotProviderRemoved( Playlists::PlaylistProvider *provider, int category );

    private:
        int m_playlistCategory;
};

#endif // PLAYLISTSBYPROVIDERPROXY_H
