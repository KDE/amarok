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
    AmarokMimeData* mime = new AmarokMimeData();
    QModelIndexList sourceIndexes;
    foreach( const QModelIndex &idx, indexes )
    {
        debug() << idx;
        if( isGroup( idx ) )
        {
            debug() << "is a group, add mimeData of all children";
        }
        else
        {
            debug() << "is original item, add mimeData from source model";
            sourceIndexes << mapToSource( idx );
        }
    }

    if( !sourceIndexes.isEmpty() )
        return m_model->mimeData( sourceIndexes );

    return mime;
}

bool
PlaylistsByProviderProxy::dropMimeData( const QMimeData *data, Qt::DropAction action,
                                   int row, int column, const QModelIndex &parent )
{
    DEBUG_BLOCK
    debug() << "dropped on " << QString("row: %1, column: %2, parent:").arg( row ).arg( column );
    debug() << parent;
    if( action == Qt::IgnoreAction )
    {
        debug() << "ignored";
        return true;
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
