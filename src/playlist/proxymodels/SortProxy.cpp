/***************************************************************************
 *   Copyright © 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>       *
 *             © 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                *
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

#include "SortProxy.h"

#include "Debug.h"
#include "playlist/PlaylistModel.h"
#include "FilterProxy.h"

namespace Playlist
{
//Téo 7/5/2009: Attention coding style police guys: this is very WiP and if you see notes
//to self and debug spam here pretty please let it be until I remove it.

SortProxy* SortProxy::s_instance = 0;

SortProxy*
SortProxy::instance()
{
    if ( s_instance == 0 )
        s_instance = new SortProxy();
    return s_instance;
}

SortProxy::SortProxy()
    : QSortFilterProxyModel()
    , m_belowModel(  FilterProxy::instance() )
{
    setSourceModel( m_belowModel );
    //m_sortScheme << foo...
}

SortProxy::~SortProxy()
{
    delete m_currentSortScheme;
}

bool SortProxy::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    //return QSortFilterProxyModel::lessThan(left, right);
}

void SortProxy::sort( const SortScheme &scheme )
{

}


// PASS-THROUGH METHODS THAT PRETTY MUCH JUST FORWARD STUFF THROUGH THE STACK OF PROXIES START HERE
// Please keep them sorted alphabetically.

int
SortProxy::activeRow() const
{
    // we map the active row form the source to this model. if the active row is not in the items
    // exposed by this proxy, just point to our first item.
    //return rowFromSource( m_model->activeRow() ); //from FIlterProxy
    return m_belowModel->activeRow();   //TODO: this will need to be adjusted when this proxy starts doing some actual permutation
}

void
SortProxy::clearSearchTerm()
{
    m_belowModel->clearSearchTerm();
}

int
SortProxy::currentSearchFields()
{
    return m_belowModel->currentSearchFields();
}

QString
SortProxy::currentSearchTerm()
{
    return m_belowModel->currentSearchTerm();
}

QVariant
SortProxy::data( const QModelIndex & index, int role ) const
{
    //HACK around incomplete index causing a crash...
    //note to self by Téo: no idea what this does but I seem to need it
    QModelIndex newIndex = this->index( index.row(), index.column() );
    
    QModelIndex sourceIndex = mapToSource( newIndex );
    return m_belowModel->data( sourceIndex, role );
}

bool
SortProxy::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
    return m_belowModel->dropMimeData( data, action, row, column, parent );
}

int
SortProxy::find( const QString &searchTerm, int searchFields )
{
    return m_belowModel->find( searchTerm, searchFields );  //TODO: see SortProxy::activeRow()
}

int
SortProxy::findNext( const QString &searchTerm, int selectedRow, int searchFields )
{
    return m_belowModel->findNext( searchTerm, selectedRow, searchFields );  //TODO: see SortProxy::activeRow()
}

int
SortProxy::findPrevious( const QString &searchTerm, int selectedRow, int searchFields )
{
    return m_belowModel->findPrevious( searchTerm, selectedRow, searchFields );  //TODO: see SortProxy::activeRow()
}

Qt::ItemFlags
SortProxy::flags( const QModelIndex &index ) const
{
    return m_belowModel->flags( index );
}

QMimeData *
SortProxy::mimeData( const QModelIndexList &index ) const
{
    return m_belowModel->mimeData( index );
}

QStringList
SortProxy::mimeTypes() const
{
    return m_belowModel->mimeTypes();
}

bool
SortProxy::rowExists( int row ) const
{
    QModelIndex index = this->index( row, 0 );
    return index.isValid();
}

int
SortProxy::rowFromSource( int row ) const
{
    QModelIndex sourceIndex = m_belowModel->index( row, 0 );
    QModelIndex index = mapFromSource( sourceIndex );
    
    if ( !index.isValid() )
        return -1;
    return index.row();
}

int
SortProxy::rowToSource( int row ) const
{
    QModelIndex index = this->index( row, 0 );
    QModelIndex sourceIndex = mapToSource( index );
    
    if ( !sourceIndex.isValid() )
        return -1;
    return sourceIndex.row();
}

void
SortProxy::setActiveRow( int row )
{
    m_belowModel->setActiveRow( row );  //TODO: see SortProxy::activeRow()
}

Qt::DropActions
SortProxy::supportedDropActions() const
{
    return m_belowModel->supportedDropActions();
}

int
SortProxy::totalLength() const
{
    return m_belowModel->totalLength();     //this might not need changes
}

Meta::TrackPtr
SortProxy::trackAt(int row) const
{
    return m_belowModel->trackAt( row );    //TODO: see SortProxy::activeRow()
}

}   //namespace Playlist