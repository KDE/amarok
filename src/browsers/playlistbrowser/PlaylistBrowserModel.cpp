/****************************************************************************************
 * Copyright (c) 2009-2010 Bart Cerneels <bart.cerneels@kde.org>                        *
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

#define DEBUG_PREFIX "PlaylistBrowserModel"

#include "PlaylistBrowserModel.h"

#include "AmarokMimeData.h"
#include "playlistmanager/PlaylistManager.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModel.h"
#include "core/support/Debug.h"
#include "widgets/PrettyTreeRoles.h"

#include <QIcon>

#include <QAction>

#include <algorithm>

using namespace PlaylistBrowserNS;

// to be used with qSort.
static bool
lessThanPlaylistTitles( const Playlists::PlaylistPtr &lhs, const Playlists::PlaylistPtr &rhs )
{
    return lhs->prettyName().toLower() < rhs->prettyName().toLower();
}

PlaylistBrowserModel::PlaylistBrowserModel( int playlistCategory )
    : m_playlistCategory( playlistCategory )
{
    connect( The::playlistManager(), &PlaylistManager::updated, this, &PlaylistBrowserModel::slotUpdate );

    connect( The::playlistManager(), &PlaylistManager::playlistAdded,
             this, &PlaylistBrowserModel::slotPlaylistAdded );
    connect( The::playlistManager(), &PlaylistManager::playlistRemoved,
             this, &PlaylistBrowserModel::slotPlaylistRemoved );
    connect( The::playlistManager(), &PlaylistManager::playlistUpdated,
             this, &PlaylistBrowserModel::slotPlaylistUpdated );

    connect( The::playlistManager(), &PlaylistManager::renamePlaylist,
             this, &PlaylistBrowserModel::slotRenamePlaylist );

    m_playlists = loadPlaylists();
}

QVariant
PlaylistBrowserModel::data( const QModelIndex &index, int role ) const
{
    int row = REMOVE_TRACK_MASK(index.internalId());
    Playlists::PlaylistPtr playlist = m_playlists.value( row );

    QString name;
    QIcon icon;
    int playlistCount = 0;
    QList<QAction *> providerActions;
    QList<Playlists::PlaylistProvider *> providers =
        The::playlistManager()->getProvidersForPlaylist( playlist );
    Playlists::PlaylistProvider *provider = providers.count() == 1 ? providers.first() : nullptr;
    Meta::TrackPtr track;

    switch( index.column() )
    {
        case PlaylistBrowserModel::PlaylistItemColumn: //playlist or track data
        {
            if( IS_TRACK(index) )
            {
                track = playlist->tracks()[index.row()];
                name = track->prettyName();
                icon = QIcon::fromTheme( QStringLiteral("amarok_track") );
            }
            else
            {
                name = playlist->prettyName();
                icon = QIcon::fromTheme( QStringLiteral("amarok_playlist") );
            }
            break;
        }
        case PlaylistBrowserModel::LabelColumn: //group
        {
            if( !playlist->groups().isEmpty() )
            {
                name = playlist->groups().first();
                icon = QIcon::fromTheme( QStringLiteral("folder") );
            }
            break;
        }

        case PlaylistBrowserModel::ProviderColumn: //source
        {
            if( providers.count() > 1 )
            {
                QVariantList nameData;
                QVariantList iconData;
                QVariantList playlistCountData;
                QVariantList providerActionsData;
                for( Playlists::PlaylistProvider *provider : providers )
                {
                    name = provider->prettyName();
                    nameData << name;
                    icon = provider->icon();
                    iconData << QVariant( icon );
                    playlistCount = provider->playlists().count();
                    if( playlistCount >= 0 )
                        playlistCountData << i18ncp(
                                "number of playlists from one source",
                                "One Playlist", "%1 playlists",
                                playlistCount );
                    else
                        playlistCountData << i18nc(
                                "normally number of playlists, but they are still loading",
                                "Loading..." );
                    providerActions << provider->providerActions();
                    providerActionsData << QVariant::fromValue( providerActions );
                }
                switch( role )
                {
                case Qt::DisplayRole:
                case Qt::EditRole:
                case Qt::ToolTipRole:
                    return nameData;
                case Qt::DecorationRole:
                    return iconData;
                case PrettyTreeRoles::ByLineRole:
                    return playlistCountData;
                case PrettyTreeRoles::DecoratorRoleCount:
                    return providerActions.count();
                case PrettyTreeRoles::DecoratorRole:
                    return providerActionsData;
                }
            }
            else if( provider )
            {
                name = provider->prettyName();
                icon = provider->icon();
                playlistCount = provider->playlistCount();
                providerActions << provider->providerActions();
            }

            break;
        }

        default: break;
    }


    switch( role )
    {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return name;
        case Qt::DecorationRole:
            return QVariant( icon );
        case PrettyTreeRoles::ByLineRole:
            if( IS_TRACK(index) )
                return QVariant();
            else
                return i18ncp( "number of playlists from one source", "One playlist",
                               "%1 playlists", playlistCount );
        case PlaylistBrowserModel::ProviderRole:
            return provider ? QVariant::fromValue( provider ) : QVariant();
        case PlaylistBrowserModel::PlaylistRole:
            return playlist ? QVariant::fromValue( playlist ) : QVariant();
        case PlaylistBrowserModel::TrackRole:
            return track ? QVariant::fromValue( track ) : QVariant();

        default:
            return QVariant();
    }
}

bool
PlaylistBrowserModel::setData( const QModelIndex &idx, const QVariant &value, int role )
{

    if( !idx.isValid() )
        return false;

    switch( idx.column() )
    {
        case ProviderColumn:
        {
            if( role == Qt::DisplayRole || role == Qt::EditRole )
            {
                Playlists::PlaylistProvider *provider = getProviderByName( value.toString() );
                if( !provider )
                    return false;

                if( IS_TRACK( idx ) )
                {
                    Meta::TrackPtr track = trackFromIndex( idx );
                    if( !track )
                        return false;
                    debug() << QStringLiteral( "Copy track \"%1\" to \"%2\"." )
                            .arg( track->prettyName(),  provider->prettyName() );
    //                return !provider->addTrack( track ).isNull();
                    provider->addTrack( track ); //ignore result since UmsPodcastProvider returns NULL
                    return true;
                }
                else
                {
                    Playlists::PlaylistPtr playlist = playlistFromIndex( idx );
                    if( !playlist || ( playlist->provider() == provider ) )
                        return false;

                    for( Playlists::PlaylistPtr tempPlaylist : provider->playlists() )
                    {
                        if ( tempPlaylist->name() == playlist->name() )
                            return false;
                    }

                    debug() << QStringLiteral( "Copy playlist \"%1\" to \"%2\"." )
                            .arg( playlist->prettyName(), provider->prettyName() );

                    return !provider->addPlaylist( playlist ).isNull();
                }
            }

            //return true even for the data we didn't handle to get QAbstractItemModel::setItemData to work
            //TODO: implement setItemData()
            return true;
        }
        case LabelColumn:
        {
            debug() << "changing group of item " << idx.internalId() << " to " << value.toString();
            Playlists::PlaylistPtr item = m_playlists.value( idx.internalId() );
            item->setGroups( value.toStringList() );

            return true;
        }
    }

    return false;
}

QModelIndex
PlaylistBrowserModel::index( int row, int column, const QModelIndex &parent) const
{
    if( !parent.isValid() )
    {
        if( row >= 0 && row < m_playlists.count() )
            return createIndex( row, column, row );
    }
    else //if it has a parent it is a track
    {
        //but check if the playlist indeed has that track
        Playlists::PlaylistPtr playlist = m_playlists.value( parent.row() );
        if( row < playlist->tracks().count() )
            return createIndex( row, column, SET_TRACK_MASK(parent.row()) );
    }

    return QModelIndex();
}

QModelIndex
PlaylistBrowserModel::parent( const QModelIndex &index ) const
{
    if( IS_TRACK(index) )
    {
        int row = REMOVE_TRACK_MASK(index.internalId());
        return this->index( row, index.column(), QModelIndex() );
    }

    return QModelIndex();
}

bool
PlaylistBrowserModel::hasChildren( const QModelIndex &parent ) const
{
    if( parent.column() > 0 )
        return false;
    if( !parent.isValid() )
    {
        return !m_playlists.isEmpty();
    }
    else if( !IS_TRACK(parent) )
    {
        Playlists::PlaylistPtr playlist = m_playlists.value( parent.internalId() );
        return playlist->trackCount() != 0; //-1 might mean there are tracks, but not yet loaded.
    }

    return false;
}

int
PlaylistBrowserModel::rowCount( const QModelIndex &parent ) const
{
    if( parent.column() > 0 )
        return 0;

    if( !parent.isValid() )
    {
        return m_playlists.count();
    }
    else if( !IS_TRACK(parent) )
    {
        Playlists::PlaylistPtr playlist = m_playlists.value( parent.internalId() );
        return playlist->trackCount();
    }

    return 0;
}

int
PlaylistBrowserModel::columnCount( const QModelIndex &parent ) const
{
    if( !parent.isValid() ) //for playlists (children of root)
        return 3; //name, group and provider

    //for tracks
    return 1; //only name
}

bool
PlaylistBrowserModel::canFetchMore( const QModelIndex &parent ) const
{
    if( parent.column() > 0 )
        return false;

    if( !parent.isValid() )
    {
        return false;
    }
    else if( !IS_TRACK(parent) )
    {
        Playlists::PlaylistPtr playlist = m_playlists.value( parent.internalId() );
        //TODO: implement incremental loading of tracks by checking for ==
        if( playlist->trackCount() != playlist->tracks().count() )
            return true; //tracks still need to be loaded.
    }

    return false;
}

void
PlaylistBrowserModel::fetchMore ( const QModelIndex &parent )
{
    if( parent.column() > 0 )
        return;

    //TODO: load playlists dynamically from provider
    if( !parent.isValid() )
        return;

    if( !IS_TRACK(parent) )
    {
        Playlists::PlaylistPtr playlist = m_playlists.value( parent.internalId() );
         // TODO: following doesn't seem to be needed, PlaylistBrowserModel seems to be able to cope with async track loading fine
        playlist->makeLoadingSync();
        playlist->triggerTrackLoad();
    }
}

Qt::ItemFlags
PlaylistBrowserModel::flags( const QModelIndex &idx ) const
{
    //Both providers and groups can be empty. QtGroupingProxy makes empty groups from the data in
    //the rootnode (here an invalid QModelIndex).
    //TODO: editable only if provider is writable.
    if( idx.column() == PlaylistBrowserModel::ProviderColumn )
        return Qt::ItemIsEnabled | Qt::ItemIsEditable;

    if( idx.column() == PlaylistBrowserModel::LabelColumn )
        return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDropEnabled;

    if( !idx.isValid() )
        return Qt::ItemIsDropEnabled;

    if( IS_TRACK(idx) )
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;

    //item is a playlist
    return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled |
           Qt::ItemIsDropEnabled;
}

QVariant
PlaylistBrowserModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
    {
        switch( section )
        {
            case PlaylistBrowserModel::PlaylistItemColumn: return i18n("Name");
            case PlaylistBrowserModel::LabelColumn: return i18n("Group");
            case PlaylistBrowserModel::ProviderColumn: return i18n("Source");
            default: return QVariant();
        }
    }

    return QVariant();
}

QStringList
PlaylistBrowserModel::mimeTypes() const
{
    QStringList ret;
    ret << AmarokMimeData::PLAYLIST_MIME;
    ret << AmarokMimeData::TRACK_MIME;
    return ret;
}

QMimeData*
PlaylistBrowserModel::mimeData( const QModelIndexList &indices ) const
{
    AmarokMimeData* mime = new AmarokMimeData();

    Playlists::PlaylistList playlists;
    Meta::TrackList tracks;

    for( const QModelIndex &index : indices )
    {
        if( IS_TRACK(index) )
            tracks << trackFromIndex( index );
        else
            playlists << m_playlists.value( index.internalId() );
    }

    mime->setPlaylists( playlists );
    mime->setTracks( tracks );

    return mime;
}

bool
PlaylistBrowserModel::dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column,
                                 const QModelIndex &parent )
{
    DEBUG_BLOCK
    debug() << "Dropped on" << parent << "row" << row << "column" << column << "action" << action;
    if( action == Qt::IgnoreAction )
        return true;

    //drop on track is not possible
    if( IS_TRACK(parent) )
        return false;

    const AmarokMimeData* amarokMime = dynamic_cast<const AmarokMimeData*>( data );
    if( !amarokMime )
        return false;

    if( data->hasFormat( AmarokMimeData::PLAYLIST_MIME ) )
    {
        // TODO: is this ever called????
        Playlists::PlaylistList playlists = amarokMime->playlists();

        for( Playlists::PlaylistPtr playlist : playlists )
        {
            if( !m_playlists.contains( playlist ) )
                debug() << "Unknown playlist dragged in: " << playlist->prettyName();
        }

        return true;
    }
    else if( data->hasFormat( AmarokMimeData::TRACK_MIME ) )
    {
        Meta::TrackList tracks = amarokMime->tracks();
        if( !parent.isValid() && row == -1 && column == -1 )
        {
            debug() << "Dropped tracks on empty area: create new playlist in a default provider";
            The::playlistManager()->save( tracks, Amarok::generatePlaylistName( tracks ) );
            return true;
        }
        else if( !parent.isValid() )
        {
            warning() << "Dropped tracks between root items, this is not supported!";
            return false;
        }
        else
        {
            debug() << "Dropped tracks on " << parent << " at row: " << row;

            Playlists::PlaylistPtr playlist = playlistFromIndex( parent );
            if( !playlist )
                return false;

            for( Meta::TrackPtr track : tracks )
                playlist->addTrack( track, ( row >= 0 ) ? row++ : -1 ); // increment only if positive

            return true;
        }
    }

    return false;
}

void
PlaylistBrowserModel::metadataChanged( const Playlists::PlaylistPtr &playlist )
{
    int indexNumber = m_playlists.indexOf( playlist );
    if( indexNumber == -1 )
    {
        error() << "This playlist is not in the list of this model.";
        return;
    }
    QModelIndex playlistIdx = index( indexNumber, 0 );
    Q_EMIT dataChanged( playlistIdx, playlistIdx );
}

void
PlaylistBrowserModel::trackAdded(const Playlists::PlaylistPtr &playlist, const Meta::TrackPtr &track,
                                          int position )
{
    Q_UNUSED( track );
    int indexNumber = m_playlists.indexOf( playlist );
    if( indexNumber == -1 )
    {
        error() << "This playlist is not in the list of this model.";
        return;
    }
    QModelIndex playlistIdx = index( indexNumber, 0, QModelIndex() );
    beginInsertRows( playlistIdx, position, position );
    endInsertRows();
}

void
PlaylistBrowserModel::trackRemoved(const Playlists::PlaylistPtr &playlist, int position )
{
    int indexNumber = m_playlists.indexOf( playlist );
    if( indexNumber == -1 )
    {
        error() << "This playlist is not in the list of this model.";
        return;
    }
    QModelIndex playlistIdx = index( indexNumber, 0, QModelIndex() );
    beginRemoveRows( playlistIdx, position, position );
    endRemoveRows();
}

void
PlaylistBrowserModel::slotRenamePlaylist( Playlists::PlaylistPtr playlist )
{
    if( !playlist->provider() || playlist->provider()->category() != m_playlistCategory )
        return;

    int row = 0;
    for( Playlists::PlaylistPtr p : m_playlists )
    {
        if( p == playlist )
        {
            Q_EMIT renameIndex( index( row, 0 ) );
            break;
        }
        row++;
    }
}

void
PlaylistBrowserModel::slotUpdate( int category )
{
    if( category != m_playlistCategory )
        return;

    beginResetModel();

    for( Playlists::PlaylistPtr playlist : m_playlists )
        unsubscribeFrom( playlist );

    m_playlists.clear();
    m_playlists = loadPlaylists();

    endResetModel();
}

Playlists::PlaylistList
PlaylistBrowserModel::loadPlaylists()
{
    Playlists::PlaylistList playlists =
            The::playlistManager()->playlistsOfCategory( m_playlistCategory );
    QListIterator<Playlists::PlaylistPtr> i( playlists );

    debug() << playlists.count() << " playlists for category " << m_playlistCategory;

    while( i.hasNext() )
    {
        Playlists::PlaylistPtr playlist = i.next();
        subscribeTo( playlist );
    }

    std::sort( playlists.begin(), playlists.end(), lessThanPlaylistTitles );

    return playlists;
}

void
PlaylistBrowserModel::slotPlaylistAdded( Playlists::PlaylistPtr playlist, int category )
{
    //ignore playlists of a different category
    if( category != m_playlistCategory )
        return;

    subscribeTo( playlist );
    int i;
    for( i = 0; i < m_playlists.count(); i++ )
    {
        if( lessThanPlaylistTitles( playlist, m_playlists[i] ) )
            break;
    }

    beginInsertRows( QModelIndex(), i, i );
    m_playlists.insert( i, playlist );
    endInsertRows();
}

void
PlaylistBrowserModel::slotPlaylistRemoved( Playlists::PlaylistPtr playlist, int category )
{
    if( category != m_playlistCategory )
        return;

    int position = m_playlists.indexOf( playlist );
    if( position == -1 )
    {
        error() << "signal received for removed playlist not in m_playlists";
        return;
    }

    beginRemoveRows( QModelIndex(), position, position );
    m_playlists.removeAt( position );
    endRemoveRows();
}

void
PlaylistBrowserModel::slotPlaylistUpdated( Playlists::PlaylistPtr playlist, int category )
{
    if( category != m_playlistCategory )
        return;

    int position = m_playlists.indexOf( playlist );
    if( position == -1 )
    {
        error() << "signal received for updated playlist not in m_playlists";
        return;
    }

    //TODO: this should work by signaling a change in the model data, but QtGroupingProxy doesn't
    //work like that ATM
//    const QModelIndex &idx = index( position, 0 );
//    Q_EMIT dataChanged( idx, idx );

    //HACK: remove and readd so QtGroupingProxy can put it in the correct groups.
    beginRemoveRows( QModelIndex(), position, position );
    endRemoveRows();

    beginInsertRows( QModelIndex(), position, position );
    endInsertRows();
}

Meta::TrackList
PlaylistBrowserModel::tracksFromIndexes( const QModelIndexList &list ) const
{
    Meta::TrackList tracks;
    for( const QModelIndex &index : list )
    {
        if( IS_TRACK(index) )
            tracks << trackFromIndex( index );
        else if( Playlists::PlaylistPtr playlist = playlistFromIndex( index ) )
        {
            playlist->makeLoadingSync();
            //first trigger a load of the tracks or we'll end up with an empty list
            playlist->triggerTrackLoad();
            tracks << playlist->tracks();
        }
    }
    return tracks;
}

Meta::TrackPtr
PlaylistBrowserModel::trackFromIndex( const QModelIndex &idx ) const
{
    if( !idx.isValid() || !IS_TRACK(idx) )
        return Meta::TrackPtr();

    int playlistRow = REMOVE_TRACK_MASK(idx.internalId());
    if( playlistRow >= m_playlists.count() )
        return Meta::TrackPtr();

    Playlists::PlaylistPtr playlist = m_playlists.value( playlistRow );
    if( playlist.isNull() || playlist->tracks().count() <= idx.row() )
        return Meta::TrackPtr();

    return playlist->tracks()[idx.row()];
}

Playlists::PlaylistPtr
PlaylistBrowserModel::playlistFromIndex( const QModelIndex &index ) const
{
    if( !index.isValid() )
        return Playlists::PlaylistPtr();

    return m_playlists.value( index.internalId() );
}

Playlists::PlaylistProvider *
PlaylistBrowserModel::providerForIndex( const QModelIndex &idx ) const
{
    if( !idx.isValid() )
        return nullptr;

    int playlistRow;
    if( IS_TRACK( idx ) )
        playlistRow = REMOVE_TRACK_MASK( idx.internalId() );
    else
        playlistRow = idx.row();

    if( playlistRow >= m_playlists.count() )
        return nullptr;

    return m_playlists.at( playlistRow )->provider();
}

Playlists::PlaylistProvider *
PlaylistBrowserModel::getProviderByName( const QString &name )
{
    QList<Playlists::PlaylistProvider *> providers =
            The::playlistManager()->providersForCategory( m_playlistCategory );
    for( Playlists::PlaylistProvider *provider : providers )
    {
        if( provider->prettyName() == name )
            return provider;
    }
    return nullptr;
}
