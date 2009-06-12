/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

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
    : QSortFilterProxyModel( Model::instance() )
    , m_model( Model::instance() )
{
    setSourceModel( m_model );

    connect( m_model, SIGNAL( insertedIds( const QList<quint64>& ) ), this, SLOT( slotInsertedIds( const QList<quint64>& ) ) );
    connect( m_model, SIGNAL( removedIds( const QList<quint64>& ) ), this, SLOT( slotRemovedIds( const QList<quint64>& ) ) );

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

    const bool match = m_model->matchesCurrentSearchTerm( row );
    return match;
}

int FilterProxy::activeRow() const
{
    // we map the active row form the source to this model. if the active row is not in the items
    // exposed by this proxy, just point to our first item.
    return rowFromSource( m_model->activeRow() );
}

quint64 FilterProxy::idAt( const int row ) const
{
    QModelIndex index = this->index( row, 0 );
    QModelIndex sourceIndex = mapToSource( index );
    return m_model->idAt( sourceIndex.row() );
}

void FilterProxy::filterUpdated()
{
    if ( !m_passThrough )
    {
        invalidateFilter();
        emit( filterChanged() );
        emit( layoutChanged() );
    }
}

int FilterProxy::firstMatchAfterActive()
{
    int activeSourceRow = m_model->activeRow();

    if ( m_passThrough )
        return activeSourceRow + 1;

    int matchRow = -1;
    int nextRow = activeSourceRow + 1;
    while ( m_model->rowExists( nextRow ) )
    {
        if ( m_model->matchesCurrentSearchTerm( nextRow ) ) {
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
    int activeSourceRow = m_model->activeRow();

    if ( m_passThrough )
        return activeSourceRow - 1;

    int matchRow = -1;
    int previousRow = activeSourceRow - 1;
    while ( m_model->rowExists( previousRow ) )
    {
        if ( m_model->matchesCurrentSearchTerm( previousRow ) ) {
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
        if ( m_model->matchesCurrentSearchTerm( m_model->rowForId( id ) ) )
            proxyIds << id;
    }

    if ( proxyIds.size() > 0 )
        emit( insertedIds( proxyIds ) );
}

void FilterProxy::slotRemovedIds( const QList< quint64 > &ids )
{
    QList<quint64> proxyIds;
    foreach( quint64 id, ids ) {
        const int row = m_model->rowForId( id );
        if ( row == -1 || m_model->matchesCurrentSearchTerm( row ) ) {
            proxyIds << id;
        }
    }

    if ( proxyIds.size() > 0 )
        emit removedIds( proxyIds );
}

Item::State FilterProxy::stateOfRow( int row ) const
{
    return m_model->stateOfRow( rowToSource( row ) );
}

Item::State FilterProxy::stateOfId( quint64 id ) const
{
    return m_model->stateOfId( id );
}

void FilterProxy::setPassThrough( bool passThrough )
{
    m_passThrough = passThrough;

    //make sure to update model when mode changes ( as we might have ignored and
    //number of changes to the search term )
    invalidateFilter();
    emit( filterChanged() );
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
    QModelIndex sourceIndex = m_model->index( row, 0 );
    QModelIndex index = mapFromSource( sourceIndex );

    if ( !index.isValid() )
        return -1;
    return index.row();
}

bool FilterProxy::rowExists( int row ) const
{

    QModelIndex index = this->index( row, 0 );
    return index.isValid();
}

void FilterProxy::setActiveRow( int row )
{
    m_model->setActiveRow( rowToSource( row ) );
}

Meta::TrackPtr FilterProxy::trackAt(int row) const
{
    return m_model->trackAt( rowToSource( row ) );
}

int FilterProxy::find( const QString &searchTerm, int searchFields )
{
    return rowFromSource( m_model->find( searchTerm, searchFields ) );
}

int FilterProxy::findNext( const QString & searchTerm, int selectedRow, int searchFields )
{
    return rowFromSource( m_model->findNext( searchTerm, selectedRow, searchFields ) );
}

int FilterProxy::findPrevious( const QString & searchTerm, int selectedRow, int searchFields )
{
    return rowFromSource( m_model->findPrevious( searchTerm, selectedRow, searchFields ) );
}

int FilterProxy::totalLength() const
{
    return m_model->totalLength();
}

void FilterProxy::clearSearchTerm()
{
    m_model->clearSearchTerm();
    
    if ( !m_passThrough )
    {
        invalidateFilter();
        emit( filterChanged() );
        emit( layoutChanged() );
    }
}

QString FilterProxy::currentSearchTerm()
{
    return m_model->currentSearchTerm();
}

int FilterProxy::currentSearchFields()
{
    return m_model->currentSearchFields();
}

QVariant FilterProxy::data( const QModelIndex & index, int role ) const
{
     //HACK around incomplete index causing a crash...
    QModelIndex newIndex = this->index( index.row(), index.column() );

    QModelIndex sourceIndex = mapToSource( newIndex );
    return m_model->data( sourceIndex, role );
}


Qt::DropActions FilterProxy::supportedDropActions() const
{
    return m_model->supportedDropActions();
}

Qt::ItemFlags FilterProxy::flags( const QModelIndex &index ) const
{
    return m_model->flags( index );
}

QStringList FilterProxy::mimeTypes() const
{
    return m_model->mimeTypes();
}

QMimeData * FilterProxy::mimeData( const QModelIndexList &index ) const
{
    return m_model->mimeData( index );
}

bool FilterProxy::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
    return m_model->dropMimeData( data, action, row, column, parent );
}

void FilterProxy::setRowQueued( int row )
{
    m_model->setRowQueued( rowToSource( row ) );
}

void FilterProxy::setRowDequeued( int row )
{
    m_model->setRowDequeued( rowToSource( row ) );
}

}

#include "FilterProxy.moc"
