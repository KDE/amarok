/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#include "FilterProxy.h"

#include "Amarok.h"
#include "amarokconfig.h"

namespace Playlist {

FilterProxy::FilterProxy( AbstractModel *belowModel, QObject *parent )
    : ProxyBase( parent )
{
    m_belowModel = belowModel;
    setSourceModel( dynamic_cast< QAbstractItemModel * >( m_belowModel ) );

    connect( sourceModel(), SIGNAL( insertedIds( const QList<quint64>& ) ), this, SLOT( slotInsertedIds( const QList<quint64>& ) ) );
    connect( sourceModel(), SIGNAL( beginRemoveIds() ), this, SIGNAL( beginRemoveIds() ) );
    connect( sourceModel(), SIGNAL( removedIds( const QList<quint64>& ) ), this, SLOT( slotRemovedIds( const QList<quint64>& ) ) );
    connect( sourceModel(), SIGNAL( activeTrackChanged( const quint64 ) ), this, SIGNAL( activeTrackChanged( quint64 ) ) );
    connect( sourceModel(), SIGNAL( metadataUpdated() ), this, SIGNAL( metadataUpdated() ) );
    connect( sourceModel(), SIGNAL( queueChanged() ), this, SIGNAL( queueChanged() ) );
    connect( this, SIGNAL( metadataUpdated() ), this, SLOT( slotInvalidateFilter() ) );

    KConfigGroup config = Amarok::config("Playlist Search");
    m_passThrough = !config.readEntry( "ShowOnlyMatches", true );
    setDynamicSortFilter( true );
}

FilterProxy::~FilterProxy()
{
}

bool FilterProxy::filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const
{
    Q_UNUSED( source_parent );

    if ( m_passThrough )
        return true;

    const bool match = matchesCurrentSearchTerm( source_row );
    return match;
}

int
FilterProxy::rowCount(const QModelIndex& parent) const
{
    return QSortFilterProxyModel::rowCount( parent );
}

void FilterProxy::filterUpdated()
{
    if ( !m_passThrough )
    {
        invalidateFilter();
        emit( layoutChanged() );
    }
}

int
FilterProxy::find( const QString &searchTerm, int searchFields )
{
    m_currentSearchTerm = searchTerm;
    m_currentSearchFields = searchFields;
    if( !m_passThrough )
    {
        filterUpdated();
    }
    return -1;
}

void FilterProxy::slotInsertedIds( const QList< quint64 > &ids )
{
    QList< quint64 > proxyIds;
    foreach( quint64 id, ids )
    {
        if ( matchesCurrentSearchTerm( m_belowModel->rowForId( id ) ) )
            proxyIds << id;
    }

    if ( proxyIds.size() > 0 )
        emit( insertedIds( proxyIds ) );
}

void FilterProxy::slotRemovedIds( const QList< quint64 > &ids )
{
    QList<quint64> proxyIds;
    foreach( quint64 id, ids ) {
        const int row = m_belowModel->rowForId( id );
        if ( row == -1 || matchesCurrentSearchTerm( row ) ) {
            proxyIds << id;
        }
    }

    if ( proxyIds.size() > 0 )
        emit removedIds( proxyIds );
}

void
FilterProxy::showOnlyMatches( bool onlyMatches )
{
    m_passThrough = !onlyMatches;

    //make sure to update model when mode changes ( as we might have ignored and
    //number of changes to the search term )
    invalidateFilter();
    emit( layoutChanged() );
}

void FilterProxy::clearSearchTerm()
{
    m_currentSearchTerm.clear();
    m_currentSearchFields = 0;
    m_belowModel->clearSearchTerm();
    if ( !m_passThrough )
    {
        invalidateFilter();
        emit( layoutChanged() );
    }
}

bool
FilterProxy::matchesCurrentSearchTerm( int source_row ) const
{
    if ( m_belowModel->rowExists( source_row ) )
    {
        if ( m_currentSearchTerm.isEmpty() )
            return true;
        return rowMatch( source_row, m_currentSearchTerm, m_currentSearchFields );
    }
    return false;
}

int
FilterProxy::rowToSource( int row ) const
{
    QModelIndex index = this->index( row, 0 );
    QModelIndex sourceIndex = mapToSource( index );

    if ( !sourceIndex.isValid() )
        return ( row == rowCount() ) ? m_belowModel->rowCount() : -1;
    return sourceIndex.row();
}

int
FilterProxy::rowFromSource( int row ) const
{
    QModelIndex sourceIndex = sourceModel()->index( row, 0 );
    QModelIndex index = mapFromSource( sourceIndex );

    if ( !index.isValid() )
        return -1;
    return index.row();
}

}

#include "FilterProxy.moc"
