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
    : ProxyBase( belowModel, parent )
{
    // Tell QSortFilterProxyModel: keep the filter correct when the underlying source model changes.
    // Qt will do this by receiving the standard QAbstractItemModel signals: dataChanged, rowsInserted, etc.
    setDynamicSortFilter( true );

    // Tell QSortFilterProxyModel: activate sorting.
    sort( 0 );    // 0 is a dummy column.
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

}   //namespace Playlist
