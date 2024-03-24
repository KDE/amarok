/*
 * Copyright (C) 2017  Malte Veerman <malte.veerman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef ALBUMSENGINE_H
#define ALBUMSENGINE_H

#include "AlbumsModel.h"
#include "core/meta/Meta.h"

#include <QObject>

namespace Collections
{
    class QueryMaker;
}

class AlbumsEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY( AlbumsProxyModel* model READ model CONSTANT )
    Q_PROPERTY( QString filterPattern READ filterPattern WRITE setFilterPattern NOTIFY filterPatternChanged )

public:
    explicit AlbumsEngine( QObject *parent = nullptr );

    AlbumsProxyModel * model() const { return m_proxyModel; }
    QString filterPattern() const;
    void setFilterPattern( const QString &pattern );

    Q_INVOKABLE void showContextMenu( const QModelIndexList &indexes, const QModelIndex &mouseOverIndex ) const;
    Q_INVOKABLE QString getSelectedUrlList(const QModelIndexList &indexes) const;

Q_SIGNALS:
    void lengthAlignmentChanged();
    void filterPatternChanged();

private Q_SLOTS:
    void slotTrackChanged( const Meta::TrackPtr &track );
    void slotTrackMetadataChanged( Meta::TrackPtr track );
    void stopped();
    void resultReady( const Meta::AlbumList &albums );

private:
    void update();
    void updateRecentlyAddedAlbums();
    void clear();
    void appendSelected( const QModelIndexList &indexes ) const;
    void replaceWithSelected( const QModelIndexList &indexes ) const;
    void queueSelected( const QModelIndexList &indexes ) const;
    void editSelected( const QModelIndexList &indexes ) const;
    Meta::TrackList getSelectedTracks( const QModelIndexList &indexes ) const;

    Collections::QueryMaker *m_lastQueryMaker;
    Meta::TrackPtr m_currentTrack;
    Meta::ArtistPtr m_artist;

    AlbumsModel *m_model;
    AlbumsProxyModel *m_proxyModel;
};

#endif // ALBUMSENGINE_H
