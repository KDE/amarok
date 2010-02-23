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

#include "Debug.h"

const QString PlaylistsByProviderProxy::AMAROK_PROVIDERPROXY_INDEXES =
        "application/x-amarok-providerproxy-indexes";

PlaylistsByProviderProxy::PlaylistsByProviderProxy( QAbstractItemModel *model, int column )
        : QtGroupingProxy( model, QModelIndex(), column )
{
    connect( m_model, SIGNAL( renameIndex( QModelIndex ) ), SLOT( slotRename( QModelIndex ) ) );
}

bool
PlaylistsByProviderProxy::removeRows( int row, int count, const QModelIndex &parent )
{
    DEBUG_BLOCK
    bool result;
    debug() << "in parent " << parent << "remove " << count << " starting at row " << row;
    beginRemoveRows( parent, row, row + count );
    QModelIndex originalIdx = mapToSource( parent );
    result = m_model->removeRows( row, count, originalIdx );
    endRemoveRows();
    return result;
}

//TODO: move the next 3 implementation to QtGroupingProxy
QStringList
PlaylistsByProviderProxy::mimeTypes() const
{
    //nothing to add
    return m_model->mimeTypes();
}

QMimeData *
PlaylistsByProviderProxy::mimeData( const QModelIndexList &indexes ) const
{
    DEBUG_BLOCK
    QModelIndexList sourceIndexes;
    QList<int> originalRows;
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
            {
                sourceIndexes << originalIdx;
                originalRows << originalIdx.row();
            }
        }
    }

    QMimeData* mime = 0;
    if( !sourceIndexes.isEmpty() )
        mime = m_model->mimeData( sourceIndexes );

    if( !mime )
        mime = new QMimeData();

    if( !originalRows.isEmpty() )
    {
        QByteArray encodedRows = encodeMimeRows( originalRows );
        mime->setData( AMAROK_PROVIDERPROXY_INDEXES, encodedRows );
    }

    return mime;
}

QByteArray
PlaylistsByProviderProxy::encodeMimeRows( const QList<int> rows )
{
    QByteArray encodedRows;
    QDataStream stream( &encodedRows, QIODevice::WriteOnly );
    foreach( int row, rows )
        stream << row;

    return encodedRows;
}

QList<int>
PlaylistsByProviderProxy::decodeMimeRows( QByteArray mimeData )
{
    DEBUG_BLOCK
    debug() << mimeData;
    QList<int> rows;
    QDataStream stream( &mimeData, QIODevice::ReadOnly );
    while( !stream.atEnd() )
    {
        int row;
        stream >> row;
        rows << row;
    }
    return rows;
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
        if( data->hasFormat( AmarokMimeData::PODCASTCHANNEL_MIME ) )
        {
            const AmarokMimeData* amarokMime = dynamic_cast<const AmarokMimeData*>( data );
            QList<int> originalRows = decodeMimeRows( data->data( AMAROK_PROVIDERPROXY_INDEXES ) );
            foreach( Meta::PodcastChannelPtr channel, amarokMime->podcastChannels() )
            {
                //set the groupedColumn data of all playlist indexes to the data of this group
                //the model will understand this as a copy to the provider it's dropped on
                RoleVariantMap groupData =
                        m_groupMaps.value( parent.row() ).value( parent.column() );

                int originalRow = originalRows.takeFirst();
                QModelIndex originalIndex =
                        m_model->index( originalRow, m_groupedColumn, m_rootNode );
                if( !originalIndex.isValid() )
                    continue;

                m_model->setItemData( originalIndex, groupData );
            }
            return true;
        }
        return false;
    }

    QModelIndex sourceIndex = mapToSource( parent );
    return m_model->dropMimeData( data, action, row, column,
                               sourceIndex );
}

Qt::DropActions
PlaylistsByProviderProxy::supportedDropActions() const
{
    //always add CopyAction because playlists can copied to a Provider
    return m_model->supportedDropActions() | Qt::CopyAction;
}

Qt::DropActions
PlaylistsByProviderProxy::supportedDragActions() const
{
    //always add CopyAction because playlists can be put into a different group
    return m_model->supportedDragActions() | Qt::CopyAction;
}

QList<QAction *>
PlaylistsByProviderProxy::actionsFor( const QModelIndexList &list )
{
    DEBUG_BLOCK
    QList<QAction *> actions;
    if( isAGroupSelected( list ) )
    {
        //TODO: get provider actions
        return actions;
    }

    QModelIndexList originalList = mapToSource( list );
    debug() << originalList.count() << "original indices";
    MetaPlaylistModel *mpm = dynamic_cast<MetaPlaylistModel *>( m_model );
    if( mpm == 0 )
        return actions;
    if( !originalList.isEmpty() )
        actions << mpm->actionsFor( originalList );

    return actions;
}

void
PlaylistsByProviderProxy::loadItems( QModelIndexList list, Playlist::AddOptions insertMode )
{
    if( isAGroupSelected( list ) )
        return;
    QModelIndexList originalList = mapToSource( list );
    debug() << originalList.count() << "original indices";
    MetaPlaylistModel *mpm = dynamic_cast<MetaPlaylistModel *>( m_model );
    if( mpm == 0 )
        return;
    if( !originalList.isEmpty() )
        mpm->loadItems( originalList, insertMode );
}

void
PlaylistsByProviderProxy::buildTree()
{
    //clear that data anyway since provider can disappear and should no longer be listed.
    m_groupMaps.clear();
    QtGroupingProxy::buildTree();
}

void
PlaylistsByProviderProxy::slotRename( QModelIndex sourceIdx )
{
    QModelIndex proxyIdx = mapFromSource( sourceIdx );
    emit( renameIndex( proxyIdx ) );
}
