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
 
#include "NavigatorFilterProxyModel.h"

#include "Debug.h"
#include "playlist/PlaylistModel.h"

namespace Playlist {

NavigatorFilterProxyModel* NavigatorFilterProxyModel::s_instance = 0;

NavigatorFilterProxyModel * NavigatorFilterProxyModel::instance()
{
    if ( s_instance == 0 )
        s_instance = new NavigatorFilterProxyModel();
    return s_instance;
}

NavigatorFilterProxyModel::NavigatorFilterProxyModel()
    : QSortFilterProxyModel( Model::instance() )
{
    setSourceModel( Model::instance() );

    connect( Model::instance(), SIGNAL( insertedIds( const QList<quint64>& ) ), this, SLOT( slotInsertedIds( const QList<quint64>& ) ) );
    connect( Model::instance(), SIGNAL( removedIds( const QList<quint64>& ) ), this, SLOT( slotRemovedIds( const QList<quint64>& ) ) );

    KConfigGroup config = Amarok::config("Playlist Search");
    m_passThrough = !config.readEntry( "PlayOnlyMatches", true );

}

NavigatorFilterProxyModel::~NavigatorFilterProxyModel()
{
}

bool Playlist::NavigatorFilterProxyModel::filterAcceptsRow( int row, const QModelIndex & source_parent ) const
{
    Q_UNUSED( source_parent );

    if ( m_passThrough )
        return true;
    
    bool match = Model::instance()->matchesCurrentSearchTerm( row );
    return match;
}

int Playlist::NavigatorFilterProxyModel::activeRow() const
{

    //we map the active row form the source to this model. if The active row is not in the items exposed by this proxy, just point to out first item.
    Model * model = Model::instance();

    QModelIndex sourceIndex = model->index( model->activeRow(), 0 );
    QModelIndex index = mapFromSource( sourceIndex );

    if ( !index.isValid() )
        return -1;
    else
        return index.row();
    
}

quint64 Playlist::NavigatorFilterProxyModel::idAt( const int row ) const
{
    QModelIndex index = this->index( row, 0 );
    QModelIndex sourceIndex = mapToSource( index );
    return Model::instance()->idAt( sourceIndex.row() );
}

void Playlist::NavigatorFilterProxyModel::filterUpdated()
{
    if ( !m_passThrough ) {
        invalidateFilter();
        emit( filterChanged() );
    }
}

int Playlist::NavigatorFilterProxyModel::firstMatchAfterActive()
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
    
    //convert to proxy row:
    QModelIndex sourceIndex = model->index( matchRow, 0 );
    QModelIndex index = mapFromSource( sourceIndex );

    return index.row();
}

int Playlist::NavigatorFilterProxyModel::firstMatchBeforeActive()
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
    
    //convert to proxy row:
    QModelIndex sourceIndex = model->index( matchRow, 0 );
    QModelIndex index = mapFromSource( sourceIndex );

    return index.row();
}

void Playlist::NavigatorFilterProxyModel::slotInsertedIds( const QList< quint64 > &ids )
{
    Model * model = Model::instance();

    QList< quint64 > proxyIds;
    foreach( quint64 id, ids ) {
        if ( model->matchesCurrentSearchTerm( model->rowForId( id ) ) ) {
            proxyIds << id;
        }
    }

    if ( proxyIds.size() > 0 )
        emit( insertedIds( proxyIds ) );
}

void Playlist::NavigatorFilterProxyModel::slotRemovedIds( const QList< quint64 > &ids )
{
    Model * model = Model::instance();

    QList< quint64 > proxyIds;
    foreach( quint64 id, ids ) {
        const int row = model->rowForId( id );
        if ( row == -1 || model->matchesCurrentSearchTerm( row ) ) {
            proxyIds << id;
        }
    }

    if ( proxyIds.size() > 0 )
        emit( removedIds( proxyIds ) );
}


Item::State Playlist::NavigatorFilterProxyModel::stateOfRow( int row ) const
{
    //map to sourceRow:
    QModelIndex index = this->index( row, 0 );
    QModelIndex sourceIndex = mapToSource( index );
    
    return Model::instance()->stateOfRow( sourceIndex.row() );
}

Item::State Playlist::NavigatorFilterProxyModel::stateOfId( quint64 id ) const
{
    return Model::instance()->stateOfId( id );
}


void Playlist::NavigatorFilterProxyModel::setPassThrough( bool passThrough )
{
    m_passThrough = passThrough;

    //make sure to update model when mode changes ( as we might have ignored and
    //number of changes to the search therm )
    invalidateFilter();
    emit( filterChanged() );
}

}

#include "NavigatorFilterProxyModel.moc"







