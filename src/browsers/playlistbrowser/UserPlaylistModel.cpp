/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2008 Ian Monroe <imonroe@kde.org>                                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "UserPlaylistModel.h"
#include "playlistmanager/PlaylistManager.h"

#include "AmarokMimeData.h"
#include "Debug.h"
#include "CollectionManager.h"
#include "context/popupdropper/libpud/PopupDropperAction.h"
#include "SvgHandler.h"

#include <KIcon>

#include <QAbstractListModel>

#include <typeinfo>

//Playlist & Track index differentiator macros
//QModelIndex::intenalId() is a qint64 to support 64-bit pointers in a union with the ID
#define TRACK_MASK (0x1<<63)
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

PlaylistBrowserNS::UserModel * PlaylistBrowserNS::UserModel::s_instance = 0;

PlaylistBrowserNS::UserModel * PlaylistBrowserNS::UserModel::instance()
{
    if ( s_instance == 0 )
        s_instance = new UserModel();

    return s_instance;
}

void
PlaylistBrowserNS::UserModel::destroy()
{
    if (s_instance) {
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

    connect( The::playlistManager(), SIGNAL(updated()), SLOT(slotUpdate()) );
    connect( The::playlistManager(), SIGNAL(renamePlaylist( Meta::PlaylistPtr )),
             SLOT(slotRenamePlaylist( Meta::PlaylistPtr )) );
}

PlaylistBrowserNS::UserModel::~UserModel()
{
}

void
PlaylistBrowserNS::UserModel::slotUpdate()
{
    DEBUG_BLOCK
    loadPlaylists();

    emit layoutAboutToBeChanged();
    emit layoutChanged();
}

void
PlaylistBrowserNS::UserModel::slotRenamePlaylist( Meta::PlaylistPtr playlist )
{
    DEBUG_BLOCK
    //search index of this Playlist
    int row = -1;
    foreach( const Meta::PlaylistPtr p, m_playlists )
    {
        row++;
        if( p->name() == playlist->name() )
            break;
    }
    if( row == -1 )
        return;

    QModelIndex index = this->index( row, 0, QModelIndex() );
    debug() << index;
    emit( renameIndex( index ) );
}

void
PlaylistBrowserNS::UserModel::loadPlaylists()
{
    DEBUG_BLOCK
    QList<Meta::PlaylistPtr> playlists =
    The::playlistManager()->playlistsOfCategory( PlaylistManager::UserPlaylist );
    QListIterator<Meta::PlaylistPtr> i(playlists);
    m_playlists.clear();
    while( i.hasNext() )
    {
        Meta::PlaylistPtr playlist = Meta::PlaylistPtr::staticCast( i.next() );
        m_playlists << playlist;
    }
}

QVariant
PlaylistBrowserNS::UserModel::data(const QModelIndex & index, int role) const
{
//    debug() << "index: " << index;
    if ( !index.isValid() )
        return QVariant();

    int row = REMOVE_TRACK_MASK(index.internalId());
//    debug() << "playlist at row: " << row;
    Meta::PlaylistPtr playlist = m_playlists.value( row );

    QVariant food;
    QString name;
    QString description;
    KIcon icon;
    QStringList groups;

    if( IS_TRACK(index) )
    {
        Meta::TrackPtr track = playlist->tracks()[index.row()];
        food = QVariant::fromValue( track );
        name = track->prettyName();
        icon = KIcon( "amarok_track" );
    }
    else
    {
        food = QVariant::fromValue( playlist );
        name = playlist->name();
        description = playlist->description();
        icon = KIcon( "amarok_playlist" );
        groups = playlist->groups();
    }

    switch( role )
    {
        case 0xf00d: return food;
        case Qt::DisplayRole:
        case Qt::EditRole: return name;
        case DescriptionRole:
        case Qt::ToolTipRole: return description;
        case OriginRole: return QVariant(); //TODO return the provider name
        case Qt::DecorationRole: return QVariant( icon );
        case GroupRole: return groups;
        default: return QVariant();
    }
}

QModelIndex
PlaylistBrowserNS::UserModel::index(int row, int column, const QModelIndex & parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    //if it has a parent it is a track
    if( parent.isValid() )
        return createIndex( row, column, SET_TRACK_MASK(parent.row()) );
    return createIndex( row, column, row );
}

QModelIndex
PlaylistBrowserNS::UserModel::parent( const QModelIndex & index ) const
{
//    DEBUG_BLOCK
//    debug() << index;
    if( IS_TRACK(index) )
    {
//        debug() << " is a track.";
        int row = REMOVE_TRACK_MASK(index.internalId());
//        debug() << "parent at row: " << row;
        return this->index( row, index.column(), QModelIndex() );
    }

    return QModelIndex();
}

int
PlaylistBrowserNS::UserModel::rowCount( const QModelIndex & parent ) const
{
//    debug() << "parent: " << parent;
    if (parent.column() > 0)
    {
        return 0;
    }

    if (!parent.isValid())
    {
        return m_playlists.count();
    }
    else if( !IS_TRACK(parent) )
    {
        Meta::PlaylistPtr playlist = m_playlists.value( parent.internalId() );
        //debug() << QString( "has %1 tracks.").arg(playlist->tracks().count());
        return playlist->tracks().count();
    }

    return 0;
}

int
PlaylistBrowserNS::UserModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 1;
}

Qt::ItemFlags
PlaylistBrowserNS::UserModel::flags( const QModelIndex & index ) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;

    if( IS_TRACK(index) )
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;

    //item is a playlist
    return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;
}

QVariant
PlaylistBrowserNS::UserModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch( section )
        {
            case 0: return i18n("Name");
            default: return QVariant();
        }
    }

    return QVariant();
}

bool
PlaylistBrowserNS::UserModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    DEBUG_BLOCK
    switch( role )
    {
        case Qt::EditRole:
        {
            debug() << "setting name of item " << index.internalId() << " to " << value.toString();
            Meta::PlaylistPtr item = m_playlists.value( index.internalId() );
            item->setName( value.toString() );
            break;
        }
        case GroupRole:
        {
            debug() << "changing group of item " << index.internalId() << " to " << value.toString();
            Meta::PlaylistPtr item = m_playlists.value( index.internalId() );
            item->setGroups( value.toStringList() );
            break;
        }
        default:
            return false;
    }

    //call update reload playlists and emit signals
    slotUpdate();
    return true;
}

bool
PlaylistBrowserNS::UserModel::removeRows( int row, int count, const QModelIndex &parent )
{
    DEBUG_BLOCK
    debug() << "in parent " << parent << "remove " << count << " starting at row " << row;
    int playlistRow = REMOVE_TRACK_MASK(parent.internalId());
    debug() << "playlist at row: " << playlistRow;
    Meta::PlaylistPtr playlist = m_playlists.value( playlistRow );
    for( int i = row; i < row + count; i++ )
        playlist->removeTrack( i );
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
    DEBUG_BLOCK
    AmarokMimeData* mime = new AmarokMimeData();

    Meta::PlaylistList playlists;
    Meta::TrackList tracks;

    foreach( const QModelIndex &index, indices )
    {
        if( IS_TRACK(index) )
        {
            debug() << "track";
            tracks << trackFromIndex( index );
        }
        else
        {
            debug() << "playlist";
            playlists << m_playlists.value( index.internalId() );
        }
    }

    mime->setPlaylists( playlists );
    mime->setTracks( tracks );

    return mime;
}

bool
PlaylistBrowserNS::UserModel::dropMimeData ( const QMimeData *data, Qt::DropAction action, int row,
        int column, const QModelIndex &parent ) //reimplemented
{
    DEBUG_BLOCK
    debug() << "droped on " << QString("row: %1, column: %2, parent:").arg( row ).arg( column );
    debug() << parent;

    if( action == Qt::IgnoreAction )
        return true;

    //drop on track is not possible
    if( IS_TRACK(parent) )
            return false;

    if( data->hasFormat( AmarokMimeData::TRACK_MIME ) )
    {
        debug() << "Found track mime type";
        const AmarokMimeData* dragList = dynamic_cast<const AmarokMimeData*>( data );
        if( !dragList )
            return false;

        emit layoutAboutToBeChanged();
        int playlistRow = REMOVE_TRACK_MASK(parent.internalId());
        debug() << "playlist at row: " << playlistRow;
        Meta::PlaylistPtr playlist = m_playlists.value( playlistRow );
        int insertAt = (row == -1) ? playlist->tracks().count() : row;
        foreach( Meta::TrackPtr track, dragList->tracks() )
        {
            debug() << track->prettyName() << "dropped on " << playlist->prettyName() << "insert at " << insertAt;
            playlist->addTrack( track, insertAt++ );
        }
        emit rowsInserted( parent, row, insertAt );

        return true;
    }

#if 0
    if( data->hasFormat( AmarokMimeData::PLAYLIST_MIME ) )
    {
        debug() << "Found playlist mime type";

        const AmarokMimeData* dragList = dynamic_cast<const AmarokMimeData*>( data );
        if( !dragList )
            return false;

        foreach( Meta::PlaylistPtr playlistPtr, dragList->playlists() )
        {
            //TODO: figure out what to do when a playlist is dropped onto another playlist
        }

        return true;
    }
#endif

    return false;
}

QList<PopupDropperAction *>
PlaylistBrowserNS::UserModel::actionsFor( const QModelIndexList &indices )
{
    QSet<PopupDropperAction *> actions;
    QList<PopupDropperAction *> actionList;
    m_selectedPlaylists.clear();
    m_selectedPlaylists << selectedPlaylists( indices );
    m_selectedTracks.clear();
    m_selectedTracks << selectedTracks( indices );

    if( !m_selectedPlaylists.isEmpty() )
    {
        actions += QSet<PopupDropperAction *>::fromList(
                            The::playlistManager()->playlistActions( m_selectedPlaylists )
                        );
    }
    else
    {
        foreach( const QModelIndex &idx, indices )
        {
            actions += QSet<PopupDropperAction *>::fromList(
                            The::playlistManager()->trackActions(
                                        m_playlists.value( idx.parent().internalId() ),
                                        idx.row()
                                    )
                        );
        }
    }

    actionList = actions.toList();
    actionList << createCommonActions( indices );

    return actionList;
}

void
PlaylistBrowserNS::UserModel::loadItems( QModelIndexList list, Playlist::AddOptions insertMode )
{
    DEBUG_BLOCK
    Meta::PlaylistList playlists;
    foreach( const QModelIndex &index, list )
    {
        Meta::PlaylistPtr playlist =
                m_playlists.value( index.internalId() );
        if( playlist )
            playlists << playlist;
    }
    if( !playlists.isEmpty() )
        The::playlistController()->insertOptioned( playlists, insertMode );
}

QList<PopupDropperAction *>
PlaylistBrowserNS::UserModel::createCommonActions( QModelIndexList indices )
{
    DEBUG_BLOCK
    QList< PopupDropperAction * > actions;

    if ( m_appendAction == 0 )
    {
        m_appendAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "append", KIcon( "media-track-add-amarok" ), i18n( "&Append to Playlist" ), this );
        connect( m_appendAction, SIGNAL( triggered() ), this, SLOT( slotAppend() ) );
    }

    if ( m_loadAction == 0 )
    {
        m_loadAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "load", KIcon( "folder-open" ), i18nc( "Replace the currently loaded tracks with these", "&Load" ), this );
        connect( m_loadAction, SIGNAL( triggered() ), this, SLOT( slotLoad() ) );
    }

    if ( indices.count() > 0 )
    {
        actions << m_appendAction;
        actions << m_loadAction;
    }

    return actions;
}

void
PlaylistBrowserNS::UserModel::slotLoad()
{
    Meta::TrackList tracks;
    foreach( Meta::TrackPtr track, selectedTracks() )
            tracks << track;
    foreach( Meta::PlaylistPtr playlist, selectedPlaylists() )
        tracks << playlist->tracks();
    if( !tracks.isEmpty() )
        The::playlistController()->insertOptioned( tracks, Playlist::LoadAndPlay );
}

void
PlaylistBrowserNS::UserModel::slotAppend()
{
    Meta::TrackList tracks;
    foreach( Meta::PlaylistPtr playlist, selectedPlaylists() )
    {
        if( playlist )
            tracks << playlist->tracks();
    }
    foreach( Meta::TrackPtr track, selectedTracks() )
        tracks << track;
    if( !tracks.isEmpty() )
        The::playlistController()->insertOptioned( tracks, Playlist::AppendAndPlay );
}

Meta::PlaylistList
PlaylistBrowserNS::UserModel::selectedPlaylists( const QModelIndexList &list )
{
    Meta::PlaylistList playlists;
    foreach( const QModelIndex &index, list )
    {
        if( !IS_TRACK(index) )
            playlists << m_playlists.value( index.internalId() );
    }
    return playlists;
}

Meta::TrackList
PlaylistBrowserNS::UserModel::selectedTracks( const QModelIndexList &list )
{
    Meta::TrackList tracks;
    foreach( const QModelIndex &index, list )
    {
        if( IS_TRACK(index) )
            tracks << trackFromIndex( index );
    }
    return tracks;
}

Meta::TrackPtr
PlaylistBrowserNS::UserModel::trackFromIndex( const QModelIndex &index ) const
{
    Meta::PlaylistPtr playlist = m_playlists.value(
            REMOVE_TRACK_MASK(index.internalId()) );
    return playlist->tracks()[index.row()];
}

#include "UserPlaylistModel.moc"
