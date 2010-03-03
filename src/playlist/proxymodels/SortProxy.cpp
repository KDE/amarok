/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
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

#include "SortProxy.h"

namespace Playlist
{

// Note: the 'sort' mode of QSortFilterProxyModel can emit QAbstractItemModel::layoutChanged signals.

// Note: the QSortFilterProxyModel sorting is always on, even with an empty SortScheme.
//         - That case does not seem worth special-casing
//         - Cleanly "disabling" QSortFilterProxyModel sort mode is "under-documented"
//           and non-trivial (in Qt 4.6 at least).

SortProxy::SortProxy( AbstractModel *belowModel, QObject *parent )
    : ProxyBase( parent )
{
    m_belowModel = belowModel;
    setSourceModel( dynamic_cast< QAbstractItemModel * >( m_belowModel ) );

    // Tell QSortFilterProxyModel: keep the filter correct when the underlying source model changes.
    // Qt will do this by receiving the standard QAbstractItemModel signals: dataChanged, rowsInserted, etc.
    setDynamicSortFilter( true );

    // Tell QSortFilterProxyModel: activate sorting.
    sort( 0 );    // 0 is a dummy column.

    // Proxy the Playlist::AbstractModel signals
    //As this Proxy doesn't add or remove tracks, and unique track IDs must be left untouched
    //by sorting, they may be just blindly forwarded
    connect( sourceModel(), SIGNAL( activeTrackChanged( const quint64 ) ), this, SIGNAL( activeTrackChanged( quint64 ) ) );
    connect( sourceModel(), SIGNAL( beginRemoveIds() ), this, SIGNAL( beginRemoveIds() ) );
    connect( sourceModel(), SIGNAL( insertedIds( const QList<quint64>& ) ), this, SIGNAL( insertedIds( const QList< quint64>& ) ) );
    connect( sourceModel(), SIGNAL( metadataUpdated() ), this, SIGNAL( metadataUpdated() ) );
    connect( sourceModel(), SIGNAL( removedIds( const QList<quint64>& ) ), this, SIGNAL( removedIds( const QList< quint64 >& ) ) );
    connect( sourceModel(), SIGNAL( queueChanged() ), this, SIGNAL( queueChanged() ) );
}

SortProxy::~SortProxy()
{}

bool
SortProxy::lessThan( const QModelIndex & sourceModelIndexA, const QModelIndex & sourceModelIndexB ) const
{
    int rowA = sourceModelIndexA.row();
    int rowB = sourceModelIndexB.row();
    return m_mlt( sourceModel(), rowA, rowB );
}

void
SortProxy::updateSortMap( SortScheme scheme )
{
    m_scheme = scheme;
    m_mlt.setSortScheme( m_scheme );

    invalidate();    // Tell QSortFilterProxyModel: re-sort
}


// Pass-through public methods, basically identical to those in Playlist::FilterProxy, that
// pretty much just forward stuff through the stack of proxies start here.
// Please keep them sorted alphabetically.  -- Téo

int
SortProxy::rowFromSource( int row ) const
{
    QModelIndex sourceIndex = sourceModel()->index( row, 0 );
    QModelIndex index = mapFromSource( sourceIndex );

    if ( !index.isValid() )
        return -1;
    return index.row();
}

int
SortProxy::rowToSource( int row ) const
{
    QModelIndex index = this->index( row, 0 );
    QModelIndex sourceIndex = SortProxy::mapToSource( index );
    if ( !sourceIndex.isValid() )
        return ( row == rowCount() ) ? m_belowModel->rowCount() : -1;
    return sourceIndex.row();
}

}   //namespace Playlist
