/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "SingleTracksModel.h"

#include "AmarokMimeData.h"
#include "MetaValues.h"
#include "core/meta/support/MetaConstants.h"

using namespace StatSyncing;

SingleTracksModel::SingleTracksModel( const TrackList &tracks,
                                      const QList<qint64> &columns, const Options &options,
                                      QObject *parent )
    : QAbstractTableModel( parent )
    , CommonModel( columns, options )
    , m_tracks( tracks )
{
}

int
SingleTracksModel::rowCount( const QModelIndex &parent ) const
{
    return parent.isValid() ? 0 : m_tracks.count();
}

int
SingleTracksModel::columnCount( const QModelIndex &parent ) const
{
    return parent.isValid() ? 0 : m_columns.count();
}

QVariant
SingleTracksModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    return CommonModel::headerData( section, orientation, role );
}

QVariant
SingleTracksModel::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() ||
        index.row() < 0 || index.row() >= m_tracks.count() ||
        index.column() < 0 || index.column() >= m_columns.count() )
    {
        return QVariant();
    }

    qint64 field = m_columns.at( index.column() );
    const TrackPtr &track = m_tracks.at( index.row() );
    return trackData( track, field, role );
}

Qt::ItemFlags
SingleTracksModel::flags( const QModelIndex &index ) const
{
    return QAbstractItemModel::flags( index ) | Qt::ItemIsDragEnabled;
}

QStringList
SingleTracksModel::mimeTypes() const
{
    return QStringList() << AmarokMimeData::TRACK_MIME << QStringLiteral("text/uri-list") << QStringLiteral("text/plain");
}

QMimeData *
SingleTracksModel::mimeData( const QModelIndexList &indexes ) const
{
    Meta::TrackList tracks;
    for( const QModelIndex &idx : indexes )
    {
        if( idx.isValid() && idx.row() >= 0 && idx.row() < m_tracks.count() &&
            idx.column() == 0 )
        {
            Meta::TrackPtr metaTrack = m_tracks.at( idx.row() )->metaTrack();
            if( metaTrack )
                tracks << metaTrack;
        }
    }
    if( tracks.isEmpty() )
        return nullptr;

    AmarokMimeData *mime = new AmarokMimeData();
    mime->setTracks( tracks );
    return mime;
}
