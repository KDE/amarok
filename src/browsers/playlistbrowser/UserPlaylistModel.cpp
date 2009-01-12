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
 : QAbstractItemModel()
{

//     m_root = PlaylistGroupPtr( new PlaylistGroup( "root", PlaylistGroupPtr() ) );
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

    if ( !index.isValid() )
        return QVariant();

//     PlaylistViewItemPtr item =  m_viewItems.value( index.internalId() );
    Meta::PlaylistPtr item = m_playlists.value( index.internalId() );

    if ( role == 0xf00d )
        return QVariant::fromValue( item );
    else if ( role == Qt::DisplayRole || role == Qt::EditRole )
        return item->name();
    else if (role == Qt::DecorationRole )
    {
//         if ( typeid( * item ) == typeid( PlaylistGroup ) )
//             return QVariant( KIcon( "folder-amarok" ) );
//         else if ( typeid( * item ) == typeid( Meta::Playlist ) )
            return QVariant( KIcon( "amarok_playlist" ) );
    }

    return QVariant();
}


/*
QModelIndex
PlaylistBrowserNS::UserModel::createIndex( int row, int column, PlaylistViewItemPtr item ) const
{
    quint32 index = qHash( item.data() );
    bool debugIt = false;
    if( m_viewItems.contains( index ) )
        debugIt = false;
    else
        m_viewItems[ index ] = item;
    QModelIndex ret = QAbstractItemModel::createIndex( row, column, index );
//    if( debugIt )
//        debug() << "created " << ret << " with " << ret.parent().internalId();
    return ret;
}
*/

QModelIndex
PlaylistBrowserNS::UserModel::index(int row, int column, const QModelIndex & parent) const
{
    DEBUG_BLOCK

    debug() << "row: " << row << ", column: " <<column;
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex( row, column, row );
}

QModelIndex
PlaylistBrowserNS::UserModel::parent( const QModelIndex & index ) const
{
    DEBUG_BLOCK

    return QModelIndex();
}

int
PlaylistBrowserNS::UserModel::rowCount( const QModelIndex & parent ) const
{
    DEBUG_BLOCK

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

bool PlaylistBrowserNS::UserModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::EditRole)
        return false;
    if ( index.column() != 0 )
        return false;

    Meta::PlaylistPtr item = m_playlists.value( index.internalId() );

    item->setName( value.toString() );

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
PlaylistBrowserNS::UserModel::mimeData( const QModelIndexList &indexes ) const
{
    DEBUG_BLOCK
    AmarokMimeData* mime = new AmarokMimeData();

//     PlaylistGroupList groups;
    Meta::PlaylistList playlists;

    foreach( const QModelIndex &index, indexes )
    {
        playlists << m_playlists.value( index.internalId() );

//         PlaylistViewItemPtr item = m_viewItems.value( index.internalId() );

//         if ( typeid( * item ) == typeid( PlaylistGroup ) ) {
//             PlaylistGroupPtr playlistGroup = PlaylistGroupPtr::staticCast( item );
//             groups << playlistGroup;
//         }
//         else
//         {
//             Meta::PlaylistPtr playlist = Meta::PlaylistPtr::dynamicCast( item );
//             if( playlist )
//                 playlists << playlist;
//         }
    }

//     mime->setPlaylistGroups( groups );
    mime->setPlaylists( playlists );

    return mime;
}


bool
PlaylistBrowserNS::UserModel::dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent ) //reimplemented
{
    Q_UNUSED( column );
    Q_UNUSED( row );
//     DEBUG_BLOCK

    if( action == Qt::IgnoreAction )
        return true;

    /*PlaylistGroupPtr parentGroup;
    if ( !parent.isValid() )
    {
        parentGroup = m_root;
    }
    else
    {
        parentGroup = PlaylistGroupPtr::staticCast( m_viewItems.value( parent.internalId() ) );
    }

    if( data->hasFormat( AmarokMimeData::PLAYLISTBROWSERGROUP_MIME ) )
    {
        debug() << "Found playlist group mime type";

        const AmarokMimeData* playlistGroupDrag = dynamic_cast<const AmarokMimeData*>( data );
        if( playlistGroupDrag )
        {

            PlaylistGroupList groups = playlistGroupDrag->sqlPlaylistsGroups();

            foreach( PlaylistGroupPtr group, groups ) {
                group->reparent( parentGroup );
            }

            reloadFromDb();

            return true;
        }
    }
    else*/ if( data->hasFormat( AmarokMimeData::PLAYLIST_MIME ) )
    {
        debug() << "Found playlist mime type";

//         const AmarokMimeData* dragList = dynamic_cast<const AmarokMimeData*>( data );
//         if( dragList )
//         {
//
//             Meta::PlaylistList playlists = dragList->playlists();
//
//             foreach( Meta::PlaylistPtr playlistPtr, playlists )
//             {
//                 Meta::PlaylistPtr playlist = Meta::PlaylistPtr::dynamicCast( playlistPtr );
//
//                 if( playlist )
//                     playlist->reparent( parentGroup );
//             };

            return true;
//         }
    }

    return false;
}

void
PlaylistBrowserNS::UserModel::editPlaylist( int id )
{

  //for now, assume that the newly added playlist is in the top level:
//     int row = m_root->childGroups().count() - 1;
//     foreach ( Meta::PlaylistPtr playlist, m_root->childPlaylists() ) {
//         row++;
//         if ( playlist->id() == id ) {
//             emit editIndex( createIndex( row , 0, PlaylistViewItemPtr::staticCast( playlist ) ) );
//         }
//     }
}

// void
// PlaylistBrowserNS::UserModel::createNewGroup()
// {
//     DEBUG_BLOCK
//
//     PlaylistGroup * group = new PlaylistGroup( i18n("New Group"), m_root );
//     group->save();
//     int id = group->id();
//     delete group;
//
//     reloadFromDb();
//
//     int row = 0;
//     foreach ( PlaylistGroupPtr childGroup, m_root->childGroups() ) {
//         if ( childGroup->id() == id )
//         {
//             debug() << "emmiting edit for " << childGroup->name() << " id " << childGroup->id() << " in row " << row;
//             emit editIndex( createIndex( row , 0, PlaylistViewItemPtr::staticCast( childGroup ) ) );
//         }
//         row++;
//     }
//
// }

// void
// PlaylistBrowserNS::UserModel::createNewStream( const QString& streamName, const Meta::TrackPtr& streamTrack )
// {
//     Meta::TrackList list;
//     list.append( streamTrack );
//     Meta::Playlist *stream = new Meta::Playlist( streamName, list, m_root );
//     delete stream;
//     reloadFromDb();
// }

#include "UserPlaylistModel.moc"
