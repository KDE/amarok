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
#include "playlist/PlaylistModel.h"

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
{
    setSourceModel( Model::instance() );

    connect( Model::instance(), SIGNAL( insertedIds( const QList<quint64>& ) ), this, SLOT( slotInsertedIds( const QList<quint64>& ) ) );
    connect( Model::instance(), SIGNAL( removedIds( const QList<quint64>& ) ), this, SLOT( slotRemovedIds( const QList<quint64>& ) ) );

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

int FilterProxy::activeRow() const
{
    // we map the active row form the source to this model. if the active row is not in the items
    // exposed by this proxy, just point to our first item.
    Model * model = Model::instance();
    return rowFromSource( model->activeRow() );
}

quint64 FilterProxy::idAt( const int row ) const
{
    QModelIndex index = this->index( row, 0 );
    QModelIndex sourceIndex = mapToSource( index );
    return Model::instance()->idAt( sourceIndex.row() );
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
    Model * model = Model::instance();
    int activeSourceRow = model->activeRow();

    if ( m_passThrough )
        return activeSourceRow + 1;

    int matchRow = -1;
    int nextRow = activeSourceRow + 1;
    while ( model->rowExists( nextRow ) )
    {
        if ( model->matchesCurrentSearchTerm( nextRow ) ) {
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
    Model * model = Model::instance();
    int activeSourceRow = model->activeRow();

    if ( m_passThrough )
        return activeSourceRow - 1;

    int matchRow = -1;
    int previousRow = activeSourceRow - 1;
    while ( model->rowExists( previousRow ) )
    {
        if ( model->matchesCurrentSearchTerm( previousRow ) ) {
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
    Model * model = Model::instance();

    QList< quint64 > proxyIds;
    foreach( quint64 id, ids )
    {
        if ( model->matchesCurrentSearchTerm( model->rowForId( id ) ) )
            proxyIds << id;
    }

    if ( proxyIds.size() > 0 )
        emit( insertedIds( proxyIds ) );
}

void FilterProxy::slotRemovedIds( const QList< quint64 > &ids )
{
    Model *model = Model::instance();

    QList<quint64> proxyIds;
    foreach( quint64 id, ids ) {
        const int row = model->rowForId( id );
        if ( row == -1 || model->matchesCurrentSearchTerm( row ) ) {
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
    Model * model = Model::instance();
    QModelIndex sourceIndex = model->index( row, 0 );
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
    Model::instance()->setActiveRow( rowToSource( row ) );
}

Meta::TrackPtr FilterProxy::trackAt(int row) const
{
    return Model::instance()->trackAt( rowToSource( row ) );
}

int FilterProxy::find( const QString &searchTerm, int searchFields )
{
    return rowFromSource( Model::instance()->find( searchTerm, searchFields ) );
}

int FilterProxy::findNext( const QString & searchTerm, int selectedRow, int searchFields )
{
    return rowFromSource( Model::instance()->findNext( searchTerm, selectedRow, searchFields ) );
}

int FilterProxy::findPrevious( const QString & searchTerm, int selectedRow, int searchFields )
{
    return rowFromSource( Model::instance()->findPrevious( searchTerm, selectedRow, searchFields ) );
}

int FilterProxy::totalLength() const
{
    return Model::instance()->totalLength();
}

void FilterProxy::clearSearchTerm()
{
    Model::instance()->clearSearchTerm();
    
    if ( !m_passThrough )
    {
        invalidateFilter();
        emit( filterChanged() );
        emit( layoutChanged() );
    }
}

QString FilterProxy::currentSearchTerm()
{
    return Model::instance()->currentSearchTerm();
}

int FilterProxy::currentSearchFields()
{
    return Model::instance()->currentSearchFields();
}

QVariant FilterProxy::data( const QModelIndex & index, int role ) const
{
     //HACK around incomplete index causing a crash...
    QModelIndex newIndex = this->index( index.row(), index.column() );

    QModelIndex sourceIndex = mapToSource( newIndex );
    return Model::instance()->data( sourceIndex, role );
}


Qt::DropActions FilterProxy::supportedDropActions() const
{
    return Model::instance()->supportedDropActions();
}

Qt::ItemFlags FilterProxy::flags( const QModelIndex &index ) const
{
    return Model::instance()->flags( index );
}

QStringList FilterProxy::mimeTypes() const
{
    return Model::instance()->mimeTypes();
}

QMimeData * FilterProxy::mimeData( const QModelIndexList &index ) const
{
    return Model::instance()->mimeData( index );
}

bool FilterProxy::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
    return Model::instance()->dropMimeData( data, action, row, column, parent );
}

void FilterProxy::setRowQueued( int row )
{
    Model::instance()->setRowQueued( rowToSource( row ) );
}

void FilterProxy::setRowDequeued( int row )
{
    Model::instance()->setRowDequeued( rowToSource( row ) );
}

}

#include "FilterProxy.moc"
