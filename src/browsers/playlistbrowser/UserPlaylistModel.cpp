/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *             (c) 2008  Ian Monroe <imonroe@kde.org>                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

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

PlaylistBrowserNS::UserModel * PlaylistBrowserNS::UserModel::s_instance = 0;

PlaylistBrowserNS::UserModel * PlaylistBrowserNS::UserModel::instance()
{
    if ( s_instance == 0 )
        s_instance = new UserModel();

    return s_instance;
}

PlaylistBrowserNS::UserModel::UserModel()
    : MetaPlaylistModel()
    , m_appendAction( 0 )
    , m_loadAction( 0 )
{
    loadPlaylists();

    connect( The::playlistManager(), SIGNAL(updated()), SLOT(slotUpdate()) );
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
PlaylistBrowserNS::UserModel::loadPlaylists()
{
    DEBUG_BLOCK
    QList<Meta::PlaylistPtr> playlists =
    The::playlistManager()->playlistsOfCategory( PlaylistManager::UserPlaylist );
    QListIterator<Meta::PlaylistPtr> i(playlists);
    m_playlists.clear();
    while (i.hasNext())
    {
        Meta::PlaylistPtr playlist = Meta::PlaylistPtr::staticCast( i.next() );
        m_playlists << playlist;
    }
}

QVariant
PlaylistBrowserNS::UserModel::data(const QModelIndex & index, int role) const
{
    DEBUG_BLOCK

    if ( !index.isValid() )
        return QVariant();

    Meta::PlaylistPtr item = m_playlists.value( index.internalId() );

    switch( role )
    {
        case 0xf00d: return QVariant::fromValue( item );
        case Qt::DisplayRole:
        case Qt::EditRole: return item->name();
        case DescriptionRole:
        case Qt::ToolTipRole: return item->description();
        case OriginRole: return QVariant(); //TODO return the provider name
        case Qt::DecorationRole: return QVariant( KIcon( "amarok_playlist" ) );
        case GroupRole: return item->groups();
        default: return QVariant();
    }
}

QModelIndex
PlaylistBrowserNS::UserModel::index(int row, int column, const QModelIndex & parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex( row, column, row );
}

QModelIndex
PlaylistBrowserNS::UserModel::parent( const QModelIndex & index ) const
{
    Q_UNUSED( index )
    return QModelIndex();
}

int
PlaylistBrowserNS::UserModel::rowCount( const QModelIndex & parent ) const
{
    if (parent.column() > 0)
    {
        return 0;
    }

    if (!parent.isValid())
    {
        return m_playlists.count();
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

    //item is a playlist
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
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
    if (role != Qt::EditRole)
        return false;
    if ( index.column() != 0 )
        return false;

    debug() << "setting name of item " << index.internalId() << " to " << value.toString();
    Meta::PlaylistPtr item = m_playlists.value( index.internalId() );

    item->setName( value.toString() );

    //call update reload playlists and emit signals
    slotUpdate();
    return true;
}

QStringList
PlaylistBrowserNS::UserModel::mimeTypes() const
{
    QStringList ret; // = QAbstractListModel::mimeTypes();
//     ret << AmarokMimeData::PLAYLISTBROWSERGROUP_MIME;
    ret << AmarokMimeData::PLAYLIST_MIME;
    //ret << "text/uri-list"; //we do accept urls
    return ret;
}

QMimeData*
PlaylistBrowserNS::UserModel::mimeData( const QModelIndexList &indices ) const
{
    DEBUG_BLOCK
    AmarokMimeData* mime = new AmarokMimeData();

    Meta::PlaylistList playlists;

    foreach( const QModelIndex &index, indices )
    {
        playlists << m_playlists.value( index.internalId() );
    }

    mime->setPlaylists( playlists );

    return mime;
}

bool
PlaylistBrowserNS::UserModel::dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent ) //reimplemented
{
    Q_UNUSED( column );
    Q_UNUSED( row );
    Q_UNUSED( parent )

    if( action == Qt::IgnoreAction )
        return true;

    if( data->hasFormat( AmarokMimeData::PLAYLIST_MIME ) )
    {
        debug() << "Found playlist mime type";

        const AmarokMimeData* dragList = dynamic_cast<const AmarokMimeData*>( data );
        if( dragList )
        {

            Meta::PlaylistList playlists = dragList->playlists();

            foreach( Meta::PlaylistPtr playlistPtr, playlists )
            {
                //TODO: found out if it dropped on a group and add the group to the playlists
            };

            return true;
        }
    }

    return false;
}

void
PlaylistBrowserNS::UserModel::createNewGroup()
{
    DEBUG_BLOCK
    //TODO: create the group in default provider if that supports empty groups.
}

QList<PopupDropperAction *>
PlaylistBrowserNS::UserModel::actionsFor( const QModelIndexList &indices )
{
    DEBUG_BLOCK
    QList<PopupDropperAction *> actions;
    m_selectedPlaylists.clear();
    m_selectedPlaylists << selectedPlaylists( indices );

    actions << createCommonActions( indices );

    return actions;
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

//     if ( m_deleteAction == 0 )
//     {
//         m_deleteAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "delete", KIcon( "media-track-remove-amarok" ), i18n( "&Delete" ), this );
//         connect( m_deleteAction, SIGNAL( triggered() ), this, SLOT( slotDelete() ) );
//     }
//
//     if ( m_renameAction == 0 )
//     {
//         m_renameAction =  new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "edit", KIcon( "media-track-edit-amarok" ), i18n( "&Rename" ), this );
//         connect( m_renameAction, SIGNAL( triggered() ), this, SLOT( slotRename() ) );
//     }

    if ( indices.count() > 0 )
    {
        actions << m_appendAction;
        actions << m_loadAction;
        //menu.addSeparator();
    }

    return actions;
}

void
PlaylistBrowserNS::UserModel::slotLoad()
{
    foreach( Meta::PlaylistPtr playlist, selectedPlaylists() )
    {
        The::playlistController()->insertOptioned( playlist->tracks(), Playlist::LoadAndPlay );
    }
}

void
PlaylistBrowserNS::UserModel::slotAppend()
{
    foreach( Meta::PlaylistPtr playlist, selectedPlaylists() )
    {
        The::playlistController()->insertOptioned( playlist->tracks(), Playlist::AppendAndPlay );
    }
}

Meta::PlaylistList
PlaylistBrowserNS::UserModel::selectedPlaylists( const QModelIndexList &list )
{
    Meta::PlaylistList playlists;
    foreach( QModelIndex index, list )
        playlists << m_playlists.value( index.internalId() );
    return playlists;
}

#include "UserPlaylistModel.moc"
