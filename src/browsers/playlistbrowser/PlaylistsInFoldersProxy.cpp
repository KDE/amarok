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

#include "PlaylistsInFoldersProxy.h"

#include "AmarokMimeData.h"
#include "MainWindow.h"
#include "core/support/Debug.h"
#include "core/playlists/Playlist.h"
#include "UserPlaylistModel.h"
#include "playlist/PlaylistModelStack.h"
#include "widgets/PrettyTreeRoles.h"

#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>

PlaylistsInFoldersProxy::PlaylistsInFoldersProxy( QAbstractItemModel *model )
    : QtGroupingProxy( model, QModelIndex(), PlaylistBrowserNS::UserModel::LabelColumn )
{
    m_renameFolderAction =  new QAction( QIcon::fromTheme( QStringLiteral("media-track-edit-amarok") ),
                                         i18n( "&Rename Folder..." ), this );
    m_renameFolderAction->setProperty( "popupdropper_svg_id", QStringLiteral("edit_group") );
    connect( m_renameFolderAction, &QAction::triggered, this, &PlaylistsInFoldersProxy::slotRenameFolder );

    m_deleteFolderAction = new QAction( QIcon::fromTheme( QStringLiteral("media-track-remove-amarok") ),
                                        i18n( "&Delete Folder" ), this );
    m_deleteFolderAction->setProperty( "popupdropper_svg_id", QStringLiteral("delete_group") );
    m_deleteFolderAction->setObjectName( QStringLiteral("deleteAction") );
    connect( m_deleteFolderAction, &QAction::triggered, this, &PlaylistsInFoldersProxy::slotDeleteFolder );

    if( auto m = static_cast<PlaylistBrowserNS::PlaylistBrowserModel*>(sourceModel()) )
        connect( m, &PlaylistBrowserNS::PlaylistBrowserModel::renameIndex,
                 this, &PlaylistsInFoldersProxy::slotRenameIndex );
}

PlaylistsInFoldersProxy::~PlaylistsInFoldersProxy()
{
}

QVariant
PlaylistsInFoldersProxy::data( const QModelIndex &idx, int role ) const
{

    //Turn the QVariantList of the source into a comma separated string, but only for the real items
    if( !isGroup( idx ) && idx.column() == PlaylistBrowserNS::PlaylistBrowserModel::ProviderColumn
        && role == Qt::DisplayRole )
    {
        QVariant indexData = QtGroupingProxy::data( idx, role );
        if( indexData.type() != QVariant::List )
            return indexData;

        QString providerString = indexData.toStringList().join( QStringLiteral(", ") );
        return QVariant( providerString );
    }

    if( idx.column() == 0 && isGroup( idx ) &&
        role == PrettyTreeRoles::DecoratorRole )
    {
        //whether we use the list from m_deleteFolderAction or m_renameFolderAction does not matter
        //they are the same anyway
        QPersistentModelIndexList actionList =
                m_deleteFolderAction->data().value<QPersistentModelIndexList>();

        //make a persistent modelindex since the location of the groups can change while executing
        //actions.
        actionList << QPersistentModelIndex( idx );
        QVariant value = QVariant::fromValue( actionList );
        m_deleteFolderAction->setData( value );
        m_renameFolderAction->setData( value );

        QList<QAction *> actions;
        actions << m_renameFolderAction << m_deleteFolderAction;
        return QVariant::fromValue( actions );
    }

    if( idx.column() == 0 && isGroup( idx ) &&
        role == PrettyTreeRoles::DecoratorRoleCount )
        return 2; // 2 actions, see above

    return QtGroupingProxy::data( idx, role );
}

bool
PlaylistsInFoldersProxy::removeRows( int row, int count, const QModelIndex &parent )
{
    DEBUG_BLOCK
    bool result;
    debug() << "in parent " << parent << "remove " << count << " starting at row " << row;
    if( !parent.isValid() )
    {
        QModelIndex folderIdx = index( row, 0, QModelIndex() );
        if( isGroup( folderIdx ) )
        {
            deleteFolder( folderIdx );
            return true;
        }

        //is a playlist not in a folder
        QModelIndex childIdx = mapToSource( index( row, 0, m_rootIndex ) );
        result = sourceModel()->removeRows( childIdx.row(), count, m_rootIndex );
        if( result )
        {
            beginRemoveRows( parent, row, row + count - 1 );
            endRemoveRows();
        }
        return result;
    }

    if( isGroup( parent ) )
    {
        result = true;
        for( int i = row; i < row + count; i++ )
        {
            QModelIndex childIdx = mapToSource( index( i, 0, parent ) );
            //set success to false if removeRows returns false
            result = sourceModel()->removeRow( childIdx.row(), QModelIndex() ) ? result : false;
        }
        return result;
    }

    //removing a track from a playlist
    beginRemoveRows( parent, row, row + count - 1 );
    QModelIndex originalIdx = mapToSource( parent );
    result = sourceModel()->removeRows( row, count, originalIdx );
    endRemoveRows();

    return result;
}

QStringList
PlaylistsInFoldersProxy::mimeTypes() const
{
    QStringList mimeTypes = sourceModel()->mimeTypes();
    mimeTypes << AmarokMimeData::PLAYLISTBROWSERGROUP_MIME;
    return mimeTypes;
}

QMimeData *
PlaylistsInFoldersProxy::mimeData( const QModelIndexList &indexes ) const
{
    DEBUG_BLOCK
    AmarokMimeData* mime = new AmarokMimeData();
    QModelIndexList sourceIndexes;
    for( const QModelIndex &idx : indexes )
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
        return sourceModel()->mimeData( sourceIndexes );

    return mime;
}

bool
PlaylistsInFoldersProxy::dropMimeData( const QMimeData *data, Qt::DropAction action,
                                   int row, int column, const QModelIndex &parent )
{
    DEBUG_BLOCK
    Q_UNUSED( row );
    Q_UNUSED( column );
    debug() << "dropped on " << QStringLiteral("row: %1, column: %2, parent:").arg( row ).arg( column );
    debug() << parent;
    if( action == Qt::IgnoreAction )
    {
        debug() << "ignored";
        return true;
    }

    if( data->hasFormat( AmarokMimeData::PLAYLIST_MIME ) ||
        data->hasFormat( AmarokMimeData::PLAYLISTBROWSERGROUP_MIME ) )
    {
        debug() << "has amarok mime data";
        const AmarokMimeData *amarokMime = dynamic_cast<const AmarokMimeData *>(data);
        if( amarokMime == nullptr )
        {
            error() << "could not cast to amarokMimeData";
            return false;
        }

        if( !parent.isValid() )
        {
            debug() << "dropped on the root";
            Playlists::PlaylistList playlists = amarokMime->playlists();
            for( Playlists::PlaylistPtr playlist : playlists )
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
                //TODO: apply the new groupname to the source index
                Playlists::PlaylistList playlists = amarokMime->playlists();
                for( Playlists::PlaylistPtr playlist : playlists )
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
    }
    else
    {
        QModelIndex sourceIndex = mapToSource( parent );
        return sourceModel()->dropMimeData( data, action, row, column,
                               sourceIndex );
    }

    return false;
}

Qt::DropActions
PlaylistsInFoldersProxy::supportedDropActions() const
{
    //always add MoveAction because playlists can be put into a different group
    return sourceModel()->supportedDropActions() | Qt::MoveAction;
}

Qt::DropActions
PlaylistsInFoldersProxy::supportedDragActions() const
{
    //always add MoveAction because playlists can be put into a different group
    return sourceModel()->supportedDragActions() | Qt::MoveAction;
}

void
PlaylistsInFoldersProxy::setSourceModel( QAbstractItemModel *model )
{
    if( sourceModel() )
        sourceModel()->disconnect();

    QtGroupingProxy::setSourceModel( model );

    connect( sourceModel(), SIGNAL(renameIndex(QModelIndex)),
             SLOT(slotRenameIndex(QModelIndex)) );
}

void
PlaylistsInFoldersProxy::slotRenameIndex( const QModelIndex &sourceIdx )
{
    QModelIndex idx = mapFromSource( sourceIdx );
    if( idx.isValid() )
        Q_EMIT renameIndex( idx );
}

void
PlaylistsInFoldersProxy::slotDeleteFolder()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == nullptr )
        return;

    QPersistentModelIndexList indexes = action->data().value<QPersistentModelIndexList>();

    for( const QModelIndex &groupIdx : indexes )
        deleteFolder( groupIdx );
}

void
PlaylistsInFoldersProxy::slotRenameFolder()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == nullptr )
        return;

    QPersistentModelIndexList indexes = action->data().value<QPersistentModelIndexList>();

    if( indexes.isEmpty() )
        return;

    //get the name for this new group
    //inline rename is handled by the view using setData()
    QModelIndex folder = indexes.first();
    QString folderName = folder.data( Qt::DisplayRole ).toString();
    bool ok;
    const QString newName = QInputDialog::getText( nullptr,
                                                   i18n("New name"),
                                                   i18nc("Enter a new name for a folder that already exists",
                                                         "Enter new folder name:"),
                                                   QLineEdit::Normal,
                                                   folderName,
                                                   &ok );
    if( !ok || newName == folderName )
        return;

    setData( folder, newName );
}

void
PlaylistsInFoldersProxy::deleteFolder( const QModelIndex &groupIdx )
{
    int childCount = rowCount( groupIdx );
    if( childCount > 0 )
    {
        auto button = QMessageBox::question( The::mainWindow(),
                                             i18n( "Confirm Delete" ),
                                             i18n( "Are you sure you want to delete this folder and its contents?" ) );
        //TODO:include a text area with all the names of the playlists

        if( button != QMessageBox::Yes )
            return;

        removeRows( 0, childCount, groupIdx );
    }
    removeGroup( groupIdx );
    //force a rebuild because groupHash might be incorrect
    //TODO: make QtGroupingProxy adjust groupHash keys
    buildTree();
}

QModelIndex
PlaylistsInFoldersProxy::createNewFolder( const QString &groupName )
{
    RowData data;
    ItemData roleData;
    roleData.insert( Qt::DisplayRole, groupName );
    roleData.insert( Qt::DecorationRole, QVariant( QIcon::fromTheme( QStringLiteral("folder") ) ) );
    roleData.insert( Qt::EditRole, groupName );
    data.insert( 0, roleData );
    return addEmptyGroup( data );
}

Qt::ItemFlags PlaylistsInFoldersProxy::flags(const QModelIndex &idx) const
{
    if( isGroup(idx) && idx.column() == 0)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable |
               Qt::ItemIsDropEnabled;

    return QtGroupingProxy::flags(idx);
}

