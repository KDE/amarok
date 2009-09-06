/****************************************************************************************
 * Copyright (c) 2009 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "PlaylistsInGroupsProxy.h"

#include "AmarokMimeData.h"
#include "Debug.h"
#include "meta/Playlist.h"
#include "SvgHandler.h"
#include "UserPlaylistModel.h"
#include "playlist/PlaylistModelStack.h"

#include <KIcon>
#include <KInputDialog>

PlaylistsInGroupsProxy::PlaylistsInGroupsProxy( QAbstractItemModel *model )
    : QtGroupingProxy( model, QModelIndex(), 1 )
    , m_renameFolderAction( 0 )
    , m_deleteFolderAction( 0 )
{
    connect( m_model, SIGNAL( renameIndex( QModelIndex ) ), SLOT( slotRename( QModelIndex ) ) );
}

PlaylistsInGroupsProxy::~PlaylistsInGroupsProxy()
{
}

bool
PlaylistsInGroupsProxy::removeRows( int row, int count, const QModelIndex &parent )
{
    DEBUG_BLOCK
    debug() << "in parent " << parent << "remove " << count << " starting at row " << row;
    if( isGroup( parent ) )
    {
        deleteFolder( parent );
        return true;
    }

    QModelIndex originalIdx = mapToSource( parent );
    bool success = m_model->removeRows( row, count, originalIdx );

    return success;
}

QStringList
PlaylistsInGroupsProxy::mimeTypes() const
{
    QStringList mimeTypes = m_model->mimeTypes();
    mimeTypes << AmarokMimeData::PLAYLISTBROWSERGROUP_MIME;
    return mimeTypes;
}

QMimeData *
PlaylistsInGroupsProxy::mimeData( const QModelIndexList &indexes ) const
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
PlaylistsInGroupsProxy::dropMimeData( const QMimeData *data, Qt::DropAction action,
                                   int row, int column, const QModelIndex &parent )
{
    DEBUG_BLOCK
    Q_UNUSED( row );
    Q_UNUSED( column );
    debug() << "dropped on " << QString("row: %1, column: %2, parent:").arg( row ).arg( column );
    debug() << parent;
    if( action == Qt::IgnoreAction )
    {
        debug() << "ignored";
        return true;
    }

    if( !parent.isValid() )
    {
        debug() << "dropped on the root";
        const AmarokMimeData *amarokMime = dynamic_cast<const AmarokMimeData *>(data);
        if( amarokMime == 0 )
        {
            debug() << "ERROR: could not cast to amarokMimeData at " << __FILE__ << __LINE__;
            return false;
        }
        Meta::PlaylistList playlists = amarokMime->playlists();
        foreach( Meta::PlaylistPtr playlist, playlists )
            playlist->setGroups( QStringList() );
        buildTree();
        return true;
    }

    if( isGroup( parent ) )
    {
        debug() << "dropped on a group";
        if( data->hasFormat( AmarokMimeData::PLAYLIST_MIME ) )
        {
            debug() << "playlist dropped on group";
            if( parent.row() < 0 || parent.row() >= rowCount( QModelIndex() ) )
            {
                debug() << "ERROR: something went seriously wrong in " << __FILE__ << __LINE__;
                return false;
            }
            //apply the new groupname to the source index
            QString groupName = parent.data( Qt::DisplayRole ).toString();
            debug() << QString("apply the new groupname %1 to the source index").arg( groupName );
            const AmarokMimeData *amarokMime = dynamic_cast<const AmarokMimeData *>(data);
            if( amarokMime == 0 )
            {
                debug() << "ERROR: could not cast to amarokMimeData at " << __FILE__ << __LINE__;
                return false;
            }
            Meta::PlaylistList playlists = amarokMime->playlists();
            foreach( Meta::PlaylistPtr playlist, playlists )
                playlist->setGroups( QStringList( groupName ) );
            buildTree();
            return true;
        }
        else if( data->hasFormat( AmarokMimeData::PLAYLISTBROWSERGROUP_MIME ) )
        {
            debug() << "playlistgroup dropped on group";
            //TODO: multilevel group support
            debug() << "ignore drop until we have multilevel group support";
            return false;
        }
    }
    else
    {
        debug() << "not dropped on the root or on a group";
        QModelIndex sourceIndex = mapToSource( parent );
        return m_model->dropMimeData( data, action, row, column,
                               sourceIndex );
    }

    return false;
}

Qt::DropActions
PlaylistsInGroupsProxy::supportedDropActions() const
{
    //always add MoveAction because playlists can be put into a different group
    return m_model->supportedDropActions() | Qt::MoveAction;
}

Qt::DropActions
PlaylistsInGroupsProxy::supportedDragActions() const
{
    //always add MoveAction because playlists can be put into a different group
    return m_model->supportedDragActions() | Qt::MoveAction;
}

void
PlaylistsInGroupsProxy::slotRename( QModelIndex sourceIdx )
{
    QModelIndex proxyIdx = mapFromSource( sourceIdx );
    emit renameIndex( proxyIdx );
}

void
PlaylistsInGroupsProxy::slotDeleteFolder()
{
    DEBUG_BLOCK
    if( m_selectedGroups.count() == 0 )
        return;

    QModelIndex groupIdx = m_selectedGroups.first();
    deleteFolder( groupIdx );
}

void
PlaylistsInGroupsProxy::slotRenameFolder()
{
    DEBUG_BLOCK
    //get the name for this new group
    //TODO: do inline rename
    QModelIndex folder = m_selectedGroups.first();
    QString folderName = folder.data( Qt::DisplayRole ).toString();
    bool ok;
    const QString newName = KInputDialog::getText( i18n("New name"),
                i18nc("Enter a new name for a folder that already exists",
                      "Enter new folder name:"),
                folderName,
                &ok );
    if( !ok || newName == folderName )
        return;

    for( int i = 0; i < rowCount( folder ); i++ )
    {
        QModelIndex idx = m_model->index( i, 0, QModelIndex() );
        setData( idx, newName, GroupRole );
    }
    buildTree();
    emit layoutChanged();
}

QList<QAction *>
PlaylistsInGroupsProxy::createGroupActions()
{
    QList<QAction *> actions;

    if ( m_deleteFolderAction == 0 )
    {
        m_deleteFolderAction = new QAction( KIcon( "media-track-remove-amarok" ),
                                            i18n( "&Delete Folder" ), this );
        m_deleteFolderAction->setProperty( "popupdropper_svg_id", "delete_group" );
        connect( m_deleteFolderAction, SIGNAL( triggered() ), this,
                 SLOT( slotDeleteFolder() ) );
    }
    actions << m_deleteFolderAction;

    if ( m_renameFolderAction == 0 )
    {
        m_renameFolderAction =  new QAction( KIcon( "media-track-edit-amarok" ),
                                             i18n( "&Rename Folder..." ), this );
        m_renameFolderAction->setProperty( "popupdropper_svg_id", "edit_group" );
        connect( m_renameFolderAction, SIGNAL( triggered() ), this,
                 SLOT( slotRenameFolder() ) );
    }
    actions << m_renameFolderAction;

    return actions;
}

bool
PlaylistsInGroupsProxy::isAPlaylistSelected( const QModelIndexList& list ) const
{
    return mapToSource( list ).count() > 0;
}

void
PlaylistsInGroupsProxy::deleteFolder( const QModelIndex &groupIdx )
{
    DEBUG_BLOCK
    //TODO: make work
}

QList<QAction *>
PlaylistsInGroupsProxy::actionsFor( const QModelIndexList &list )
{
    DEBUG_BLOCK
    bool playlistSelected = isAPlaylistSelected( list );
    bool groupSelected = isAGroupSelected( list );

    QList<QAction *> actions;
    m_selectedGroups.clear();
    m_selectedPlaylists.clear();

    //only playlists selected
    if( playlistSelected )
    {
        foreach( const QModelIndex &index, list )
        {
            QModelIndexList tempList;
            tempList << index;
            if( isAPlaylistSelected( tempList ) )
                m_selectedPlaylists << index;
        }
        QModelIndexList originalList = mapToSource( list );
        debug() << originalList.count() << "original indices";
        MetaPlaylistModel *mpm = dynamic_cast<MetaPlaylistModel *>(m_model);
        if( mpm == 0 )
            return actions;
        if( !originalList.isEmpty() )
            actions << mpm->actionsFor( originalList );
    }
    else if( groupSelected )
    {
        QModelIndexList originalList;
        originalList << m_model->index( 0, 0, QModelIndex() );
        originalList << m_model->index( 1, 0, QModelIndex() );
        MetaPlaylistModel *mpm = dynamic_cast<MetaPlaylistModel *>(m_model);
        if( mpm == 0 )
            return actions;
        actions << mpm->actionsFor( originalList );
        actions << createGroupActions();
        foreach( const QModelIndex &index, list )
        {
            QModelIndexList tempList;
            tempList << index;
            if( isAGroupSelected( tempList ) )
                m_selectedGroups << index;
        }
    }

    return actions;
}

void
PlaylistsInGroupsProxy::loadItems( QModelIndexList list, Playlist::AddOptions insertMode )
{
    QModelIndexList originalList = mapToSource( list );
    MetaPlaylistModel *mpm = dynamic_cast<MetaPlaylistModel *>(m_model);
    if( mpm == 0 )
        return;
    mpm->loadItems( originalList, insertMode );
}

QModelIndex
PlaylistsInGroupsProxy::createNewGroup( const QString &groupName )
{
    if( !insertRow( 0, QModelIndex() ) )
        return QModelIndex();

    emit layoutChanged();
    return index( 0, 0, QModelIndex() );
}

#include "PlaylistsInGroupsProxy.moc"
