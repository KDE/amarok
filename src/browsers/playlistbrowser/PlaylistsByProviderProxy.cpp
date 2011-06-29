/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "PlaylistsByProviderProxy.h"

#include "AmarokMimeData.h"
#include "UserPlaylistModel.h"

#include "core/support/Debug.h"

#include <QStack>

const QString PlaylistsByProviderProxy::AMAROK_PROVIDERPROXY_INDEXES =
        "application/x-amarok-providerproxy-indexes";

PlaylistsByProviderProxy::PlaylistsByProviderProxy( QAbstractItemModel *model, int column )
        : QtGroupingProxy( model, QModelIndex(), column )
{
    connect( sourceModel(), SIGNAL(renameIndex( const QModelIndex & )),
             SLOT(slotRenameIndex( const QModelIndex & )) );
}

QVariant
PlaylistsByProviderProxy::data( const QModelIndex &idx, int role ) const
{
    //TODO: filter out actions not from the provider, possibly using QAction separators marking
    // the source of the actions (makes sense in the UI as well.

    //Turn the QVariantList of the source into a comma separated string, but only for the real items
    if( !isGroup( idx ) && idx.column() == PlaylistBrowserNS::PlaylistBrowserModel::ProviderColumn
        && role == Qt::DisplayRole )
    {
        QVariant indexData = QtGroupingProxy::data( idx, role );
        if( indexData.type() != QVariant::List )
            return indexData;

        QString providerString = indexData.toStringList().join( ", " );
        return QVariant( providerString );
    }

    return QtGroupingProxy::data( idx, role );
}

Qt::ItemFlags
PlaylistsByProviderProxy::flags( const QModelIndex &idx ) const
{
    //TODO: check if provider supports addPlaylist for DropEnabled
    if( isGroup( idx ) )
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;

    return QtGroupingProxy::flags( idx );
}

bool
PlaylistsByProviderProxy::removeRows( int row, int count, const QModelIndex &parent )
{
    DEBUG_BLOCK
    bool result;
    debug() << "in parent " << parent << "remove " << count << " starting at row " << row;
    QModelIndex originalIdx = mapToSource( parent );
    result = sourceModel()->removeRows( row, count, originalIdx );
    if( result )
    {
        beginRemoveRows( parent, row, row + count - 1 );
        endRemoveRows();
    }
    return result;
}

//TODO: move the next 3 implementation to QtGroupingProxy
QStringList
PlaylistsByProviderProxy::mimeTypes() const
{
    //nothing to add
    return sourceModel()->mimeTypes();
}

QMimeData *
PlaylistsByProviderProxy::mimeData( const QModelIndexList &indexes ) const
{
    DEBUG_BLOCK
    QModelIndexList sourceIndexes;
    foreach( const QModelIndex &idx, indexes )
    {
        debug() << idx;
        if( isGroup( idx ) )
        {
            debug() << "is a group, add mimeData of all children";
            //TODO: add originalRows of children to list
        }
        else
        {
            debug() << "is original item, add mimeData from source model";
            QModelIndex originalIdx = mapToSource( idx );
            if( originalIdx.isValid() )
                sourceIndexes << originalIdx;
        }
    }

    QMimeData* mime = 0;
    if( !sourceIndexes.isEmpty() )
        mime = sourceModel()->mimeData( sourceIndexes );

    if( !mime )
        mime = new QMimeData();

    if( !sourceIndexes.isEmpty() )
    {
        QByteArray encodedIndexes = encodeMimeRows( sourceIndexes );
        mime->setData( AMAROK_PROVIDERPROXY_INDEXES, encodedIndexes );
    }

    return mime;
}

QByteArray
PlaylistsByProviderProxy::encodeMimeRows( const QList<QModelIndex> indexes ) const
{
    QByteArray encodedIndexes;
    QDataStream stream( &encodedIndexes, QIODevice::WriteOnly );
    foreach( const QModelIndex &idx, indexes )
    {
        QStack<QModelIndex> indexStack;
        //save the index and it's parents until we reach the rootnode so we have the complete tree.
        QModelIndex i = idx;
        while( i != m_rootNode )
        {
            indexStack.push( i );
            i = i.parent();
        }
        //save the length of the stack first.
        stream << indexStack.count();
        while( !indexStack.isEmpty() )
        {
            QModelIndex i = indexStack.pop();
            stream << i.row() << i.column();
        }
    }

    return encodedIndexes;
}

QList<QModelIndex>
PlaylistsByProviderProxy::decodeMimeRows( QByteArray mimeData, QAbstractItemModel *model ) const
{
    DEBUG_BLOCK
    debug() << mimeData;
    QList<QModelIndex> idxs;
    QDataStream stream( &mimeData, QIODevice::ReadOnly );
    while( !stream.atEnd() )
    {
        QStack<QModelIndex> indexStack;
        int count;
        stream >> count;
        //start from the rootNode and build "down" the tree
        QModelIndex idx = m_rootNode;
        while( count-- > 0 )
        {
            int row;
            int column;
            stream >> row >> column;
            idx = model->index( row, column, idx );
        }
        //the last one should be the index we saved in encodeMimeRows
        idxs << idx;
    }
    return idxs;
}

bool
PlaylistsByProviderProxy::dropMimeData( const QMimeData *data, Qt::DropAction action,
                                   int row, int column, const QModelIndex &parent )
{
    DEBUG_BLOCK
    debug() << "dropped on " << QString("row: %1, column: %2, parent:").arg( row ).arg( column );
    debug() << parent;
    debug() << "With action: " << action;
    if( action == Qt::IgnoreAction )
    {
        debug() << "ignored";
        return true;
    }

    if( isGroup( parent ) )
    {
        if( data->hasFormat( AMAROK_PROVIDERPROXY_INDEXES ) )
        {
            QList<QModelIndex> originalIndexes =
                    decodeMimeRows( data->data( AMAROK_PROVIDERPROXY_INDEXES ), sourceModel() );
            //set the groupedColumn data of all playlist indexes to the data of this group
            //the model will understand this as a copy to the provider it's dropped on
            ItemData groupData =
                    m_groupMaps.value( parent.row() ).value( parent.column() );
            bool result = !originalIndexes.isEmpty();
            foreach( const QModelIndex& originalIndex, originalIndexes )
            {
                QModelIndex groupedColumnIndex =
                        originalIndex.sibling( originalIndex.row(), m_groupedColumn );
                if( !groupedColumnIndex.isValid() )
                    continue;

                result = sourceModel()->setItemData( groupedColumnIndex, groupData ) ? result : false;
            }
            return result;
        }
        return false;
    }

    QModelIndex sourceIndex = mapToSource( parent );
    return sourceModel()->dropMimeData( data, action, row, column,
                               sourceIndex );
}

Qt::DropActions
PlaylistsByProviderProxy::supportedDropActions() const
{
    //always add CopyAction because playlists can copied to a Provider
    return sourceModel()->supportedDropActions() | Qt::CopyAction;
}

Qt::DropActions
PlaylistsByProviderProxy::supportedDragActions() const
{
    //always add CopyAction because playlists can be put into a different group
    return sourceModel()->supportedDragActions() | Qt::CopyAction;
}

void
PlaylistsByProviderProxy::setSourceModel( QAbstractItemModel *model )
{
    if( sourceModel() )
        sourceModel()->disconnect();

    QtGroupingProxy::setSourceModel( model );

    connect( sourceModel(), SIGNAL(renameIndex( const QModelIndex & )),
             SLOT(slotRenameIndex( const QModelIndex & )) );
}

void
PlaylistsByProviderProxy::buildTree()
{
    //clear that data anyway since provider can disappear and should no longer be listed.
    m_groupMaps.clear();
    QtGroupingProxy::buildTree();
}

void
PlaylistsByProviderProxy::slotRenameIndex( const QModelIndex &sourceIdx )
{
    QModelIndex idx = mapFromSource( sourceIdx );
    if( idx.isValid() )
        emit renameIndex( idx );
}
