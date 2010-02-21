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
#include "MetaPlaylistModel.h"

#include <QAction>

class PlaylistsByProviderProxy : public QtGroupingProxy
        , public PlaylistBrowserNS::MetaPlaylistModel
{
    Q_OBJECT
    public:
        static QByteArray encodeMimeRows( QList<int> rowList );
        static QList<int> decodeMimeRows( QByteArray data );
        static const QString AMAROK_PROVIDERPROXY_INDEXES;

        PlaylistsByProviderProxy( QAbstractItemModel *model, int column );
        ~PlaylistsByProviderProxy() {}

        /* QAbstractModel methods */
        virtual bool removeRows( int row, int count,
                                 const QModelIndex &parent = QModelIndex() );
        virtual QStringList mimeTypes() const;
        virtual QMimeData *mimeData( const QModelIndexList &indexes ) const;
        virtual bool dropMimeData( const QMimeData *data, Qt::DropAction action,
                                   int row, int column, const QModelIndex &parent );

        virtual Qt::DropActions supportedDropActions() const;
        virtual Qt::DropActions supportedDragActions() const;

        /* MetaPlaylistModel methods */
        virtual QList<QAction *> actionsFor( const QModelIndexList &indexes );

        virtual void loadItems( QModelIndexList list, Playlist::AddOptions insertMode );

    signals:
        void renameIndex( QModelIndex idx );

    protected slots:
        virtual void buildTree();

    private slots:
        void slotRename( QModelIndex idx );

};

#endif // PLAYLISTSBYPROVIDERPROXY_H
