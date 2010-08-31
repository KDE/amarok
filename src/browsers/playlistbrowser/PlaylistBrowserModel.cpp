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
#include "playlist/PlaylistModelStack.h"
#include "core/support/Debug.h"

#include <KIcon>

#include <QAction>

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
    //common, unconditional actions
    m_appendAction = new QAction( KIcon( "media-track-add-amarok" ), i18n( "&Add to Playlist" ),
                                  this );
    m_appendAction->setProperty( "popupdropper_svg_id", "append" );
    connect( m_appendAction, SIGNAL( triggered() ), this, SLOT( slotAppend() ) );

    m_loadAction = new QAction( KIcon( "folder-open" ),
                                i18nc( "Replace the currently loaded tracks with these",
                                       "&Replace Playlist" ),
                                this );
    m_loadAction->setProperty( "popupdropper_svg_id", "load" );
    connect( m_loadAction, SIGNAL( triggered() ), this, SLOT( slotLoad() ) );

    connect( The::playlistManager(), SIGNAL(updated()), SLOT(slotUpdate()) );
    connect( The::playlistManager(), SIGNAL( providerRemoved( Playlists::PlaylistProvider*, int ) ),
             SLOT( slotUpdate() ) );

    connect( The::playlistManager(), SIGNAL(renamePlaylist( Playlists::PlaylistPtr )),
             SLOT(slotRenamePlaylist( Playlists::PlaylistPtr )) );

    m_playlists = loadPlaylists();
}

QVariant
PlaylistBrowserModel::data( const QModelIndex &index, int role ) const
{
    //Special negative index to support empty provider groups (PlaylistsByProviderProxy)
    if( index.row() == -1 && index.column() == PlaylistBrowserModel::ProviderColumn )
    {
        QVariantList displayList;
        QVariantList iconList;
        QVariantList playlistCountList;
        QVariantList providerActionsCountList;
        QVariantList providerActionsList;
        QVariantList providerByLineList;

        //get data from empty providers
        PlaylistProviderList providerList =
                The::playlistManager()->providersForCategory( m_playlistCategory );
        foreach( Playlists::PlaylistProvider *provider, providerList )
        {
            if( provider && ( provider->playlistCount() > 0 || provider->playlists().count() > 0 ) )
                continue;

            displayList << provider->prettyName();
            iconList << provider->icon();
            playlistCountList << provider->playlists().count();
            providerActionsCountList << provider->providerActions().count();
            providerActionsList <<  QVariant::fromValue( provider->providerActions() );
            providerByLineList << i18ncp( "number of playlists from one source",
                                          "One Playlist", "%1 playlists",
                                          provider->playlists().count() );
        }

        switch( role )
        {
            case Qt::DisplayRole:
            case DescriptionRole:
            case Qt::ToolTipRole: return displayList;
            case Qt::DecorationRole: return iconList;
            case PlaylistBrowserModel::ActionCountRole: return providerActionsCountList;
            case PlaylistBrowserModel::ActionRole: return providerActionsList;
            case PlaylistBrowserModel::ByLineRole: return providerByLineList;
            case Qt::EditRole: return QVariant();
        }
    }

    if( !index.isValid() )
        return QVariant();

    int row = REMOVE_TRACK_MASK(index.internalId());
    Playlists::PlaylistPtr playlist = m_playlists.value( row );

    QVariant food = QVariant();
    QString name;
    QString description;
    KIcon icon;
    QStringList groups;
    int playlistCount = 0;
    QList<QAction *> providerActions;

    if( IS_TRACK(index) )
    {
        Meta::TrackPtr track = playlist->tracks()[index.row()];
        food = QVariant::fromValue( track );
        name = track->prettyName();
        icon = KIcon( "amarok_track" );
    }
    else
    {
        switch( index.column() )
        {
            case PlaylistBrowserModel::PlaylistColumn: //playlist
            {
                food = QVariant::fromValue( playlist );
                name = playlist->prettyName();
                description = playlist->description();
                icon = KIcon( "amarok_playlist" );
                break;
            }
            case PlaylistBrowserModel::LabelColumn: //group
            {
                if( !playlist->groups().isEmpty() )
                {
                    name = playlist->groups().first();
                    icon = KIcon( "folder" );
                }
                break;
            }

            case PlaylistBrowserModel::ProviderColumn: //source
            {
                QList<Playlists::PlaylistProvider *> providers =
                        The::playlistManager()->getProvidersForPlaylist( playlist );

                if( providers.count() > 1 )
                {
                    QVariantList nameData;
                    QVariantList descriptionData;
                    QVariantList iconData;
                    QVariantList playlistCountData;
                    QVariantList providerActionsData;
                    foreach( Playlists::PlaylistProvider *provider, providers )
                    {
                        name = description = provider->prettyName();
                        nameData << name;
                        descriptionData << description;
                        icon = provider->icon();
                        iconData << QVariant( icon );
                        playlistCount = provider->playlists().count();
                        playlistCountData << i18ncp( "number of playlists from one source",
                                                     "One Playlist", "%1 playlists",
                                                     playlistCount );
                        providerActions << provider->providerActions();
                        providerActionsData << QVariant::fromValue( providerActions );
                    }
                    switch( role )
                    {
                    case Qt::DisplayRole:
                    case Qt::EditRole: return nameData;
                    case DescriptionRole:
                    case Qt::ToolTipRole: return descriptionData;
                    case Qt::DecorationRole: return iconData;
                    case PlaylistBrowserModel::ByLineRole:
                        return playlistCountData;
                    case PlaylistBrowserModel::ActionRole:
                        return providerActionsData;
                    }
                }
                else if( providers.count() )
                {
                    Playlists::PlaylistProvider *provider = providers.first();
                    name = description = provider->prettyName();
                    icon = provider->icon();
                    playlistCount = provider->playlists().count();
                    providerActions << provider->providerActions();
                }

                break;
            }

            default: return QVariant();
        }
    }


    switch( role )
    {
        case 0xf00d: return food;
        case Qt::DisplayRole:
        case Qt::EditRole: return name;
        case DescriptionRole:
        case Qt::ToolTipRole: return description;
        case Qt::DecorationRole: return QVariant( icon );
        case PlaylistBrowserModel::ByLineRole:
            return i18ncp( "number of playlists from one source",
                           "One Playlist", "%1 playlists",
                           playlistCount );
        case PlaylistBrowserModel::ActionRole:
            return QVariant::fromValue( index.column() == PlaylistBrowserModel::ProviderColumn ?
                    providerActions : actionsFor( index ) );
        case PlaylistBrowserModel::ActionCountRole:
            return QVariant( index.column() == PlaylistBrowserModel::ProviderColumn ?
                    providerActions.count() : actionsFor( index ).count() );

        default: return QVariant();
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
            if( role == Qt::DisplayRole )
            {
                Playlists::PlaylistProvider *provider = getProviderByName( value.toString() );
                if( !provider )
                    return false;

                if( IS_TRACK( idx ) )
                {
                    Meta::TrackPtr track = trackFromIndex( idx );
                    if( !track )
                        return false;
                    debug() << QString( "Copy track \"%1\" to \"%2\"." )
                            .arg( track->prettyName() ).arg( provider->prettyName() );
    //                return !provider->addTrack( track ).isNull();
                    provider->addTrack( track ); //ignore result since UmsPodcastProvider returns NULL
                    return true;
                }
                else
                {
                    Playlists::PlaylistPtr playlist = playlistFromIndex( idx );
                    if( !playlist )
                        return false;
                    debug() << QString( "Copy playlist \"%1\" to \"%2\"." )
                            .arg( playlist->prettyName() ).arg( provider->prettyName() );
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
        //there are valid indexes available with row == -1 for empty groups and providers
        if( row == -1 && column >= 0 )
            return createIndex( row, column, row );

        if( row < m_playlists.count() )
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
        return playlist->tracks().count();
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
            case PlaylistBrowserModel::PlaylistColumn: return i18n("Name");
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

    foreach( const QModelIndex &index, indices )
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
    Q_UNUSED( column );
    if( action == Qt::IgnoreAction )
        return true;

    const AmarokMimeData* amarokMime = dynamic_cast<const AmarokMimeData*>( data );
    if( !amarokMime )
        return false;

    if( data->hasFormat( AmarokMimeData::PLAYLIST_MIME ) )
    {
        Playlists::PlaylistList playlists = amarokMime->playlists();

        foreach( Playlists::PlaylistPtr playlist, playlists )
        {
            if( !m_playlists.contains( playlist ) )
            {
                debug() << "unknown playlist dragged in: " << playlist->prettyName();
                debug() << "TODO: start synchronization";
            }
        }

        return true;
    }
    else if( data->hasFormat( AmarokMimeData::TRACK_MIME ) )
    {
        debug() << "Dropped track on " << parent << " at row: " << row;

        Playlists::PlaylistPtr playlist = playlistFromIndex( parent );
        if( !playlist )
            return false;

        Meta::TrackList tracks = amarokMime->tracks();
        bool allAdded = true;
        foreach( Meta::TrackPtr track, tracks )
            /*allAdded = */playlist->addTrack( track, row )/* ? allAdded : false*/;

        return allAdded;
    }

    return false;
}

void
PlaylistBrowserModel::trackAdded( Playlists::PlaylistPtr playlist, Meta::TrackPtr track,
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
PlaylistBrowserModel::trackRemoved( Playlists::PlaylistPtr playlist, int position )
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
    //search index of this Playlist
    // HACK: matches first to match same name, but there could be
    // several playlists with the same name
    int row = -1;
    foreach( const Playlists::PlaylistPtr p, m_playlists )
    {
        row++;
        if( p->name() == playlist->name() )
            break;
    }
    if( row == -1 )
        return;

    QModelIndex idx = index( row, 0, QModelIndex() );
    emit( renameIndex( idx ) );
}

void
PlaylistBrowserModel::slotUpdate()
{
    emit layoutAboutToBeChanged();

    foreach( Playlists::PlaylistPtr playlist, m_playlists )
        unsubscribeFrom( playlist );

    m_playlists.clear();
    m_playlists = loadPlaylists();

    emit layoutChanged();
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

    qSort( playlists.begin(), playlists.end(), lessThanPlaylistTitles );

    return playlists;
}

void
PlaylistBrowserModel::slotLoad()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    QModelIndexList indexes = action->data().value<QModelIndexList>();

    Meta::TrackList tracks = tracksFromIndexes( indexes );
    if( !tracks.isEmpty() )
        The::playlistController()->insertOptioned( tracks, Playlist::LoadAndPlay );
}

void
PlaylistBrowserModel::slotAppend()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    QModelIndexList indexes = action->data().value<QModelIndexList>();

    Meta::TrackList tracks = tracksFromIndexes( indexes );
    if( !tracks.isEmpty() )
        The::playlistController()->insertOptioned( tracks, Playlist::AppendAndPlay );
}

Meta::TrackList
PlaylistBrowserModel::tracksFromIndexes( const QModelIndexList &list ) const
{
    Meta::TrackList tracks;
    foreach( const QModelIndex &index, list )
    {
        if( IS_TRACK(index) )
            tracks << trackFromIndex( index );
        else if( Playlists::PlaylistPtr playlist = playlistFromIndex( index ) )
            tracks << playlist->tracks();
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
        return 0;

    int playlistRow;
    if( IS_TRACK( idx ) )
        playlistRow = REMOVE_TRACK_MASK( idx.internalId() );
    else
        playlistRow = idx.row();

    if( playlistRow >= m_playlists.count() )
        return 0;

    return m_playlists.at( playlistRow )->provider();
}

QActionList
PlaylistBrowserModel::actionsFor( const QModelIndex &idx ) const
{
    //wheter we use the list from m_appendAction of m_loadAction does not matter they are the same
    QModelIndexList actionList = m_appendAction->data().value<QModelIndexList>();

    actionList << idx;
    QVariant value = QVariant::fromValue( actionList );
    m_appendAction->setData( value );
    m_loadAction->setData( value );

    QActionList actions;
    actions << m_appendAction << m_loadAction;

    if( !IS_TRACK(idx) )
    {
        Playlists::PlaylistPtr playlist = m_playlists.value( idx.internalId() );
        actions << playlist->actions();
    }
    else
    {
        Playlists::PlaylistPtr playlist = m_playlists.value( idx.parent().internalId() );
        actions << playlist->trackActions( idx.row() );
    }

    return actions;
}

Playlists::PlaylistProvider *
PlaylistBrowserModel::getProviderByName( const QString &name )
{
    QList<Playlists::PlaylistProvider *> providers =
            The::playlistManager()->providersForCategory( m_playlistCategory );
    foreach( Playlists::PlaylistProvider *provider, providers )
    {
        if( provider->prettyName() == name )
            return provider;
    }
    return 0;
}
