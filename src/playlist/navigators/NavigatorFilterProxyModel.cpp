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
}

NavigatorFilterProxyModel::~NavigatorFilterProxyModel()
{
}

bool Playlist::NavigatorFilterProxyModel::filterAcceptsRow( int row, const QModelIndex & source_parent ) const
{
    DEBUG_BLOCK
    Q_UNUSED( source_parent );

    bool match = Model::instance()->matchesCurrentSearchTerm( row );
    debug() << "Match for row " << row << ": " << match;
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

    DEBUG_BLOCK

    QModelIndex index = this->index( row, 0 );
    QModelIndex sourceIndex = mapToSource( index );

    debug() << "proxy row " << row << " = source row " << sourceIndex.row();
    
    return Model::instance()->idAt( sourceIndex.row() );
}

void Playlist::NavigatorFilterProxyModel::filterUpdated()
{
    invalidateFilter();
}

}

int Playlist::NavigatorFilterProxyModel::firstMatchAfterActive()
{
    Model * model = Model::instance();
    int activeSourceRow = model->activeRow();

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







