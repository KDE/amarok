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

#include "SortFilterProxy.h"

#include "SortAlgorithms.h"
#include "SortScheme.h"

#include "core/support/Amarok.h"
#include "amarokconfig.h"


// Note: the 'sort' mode of QSortFilterProxyModel can emit QAbstractItemModel::layoutChanged signals.

// Note: the QSortFilterProxyModel sorting is always on, even with an empty SortScheme.
//         - That case does not seem worth special-casing
//         - Cleanly "disabling" QSortFilterProxyModel sort mode is "under-documented"
//           and non-trivial (in Qt 4.6 at least).

// Note: the sort and filter functions have been combined into 1 QSFPM because that gives
//       optimal performance. If you have a reason to split sorting and filtering up into
//       separate QSFPMs again: be sure to put the SortProxy closer to the bottom than
//       the FilterProxy. Otherwise the FilterProxy's nicely-grouped 'rowsRemoved'
//       signals (when changing the filter) get fragmented into thousands of pieces by
//       the SortProxy, causing lousy performance on large playlists.


namespace Playlist {

SortFilterProxy::SortFilterProxy( AbstractModel *belowModel, QObject *parent )
    : ProxyBase( belowModel, parent )
{
    // Tell QSortFilterProxyModel: keep the filter correct when the underlying source model changes.
    // Qt will do this by receiving the standard QAbstractItemModel signals: dataChanged, rowsInserted, etc.
    setDynamicSortFilter( true );

    // Tell QSortFilterProxyModel: activate sorting.
    sort( 0 );    // 0 is a dummy column.

    KConfigGroup config = Amarok::config("Playlist Search");
    m_showOnlyMatches = config.readEntry( "ShowOnlyMatches", true );
}

SortFilterProxy::~SortFilterProxy()
{
}


//! Sort-related functions

bool
SortFilterProxy::lessThan( const QModelIndex & sourceModelIndexA, const QModelIndex & sourceModelIndexB ) const
{
    int rowA = sourceModelIndexA.row();
    int rowB = sourceModelIndexB.row();
    return m_mlt( sourceModel(), rowA, rowB );
}

bool
SortFilterProxy::isSorted()
{
    return m_scheme.length() > 0;
}

void
SortFilterProxy::updateSortMap( SortScheme scheme )
{
    m_scheme = scheme;
    m_mlt.setSortScheme( m_scheme );

    invalidate();    // Tell QSortFilterProxyModel: re-sort
}


//! Filter-related functions

void
SortFilterProxy::clearSearchTerm()
{
    find( QString(), 0 );
    ProxyBase::clearSearchTerm();
}

void
SortFilterProxy::filterUpdated()
{
    if ( m_showOnlyMatches )
        invalidateFilter();    // Tell QSortFilterProxyModel: re-filter
    //else
    //  Search criteria are not being used for filtering, so we can ignore the update
}

int
SortFilterProxy::find( const QString &searchTerm, int searchFields )
{
    m_currentSearchTerm = searchTerm;
    m_currentSearchFields = searchFields;

    // Don't call 'filterUpdated()': our client must do that as part of the API.
    // This allows client 'PrettyListView' to give the user the time to type a few
    // characters before we do a filter run that might block for a few seconds.

    return -1;
}

void
SortFilterProxy::showOnlyMatches( bool onlyMatches )
{
    m_showOnlyMatches = onlyMatches;

    //make sure to update model when mode changes ( as we might have ignored any
    //number of changes to the search term )
    invalidateFilter();    // Tell QSortFilterProxyModel: re-filter.
}

bool
SortFilterProxy::filterAcceptsRow( int sourceModelRow, const QModelIndex &sourceModelParent ) const
{
    Q_UNUSED( sourceModelParent );

    if ( m_showOnlyMatches )
    {
        if ( m_currentSearchTerm.isEmpty() )
            return true;
        else
            return rowMatch( sourceModelRow, m_currentSearchTerm, m_currentSearchFields );
    } else
        return true;
}


}    //namespace Playlist

#include "SortFilterProxy.moc"
