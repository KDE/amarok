/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Ian Monroe <imonroe@kde.org>                                      *
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

#include "UserPlaylistModel.h"
#include "playlistmanager/PlaylistManager.h"
#include "core/playlists/PlaylistProvider.h"

#include "AmarokMimeData.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "SvgHandler.h"

#include <KIcon>

#include <QAbstractListModel>

#include <typeinfo>

//Playlist & Track index differentiator macros
//QModelIndex::intenalId() is a qint64 to support 64-bit pointers in a union with the ID
#define TRACK_MASK (0x1<<31)
#define IS_TRACK(x) ((x.internalId()) & (TRACK_MASK))?true:false
#define SET_TRACK_MASK(x) ((x) | (TRACK_MASK))
#define REMOVE_TRACK_MASK(x) ((x) & ~(TRACK_MASK))

namespace The
{
    PlaylistBrowserNS::UserModel* userPlaylistModel()
    {
        return PlaylistBrowserNS::UserModel::instance();
    }
}

PlaylistBrowserNS::UserModel *PlaylistBrowserNS::UserModel::s_instance = 0;

PlaylistBrowserNS::UserModel *PlaylistBrowserNS::UserModel::instance()
{
    if( s_instance == 0 )
        s_instance = new UserModel();

    return s_instance;
}

void
PlaylistBrowserNS::UserModel::destroy()
{
    if( s_instance )
    {
        delete s_instance;
        s_instance = 0;
    }
}

PlaylistBrowserNS::UserModel::UserModel()
    : MetaPlaylistModel()
    , m_appendAction( 0 )
    , m_loadAction( 0 )
{
    s_instance = this;
    loadPlaylists();

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
    connect( The::playlistManager(), SIGNAL(renamePlaylist( Playlists::PlaylistPtr )),
             SLOT(slotRenamePlaylist( Playlists::PlaylistPtr )) );
}

PlaylistBrowserNS::UserModel::~UserModel()
{
}

void
PlaylistBrowserNS::UserModel::slotUpdate()
{
    loadPlaylists();

    emit layoutAboutToBeChanged();
    emit layoutChanged();
}

void
PlaylistBrowserNS::UserModel::slotRenamePlaylist( Playlists::PlaylistPtr playlist )
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

    QModelIndex index = this->index( row, 0, QModelIndex() );
    emit( renameIndex( index ) );
}

void
PlaylistBrowserNS::UserModel::loadPlaylists()
{
    QList<Playlists::PlaylistPtr> playlists =
            The::playlistManager()->playlistsOfCategory( Playlists::UserPlaylist );
    QListIterator<Playlists::PlaylistPtr> i( playlists );
    m_playlists.clear();
    while( i.hasNext() )
    {
        Playlists::PlaylistPtr playlist = Playlists::PlaylistPtr::dynamicCast( i.next() );
        if( playlist.isNull() )
        {
          error() << "Playlist was NULL!";
          continue;
        }
        m_playlists << playlist;
        playlist->subscribe( this );
    }
}

QVariant
PlaylistBrowserNS::UserModel::data( const QModelIndex &index, int role ) const
{
    if( index.row() == -1 )
    {
        if( index.column() == MetaPlaylistModel::ProviderColumn )
        {
            QVariantList displayList;
            QVariantList iconList;
            QVariantList playlistCountList;
            QVariantList providerActionsCountList;
            QVariantList providerActionsList;
            QVariantList providerByLineList;

            //get data from empty providers
            PlaylistProviderList providerList =
                    The::playlistManager()->providersForCategory( Playlists::UserPlaylist );
            foreach( Playlists::PlaylistProvider *provider, providerList )
            {
                if( provider->playlistCount() > 0 || provider->playlists().count() > 0 )
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
                case MetaPlaylistModel::ActionCountRole: return providerActionsCountList;
                case MetaPlaylistModel::ActionRole: return providerActionsList;
                case MetaPlaylistModel::ByLineRole: return providerByLineList;
                case Qt::EditRole: return QVariant();
            }
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
            case MetaPlaylistModel::PlaylistColumn: //playlist
            {
                food = QVariant::fromValue( playlist );
                name = playlist->prettyName();
                description = playlist->description();
                icon = KIcon( "amarok_playlist" );
                groups = playlist->groups();
                break;
            }
            case MetaPlaylistModel::LabelColumn: //group
            {
                if( !playlist->groups().isEmpty() )
                {
                    name= playlist->groups().first();
                    icon = KIcon( "folder" );
                }
                break;
            }

            case MetaPlaylistModel::ProviderColumn: //source
            {
                Playlists::PlaylistProvider *provider =
                        The::playlistManager()->getProviderForPlaylist( playlist );
                //if provider is 0 there is something seriously wrong.
                if( provider )
                {
                    name = description = provider->prettyName();
                    icon = provider->icon();
                    playlistCount = provider->playlists().count();
                    providerActions = provider->providerActions();
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
        case MetaPlaylistModel::ByLineRole:
            return i18ncp( "number of playlists from one source",
                           "One Playlist", "%1 playlists",
                           playlistCount );
        case MetaPlaylistModel::ActionCountRole: return providerActions.count();
        case MetaPlaylistModel::ActionRole:
            return QVariant::fromValue( index.column() == MetaPlaylistModel::ProviderColumn ?
                        providerActions : actionsFor( index ) );

        default: return QVariant();
    }
}

QModelIndex
PlaylistBrowserNS::UserModel::index( int row, int column, const QModelIndex &parent ) const
{
    //there are valid indexes available with row == -1 for empty groups and providers
    if( !parent.isValid() )
    {
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
PlaylistBrowserNS::UserModel::parent( const QModelIndex &index ) const
{
    if( IS_TRACK(index) )
    {
        int row = REMOVE_TRACK_MASK(index.internalId());
        return this->index( row, index.column(), QModelIndex() );
    }

    return QModelIndex();
}

int
PlaylistBrowserNS::UserModel::rowCount( const QModelIndex &parent ) const
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
PlaylistBrowserNS::UserModel::columnCount( const QModelIndex &parent ) const
{
    if( !parent.isValid() ) //for playlists (children of root)
        return 3; //name, group and provider

    //for tracks
    return 1; //only name
}

Qt::ItemFlags
PlaylistBrowserNS::UserModel::flags( const QModelIndex &index ) const
{
    //Both providers and groups can be empty. QtGroupingProxy makes empty groups from the data in
    //the rootnode (here an invalid QModelIndex).
    //TODO: accept drops and allow drags only if provider is writable.
    if( index.column() == MetaPlaylistModel::ProviderColumn )
        return Qt::ItemIsEnabled;

    if( index.column() == MetaPlaylistModel::LabelColumn )
        return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDropEnabled;

    if( !index.isValid() )
        return Qt::ItemIsDropEnabled;

    if( IS_TRACK(index) )
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;

    //item is a playlist
    return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled |
           Qt::ItemIsDropEnabled;
}

QVariant
PlaylistBrowserNS::UserModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch( section )
        {
            case MetaPlaylistModel::PlaylistColumn: return i18n("Name");
            case MetaPlaylistModel::LabelColumn: return i18n("Group");
            case MetaPlaylistModel::ProviderColumn: return i18n("Source");
            default: return QVariant();
        }
    }

    return QVariant();
}

bool
PlaylistBrowserNS::UserModel::setData( const QModelIndex &idx, const QVariant &value, int role )
{
    Q_UNUSED( role )

    switch( idx.column() )
    {
        case MetaPlaylistModel::PlaylistColumn:
        {
            debug() << "setting name of item " << idx.internalId() << " to " << value.toString();
            Playlists::PlaylistPtr item = m_playlists.value( idx.internalId() );
            item->setName( value.toString() );
            break;
        }
        case MetaPlaylistModel::LabelColumn:
        {
            debug() << "changing group of item " << idx.internalId() << " to " << value.toString();
            Playlists::PlaylistPtr item = m_playlists.value( idx.internalId() );
            item->setGroups( value.toStringList() );
            break;
        }
        default:
            return false;
    }

    return true;
}

bool
PlaylistBrowserNS::UserModel::removeRows( int row, int count, const QModelIndex &parent )
{
    if( row < 0 || row > rowCount( parent ) )
        return false;

    if( !parent.isValid() )
    {
      Playlists::PlaylistList playlistToRemove;
      for( int i = row; i < row + count; i++ )
      {
        if( m_playlists.count() > i )
        {
            Playlists::PlaylistPtr playlist = m_playlists[i];
            debug() << "Removing " << playlist->name();
            playlistToRemove << playlist;
        }
      }
      if( playlistToRemove.isEmpty() )
        return false;

      The::playlistManager()->deletePlaylists( playlistToRemove );
      return true;
    }
    int playlistRow = REMOVE_TRACK_MASK(parent.internalId());

    //don't try to get a playlist beyond the last item in the list
    if( playlistRow >=  m_playlists.count() )
    {
        error() << "Tried to remove from non existing playlist:";
        error() << playlistRow << " while there are only " << m_playlists.count();
        return false;
    }

    Playlists::PlaylistPtr playlist = m_playlists.value( playlistRow );

    //if we are trying to delete more tracks then what the playlist has, return.
    //count will be at least 1 to delete one track
    if( row + count - 1 >= playlist->tracks().count() )
    {
        error() << "Tried to remove a track using an index that is not there:";
        error() << "row: " << row << " count: " << count << " number of tracks: "
                << playlist->tracks().count();
        return false;
    }

    beginRemoveRows( parent, row, row + count - 1 );
    //ignore notifications while removing tracks
    playlist->unsubscribe( this );
    for( int i = row; i < row + count; i++ )
        //deleting a track moves the next track up, so use the same row number each time
        playlist->removeTrack( row );
    playlist->subscribe( this );
    endRemoveRows();

    return true;
}

QStringList
PlaylistBrowserNS::UserModel::mimeTypes() const
{
    QStringList ret;
    ret << AmarokMimeData::PLAYLIST_MIME;
    ret << AmarokMimeData::TRACK_MIME;
    ret << "text/uri-list"; //we do accept urls
    return ret;
}

QMimeData*
PlaylistBrowserNS::UserModel::mimeData( const QModelIndexList &indices ) const
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
PlaylistBrowserNS::UserModel::dropMimeData ( const QMimeData *data, Qt::DropAction action, int row,
        int column, const QModelIndex &parent ) //reimplemented
{
    if( action == Qt::IgnoreAction )
        return true;

    //drop on track is not possible
    if( IS_TRACK(parent) )
            return false;

    if( data->hasFormat( AmarokMimeData::TRACK_MIME ) )
    {
        const AmarokMimeData* dragList = dynamic_cast<const AmarokMimeData*>( data );
        if( !dragList )
            return false;

        emit layoutAboutToBeChanged();
        int playlistRow = REMOVE_TRACK_MASK(parent.internalId());
        Playlists::PlaylistPtr playlist = m_playlists.value( playlistRow );
        if( playlist )
        {
            int insertAt = (row == -1) ? playlist->tracks().count() : row;
            foreach( Meta::TrackPtr track, dragList->tracks() )
                playlist->addTrack( track, insertAt++ );
            emit rowsInserted( parent, row, insertAt );
        }
        return !playlist.isNull();
    }

    if( data->hasUrls() )
    {
        bool success = true;
        foreach( const QUrl &url, data->urls() )
            success = The::playlistManager()->import( url.toString() ) ? success : false;
        return success;
    }

    return false;
}

QList<QAction *>
PlaylistBrowserNS::UserModel::actionsFor( const QModelIndex &idx ) const
{
    //wheter we use the list from m_appendAction of m_loadAction does not matter they are the same
    QModelIndexList actionList = m_appendAction->data().value<QModelIndexList>();

    actionList << idx;
    QVariant value = QVariant::fromValue( actionList );
    m_appendAction->setData( value );
    m_loadAction->setData( value );

    QList<QAction *> actions;
    actions << m_appendAction << m_loadAction;

    // It's a playlist, we return playlist actions
    if( !IS_TRACK(idx) )
    {
        Playlists::PlaylistPtr playlist = m_playlists.value( idx.internalId() );
        if( playlist->provider() )
            actions << playlist->provider()->playlistActions( playlist );
    }
    // Otherwise, tracks are selected, so we bring up track actions
    else
    {
        Playlists::PlaylistPtr playlist = m_playlists.value( idx.parent().internalId() );
        if( playlist->provider() )
            actions << playlist->provider()->trackActions( playlist, idx.row() );
    }

    return actions;
}

void
PlaylistBrowserNS::UserModel::loadItems( QModelIndexList list, Playlist::AddOptions insertMode )
{
    DEBUG_BLOCK
    Playlists::PlaylistList playlists;
    foreach( const QModelIndex &index, list )
    {
        Playlists::PlaylistPtr playlist =
                m_playlists.value( index.internalId() );
        if( playlist )
            playlists << playlist;
    }
    if( !playlists.isEmpty() )
        The::playlistController()->insertOptioned( playlists, insertMode );
}

void
PlaylistBrowserNS::UserModel::slotLoad()
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
PlaylistBrowserNS::UserModel::slotAppend()
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
PlaylistBrowserNS::UserModel::tracksFromIndexes( const QModelIndexList &list ) const
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
PlaylistBrowserNS::UserModel::trackFromIndex( const QModelIndex &index ) const
{
    if( !index.isValid() )
        return Meta::TrackPtr();

    Playlists::PlaylistPtr playlist = m_playlists.value( REMOVE_TRACK_MASK(index.internalId()) );
    if( playlist.isNull() || playlist->tracks().count() <= index.row() )
        return Meta::TrackPtr();

    return playlist->tracks()[index.row()];
}

Playlists::PlaylistPtr
PlaylistBrowserNS::UserModel::playlistFromIndex( const QModelIndex &index ) const
{
    if( !index.isValid() )
        return Playlists::PlaylistPtr();

    return m_playlists.value( index.internalId() );
}

void
PlaylistBrowserNS::UserModel::trackAdded( Playlists::PlaylistPtr playlist, Meta::TrackPtr track,
                                          int position )
{
    int indexNumber = m_playlists.indexOf( playlist );
    if( indexNumber == -1 )
    {
        error() << "This playlist is not in the list of this model.";
        return;
    }
    QModelIndex playlistIdx = index( indexNumber, 0, QModelIndex() );
    rowsInserted( playlistIdx, position, position );
}

void
PlaylistBrowserNS::UserModel::trackRemoved( Playlists::PlaylistPtr playlist, int position )
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

#include "UserPlaylistModel.moc"
