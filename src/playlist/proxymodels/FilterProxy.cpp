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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "FilterProxy.h"

#include "Debug.h"

namespace Playlist {

FilterProxy* FilterProxy::s_instance = 0;

FilterProxy* FilterProxy::instance()
{
    if ( s_instance == 0 )
        s_instance = new FilterProxy();
    return s_instance;
}

FilterProxy::FilterProxy()
    : ProxyBase( Model::instance() )
{
    m_belowModel = Model::instance();
    setSourceModel( dynamic_cast< Model * >( m_belowModel ) );

    connect( sourceModel(), SIGNAL( insertedIds( const QList<quint64>& ) ), this, SLOT( slotInsertedIds( const QList<quint64>& ) ) );
    connect( sourceModel(), SIGNAL( removedIds( const QList<quint64>& ) ), this, SLOT( slotRemovedIds( const QList<quint64>& ) ) );

    KConfigGroup config = Amarok::config("Playlist Search");
    m_passThrough = !config.readEntry( "ShowOnlyMatches", true );

    //setDynamicSortFilter( true );
}

FilterProxy::~FilterProxy()
{
}

bool FilterProxy::filterAcceptsRow( int row, const QModelIndex & source_parent ) const
{
    Q_UNUSED( source_parent );

    if ( m_passThrough )
        return true;

    const bool match = Model::instance()->matchesCurrentSearchTerm( row );
    return match;
}

quint64 FilterProxy::idAt( const int row ) const
{
    QModelIndex index = this->index( row, 0 );
    QModelIndex sourceIndex = mapToSource( index );
    return Model::instance()->idAt( sourceIndex.row() );
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

int FilterProxy::firstMatchAfterActive()
{
    int activeSourceRow = m_belowModel->activeRow();

    if ( m_passThrough )
        return activeSourceRow + 1;

    int matchRow = -1;
    int nextRow = activeSourceRow + 1;
    while ( m_belowModel->rowExists( nextRow ) )
    {
        if ( Model::instance()->matchesCurrentSearchTerm( nextRow ) ) {
            matchRow = nextRow;
            break;
        }

        nextRow++;
    }

    if ( matchRow == -1 )
        return -1;

    return rowFromSource( matchRow );
}

int FilterProxy::firstMatchBeforeActive()
{
    int activeSourceRow = m_belowModel->activeRow();

    if ( m_passThrough )
        return activeSourceRow - 1;

    int matchRow = -1;
    int previousRow = activeSourceRow - 1;
    while ( m_belowModel->rowExists( previousRow ) )
    {
        if ( Model::instance()->matchesCurrentSearchTerm( previousRow ) ) {
            matchRow = previousRow;
            break;
        }

        previousRow--;
    }

    if ( matchRow == -1 )
        return -1;

    return rowFromSource( matchRow );
}

void FilterProxy::slotInsertedIds( const QList< quint64 > &ids )
{
    QList< quint64 > proxyIds;
    foreach( quint64 id, ids )
    {
        if ( Model::instance()->matchesCurrentSearchTerm( Model::instance()->rowForId( id ) ) )
            proxyIds << id;
    }

    if ( proxyIds.size() > 0 )
        emit( insertedIds( proxyIds ) );
}

void FilterProxy::slotRemovedIds( const QList< quint64 > &ids )
{
    QList<quint64> proxyIds;
    foreach( quint64 id, ids ) {
        const int row = Model::instance()->rowForId( id );
        if ( row == -1 || Model::instance()->matchesCurrentSearchTerm( row ) ) {
            proxyIds << id;
        }
    }

    if ( proxyIds.size() > 0 )
        emit removedIds( proxyIds );
}

Item::State FilterProxy::stateOfRow( int row ) const
{
    return Model::instance()->stateOfRow( rowToSource( row ) );
}

Item::State FilterProxy::stateOfId( quint64 id ) const
{
    return Model::instance()->stateOfId( id );
}

void FilterProxy::setPassThrough( bool passThrough )
{
    m_passThrough = passThrough;

    //make sure to update model when mode changes ( as we might have ignored and
    //number of changes to the search term )
    invalidateFilter();
    emit( layoutChanged() );
}

int FilterProxy::rowToSource( int row ) const
{
    QModelIndex index = this->index( row, 0 );
    QModelIndex sourceIndex = mapToSource( index );

    if ( !sourceIndex.isValid() )
        return -1;
    return sourceIndex.row();
}

int FilterProxy::rowFromSource( int row ) const
{
    QModelIndex sourceIndex = sourceModel()->index( row, 0 );
    QModelIndex index = mapFromSource( sourceIndex );

    if ( !index.isValid() )
        return -1;
    return index.row();
}

void FilterProxy::clearSearchTerm()
{
    m_belowModel->clearSearchTerm();

    if ( !m_passThrough )
    {
        invalidateFilter();
        emit( layoutChanged() );
    }
}

}

#include "FilterProxy.moc"
