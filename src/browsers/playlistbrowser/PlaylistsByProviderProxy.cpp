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
#include "PlaylistBrowserModel.h"

#include "core/playlists/PlaylistProvider.h"
#include "core/support/Debug.h"
#include "playlistmanager/PlaylistManager.h"
#include "widgets/PrettyTreeRoles.h"

#include <QIcon>

#include <QStack>

PlaylistsByProviderProxy::PlaylistsByProviderProxy( int playlistCategory, QObject *parent )
    : QtGroupingProxy( parent )
    , m_playlistCategory( playlistCategory )
{
    // we need this to track providers with no playlists
    connect( The::playlistManager(), &PlaylistManager::providerAdded,
             this, &PlaylistsByProviderProxy::slotProviderAdded );
    connect( The::playlistManager(), &PlaylistManager::providerRemoved,
             this, &PlaylistsByProviderProxy::slotProviderRemoved );
}

//TODO: remove this constructor
PlaylistsByProviderProxy::PlaylistsByProviderProxy( QAbstractItemModel *model, int column, int playlistCategory )
        : QtGroupingProxy( model, QModelIndex(), column )
        , m_playlistCategory( playlistCategory )
{
    setSourceModel( model );

    // we need this to track providers with no playlists
    connect( The::playlistManager(), &PlaylistManager::providerAdded,
             this, &PlaylistsByProviderProxy::slotProviderAdded );
    connect( The::playlistManager(), &PlaylistManager::providerRemoved,
             this, &PlaylistsByProviderProxy::slotProviderRemoved );
}

QVariant
PlaylistsByProviderProxy::data( const QModelIndex &idx, int role ) const
{
    //TODO: actions for empty providers

    //TODO: filter out actions not from the provider, possibly using QAction separators marking
    // the source of the actions (makes sense in the UI as well.

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
    for( const QModelIndex &idx : indexes )
    {
        if( isGroup( idx ) )
            continue; // drags not enabled for playlist providers
        QModelIndex originalIdx = mapToSource( idx );
        if( originalIdx.isValid() )
            sourceIndexes << originalIdx;
    }

    if( sourceIndexes.isEmpty() )
        return nullptr;
    return sourceModel()->mimeData( sourceIndexes );
}

bool
PlaylistsByProviderProxy::dropMimeData( const QMimeData *data, Qt::DropAction action,
                                        int row, int column, const QModelIndex &parent )
{
    DEBUG_BLOCK
    debug() << "Dropped on" << parent << "row" << row << "column" << column << "action" << action;
    if( action == Qt::IgnoreAction )
        return true;

    if( !isGroup( parent ) ) // drops on empty space fall here, it is okay
    {
        QModelIndex sourceIndex = mapToSource( parent );
        return sourceModel()->dropMimeData( data, action, row, column, sourceIndex );
    }

    const AmarokMimeData *amarokData = dynamic_cast<const AmarokMimeData *>( data );
    if( !amarokData )
    {
        debug() << __PRETTY_FUNCTION__ << "supports only drag & drop originating in Amarok.";
        return false;
    }

    Playlists::PlaylistProvider *provider =
        parent.data( PlaylistBrowserNS::PlaylistBrowserModel::ProviderRole )
        .value<Playlists::PlaylistProvider *>();
    if( !provider )
    {
        warning() << "Dropped tracks to a group with no (or multiple) providers!";
        return false;
    }

    if( amarokData->hasFormat( AmarokMimeData::PLAYLIST_MIME ) )
    {
        debug() << "Dropped playlists to provider" << provider->prettyName();
        for( Playlists::PlaylistPtr pl : amarokData->playlists() )
        {
            // few PlaylistProviders implement addPlaylist(), use save() instead:
            The::playlistManager()->save( pl->tracks(), pl->name(), provider, false /* editName */ );
        }
        return true;
    }
    if( amarokData->hasFormat( AmarokMimeData::TRACK_MIME ) )
    {
        debug() << "Dropped tracks to provider" << provider->prettyName();
        Meta::TrackList tracks = amarokData->tracks();
        QString playlistName = Amarok::generatePlaylistName( tracks );
        return The::playlistManager()->save( tracks, playlistName, provider );
    }

    debug() << __PRETTY_FUNCTION__ << "Unsupported drop mime-data:" << data->formats();
    return false;
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

    connect( sourceModel(), SIGNAL(renameIndex(QModelIndex)),
             SLOT(slotRenameIndex(QModelIndex)) );
}

void
PlaylistsByProviderProxy::buildTree()
{
    //clear that data anyway since provider can disappear and should no longer be listed.
    m_groupMaps.clear();

    //add the empty providers at the top of the list
    PlaylistProviderList providerList =
            The::playlistManager()->providersForCategory( m_playlistCategory );

    for( Playlists::PlaylistProvider *provider : providerList )
    {
        slotProviderAdded( provider, provider->category() );
    }

    QtGroupingProxy::buildTree();
}

void
PlaylistsByProviderProxy::slotRenameIndex( const QModelIndex &sourceIdx )
{
    QModelIndex idx = mapFromSource( sourceIdx );
    if( idx.isValid() )
        Q_EMIT renameIndex( idx );
}

void
PlaylistsByProviderProxy::slotProviderAdded( Playlists::PlaylistProvider *provider, int category )
{
    DEBUG_BLOCK
    if( category != m_playlistCategory )
        return;

    if( provider->playlistCount() > 0
        || ( provider->playlistCount() < 0 /* not counted */
             && !provider->playlists().isEmpty() ) )
            return; // non-empty providers are handled by PlaylistBrowserModel

    ItemData itemData;
    itemData.insert( Qt::DisplayRole, provider->prettyName() );
    itemData.insert( Qt::DecorationRole, provider->icon() );
    itemData.insert( PrettyTreeRoles::DecoratorRole, QVariant::fromValue( provider->providerActions() ) );
    itemData.insert( PrettyTreeRoles::DecoratorRoleCount, provider->providerActions().count() );

    itemData.insert( PlaylistBrowserNS::PlaylistBrowserModel::ProviderRole,
                        QVariant::fromValue<Playlists::PlaylistProvider*>( provider ) );
    RowData rowData;
    rowData.insert( PlaylistBrowserNS::PlaylistBrowserModel::PlaylistItemColumn, itemData );
    //Provider column is used for filtering.
    rowData.insert( PlaylistBrowserNS::PlaylistBrowserModel::ProviderColumn, itemData );

    addEmptyGroup( rowData );
}

void
PlaylistsByProviderProxy::slotProviderRemoved( Playlists::PlaylistProvider *provider, int category )
{
    DEBUG_BLOCK
    if( category != m_playlistCategory )
        return;

    for( int i = 0; i < rowCount(); i++ )
    {
        QModelIndex idx = index( i, PlaylistBrowserNS::PlaylistBrowserModel::PlaylistItemColumn );
        Playlists::PlaylistProvider *rowProvider = data( idx, PlaylistBrowserNS::PlaylistBrowserModel::ProviderRole )
            .value<Playlists::PlaylistProvider *>();
        if( rowProvider != provider )
            continue;

        removeGroup( idx );
    }
}
