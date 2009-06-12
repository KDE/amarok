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
    : QAbstractProxyModel()
    , m_belowModel( FilterProxy::instance() )
{
    DEBUG_BLOCK
    debug() << "Instantiating SortProxy";
    setSourceModel( m_belowModel );
    m_map = new SortMap( m_belowModel );

    //As this Proxy doesn't add or remove tracks, and unique track IDs must be left untouched
    //by sorting, they may be just blindly forwarded
    connect( m_belowModel, SIGNAL( insertedIds( const QList<quint64>& ) ), this, SIGNAL( insertedIds( const QList< quint64>& ) ) );
    connect( m_belowModel, SIGNAL( removedIds( const QList<quint64>& ) ), this, SIGNAL( removedIds( const QList< quint64 >& ) ) );

    connect( m_belowModel, SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( onDataChanged( const QModelIndex&, const QModelIndex& ) ) );
    connect( m_belowModel, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( onRowsInserted( const QModelIndex &, int, int ) ) );
    connect( m_belowModel, SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), this, SLOT( onRowsRemoved( const QModelIndex&, int, int ) ) );
    // ^ rowsRemoved is currently used as rowsRemoved( start, start ), one item at a time

    connect( m_belowModel, SIGNAL( layoutChanged() ), this, SIGNAL( layoutChanged() ) );
    connect( m_belowModel, SIGNAL( filterChanged() ), this, SIGNAL( filterChanged() ) );    //FIXME: probably nobody connects to this
    connect( m_belowModel, SIGNAL( modelReset() ), this, SIGNAL( modelReset() ) );

    //NOTE to self by Téo: when rows are inserted, and that I'll know thanks to the signals
    // in FilterProxy, they must be added to the m_map but the map must
    // be declared invalid m_sorted = 0;

    //NOTE to self by Téo: dataChanged could already be implemented by QAbstractProxyModel
    /*
    needed by GroupingProxy:
    connect( m_belowModel, SIGNAL( modelReset() ), this, SLOT( regroupAll() ) );
    */
}

SortProxy::~SortProxy()
{
    delete m_map;
}

QModelIndex
SortProxy::index( int row, int column, const QModelIndex &parent ) const
{
    Q_UNUSED( parent );
    if ( m_belowModel->rowExists( m_map->inv( row ) ) )
    {
        //debug() << "the row exists!";
        return createIndex( row, column );
    }
    debug() << "bad model, no row for you! rowInProxy=" << row;
    return QModelIndex();
}

QModelIndex
SortProxy::parent( const QModelIndex& index ) const
{
    return m_belowModel->parent( index );
}

QModelIndex
SortProxy::mapFromSource( const QModelIndex& sourceIndex ) const
{
    debug() << "mapFromSource row=" << sourceIndex.row();
    return createIndex( m_map->map( sourceIndex.row() ), sourceIndex.column() );
}

QModelIndex
SortProxy::mapToSource( const QModelIndex& proxyIndex ) const
{
    //debug() << "mapToSource row=" << proxyIndex.row();
    return m_belowModel->index( m_map->inv( proxyIndex.row() ), proxyIndex.column() );
}

void
SortProxy::updateSortMap( SortScheme *scheme)
{
    //APPLY THE SORTING
    m_map->sort( scheme );
    emit sortChanged();
}

void
SortProxy::onDataChanged( const QModelIndex& start, const QModelIndex& end )
{
    emit dataChanged( mapFromSource( start ), mapFromSource( end ) );   //see on RowsRemoved
}

void
SortProxy::onRowsInserted( const QModelIndex& idx, int start, int end )
{
    m_map->insertRows( start, end );
    emit rowsInserted( mapFromSource( idx ), rowFromSource( start ), rowFromSource( end ) );    //see onRowsRemoved
}

void
SortProxy::onRowsRemoved( const QModelIndex& idx, int start, int end )
{
    m_map->deleteRows( start, end );
    emit rowsRemoved( mapFromSource( idx ), rowFromSource( start ), rowFromSource( end ) );
    //this is NOT GOOD because the removed tracks might not be a contiguous list in a sorted model
}

// Pass-through methods, basically identical to those in Playlist::FilterProxy, that pretty
// much just forward stuff through the stack of proxies start here.
// Please keep them sorted alphabetically.  - Téo

int
SortProxy::activeRow() const
{
    // We map the active row form the source to this model.
    // As in FilterProxy: return rowFromSource( m_model->activeRow() );
    return rowFromSource( m_belowModel->activeRow() );
}

void
SortProxy::clearSearchTerm()
{
    m_belowModel->clearSearchTerm();
}

int
SortProxy::columnCount(const QModelIndex& parent) const
{
    return m_belowModel->columnCount( parent );
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
    //note to self by Téo: is this still needed?
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
    return rowFromSource( m_belowModel->find( searchTerm, searchFields ) );  //DONE: see SortProxy::activeRow()
}

int
SortProxy::findNext( const QString &searchTerm, int selectedRow, int searchFields )
{
    return rowFromSource( m_belowModel->findNext( searchTerm, selectedRow, searchFields ) );  //DONE: see SortProxy::activeRow()
}

int
SortProxy::findPrevious( const QString &searchTerm, int selectedRow, int searchFields )
{
    return rowFromSource( m_belowModel->findPrevious( searchTerm, selectedRow, searchFields ) );  //DONE: see SortProxy::activeRow()
}

Qt::ItemFlags
SortProxy::flags( const QModelIndex &index ) const
{
    return m_belowModel->flags( mapToSource( index ) );
}

QMimeData *
SortProxy::mimeData( const QModelIndexList &index ) const
{
    return m_belowModel->mimeData( index );     //TODO: probably needs mapToSource!!!
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
SortProxy::rowCount(const QModelIndex& parent) const
{
    return m_belowModel->rowCount( parent );
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
    m_belowModel->setActiveRow( rowToSource( row ) );  //DONE: see SortProxy::activeRow()
}

Qt::DropActions
SortProxy::supportedDropActions() const
{
    return m_belowModel->supportedDropActions();
}

int
SortProxy::totalLength() const
{
    return m_belowModel->totalLength();     //this should not need changes by definition
}

Meta::TrackPtr
SortProxy::trackAt(int row) const
{
    return m_belowModel->trackAt( rowToSource( row ) );    //DONE: see SortProxy::activeRow()
}

}   //namespace Playlist
