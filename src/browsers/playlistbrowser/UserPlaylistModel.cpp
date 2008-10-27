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

#include "AmarokMimeData.h"
#include "Debug.h"
#include "CollectionManager.h"
#include "SqlStorage.h"
#include "SqlPlaylist.h"
#include "SqlPlaylistGroup.h"

#include <KIcon>

#include <QAbstractListModel>
#include <QListIterator>

#include <typeinfo>

static const int USERPLAYLIST_DB_VERSION = 1;
static const QString key("AMAROK_USERPLAYLIST");

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
    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
    QStringList values = sqlStorage->query( QString("SELECT version FROM admin WHERE component = '%1';").arg(sqlStorage->escape( key ) ) );
    if( values.isEmpty() )
    {
        //debug() << "creating Playlist Tables";
        createTables();
    }

    m_root = SqlPlaylistGroupPtr( new SqlPlaylistGroup( "root", SqlPlaylistGroupPtr() ) );
}


PlaylistBrowserNS::UserModel::~UserModel()
{
}

QVariant
PlaylistBrowserNS::UserModel::data(const QModelIndex & index, int role) const
{
    
    if ( !index.isValid() )
        return QVariant();

    SqlPlaylistViewItemPtr item =  m_viewItems.value( index.internalId() );

    if ( role == 0xf00d )
        return QVariant::fromValue( item );
    else if ( role == Qt::DisplayRole || role == Qt::EditRole )
        return item->name();
    else if (role == Qt::DecorationRole ) {

        if ( typeid( * item ) == typeid( SqlPlaylistGroup ) )
            return QVariant( KIcon( "folder-amarok" ) );
        else if ( typeid( * item ) == typeid( Meta::SqlPlaylist ) )
            return QVariant( KIcon( "x-media-podcast-amarok" ) );
    }

    return QVariant();
}


QModelIndex
PlaylistBrowserNS::UserModel::createIndex( int row, int column, SqlPlaylistViewItemPtr item ) const
{
    quint32 index = qHash( item.data() );
    bool debugIt = false;
    if( m_viewItems.contains( index ) )
        debugIt = false;
    else
        m_viewItems[ index ] = item;
    QModelIndex ret = QAbstractItemModel::createIndex( row, column, index );
    if( debugIt )
        debug() << "created " << ret << " with " << ret.parent().internalId();
    return ret;
}

QModelIndex
PlaylistBrowserNS::UserModel::index(int row, int column, const QModelIndex & parent) const
{
    //DEBUG_BLOCK

    //debug() << "row: " << row << ", column: " <<column;
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    if ( !parent.isValid() )
    {

        int noOfGroups = m_root->childGroups().count();
        if ( row < noOfGroups )
        {
            return createIndex( row, column, SqlPlaylistViewItemPtr::staticCast( m_root->childGroups().at( row ) ) );
        }
        else
        {
            //debug() << "Root playlist";
            return createIndex( row, column, SqlPlaylistViewItemPtr::staticCast( m_root->childPlaylists().at( row - noOfGroups ) ) );
        }
    }
    else
    {
        SqlPlaylistGroupPtr playlistGroup = SqlPlaylistGroupPtr::staticCast( m_viewItems.value( parent.internalId() ) );
        int noOfGroups = playlistGroup->childGroups().count();

        if ( row < noOfGroups )
        {
            return createIndex( row, column, SqlPlaylistViewItemPtr::staticCast( playlistGroup->childGroups().at(row) ) );
        }
        else
        {
            return createIndex( row, column, SqlPlaylistViewItemPtr::staticCast( playlistGroup->childPlaylists().at(row - noOfGroups) ) );
        }
    }
}

QModelIndex
PlaylistBrowserNS::UserModel::parent( const QModelIndex & index ) const
{
    //DEBUG_BLOCK

    if (!index.isValid())
        return QModelIndex();
    SqlPlaylistViewItemPtr item = m_viewItems.value( index.internalId() );
    
    SqlPlaylistGroupPtr parent = item->parent();

    //debug() << "parent: " << parent;

    if ( parent &&  parent->parent() )
    {
        int row = parent->parent()->childGroups().indexOf( parent );
        return createIndex( row , 0, SqlPlaylistViewItemPtr::staticCast( parent ) );
    }
    else {
        return QModelIndex();
    }
}

int
PlaylistBrowserNS::UserModel::rowCount( const QModelIndex & parent ) const
{
    //DEBUG_BLOCK

    if (parent.column() > 0) {
        return 0;
    }

    if (!parent.isValid()) {

        //debug() << "top level item has" << m_root->childCount();

        return m_root->childCount();

    }
    SqlPlaylistViewItemPtr item = m_viewItems.value( parent.internalId() );
    //debug() << "row: " << parent.row();
    //debug() << "address: " << item;
    //debug() << "name: " << item->name();

    return item->childCount();
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
    SqlPlaylistViewItemPtr item = SqlPlaylistViewItemPtr::staticCast( m_viewItems.value( index.internalId() ) );

    if ( typeid( * item ) == typeid( SqlPlaylistGroup ) )
        return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled;
    else
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

    SqlPlaylistViewItemPtr item = m_viewItems.value( index.internalId() );

    item->rename( value.toString() );

    return true;

}

QStringList
PlaylistBrowserNS::UserModel::mimeTypes() const
{
    QStringList ret; // = QAbstractListModel::mimeTypes();
    ret << AmarokMimeData::PLAYLISTBROWSERGROUP_MIME;
    ret << AmarokMimeData::PLAYLIST_MIME;
    //ret << "text/uri-list"; //we do accept urls
    return ret;
}

QMimeData*
PlaylistBrowserNS::UserModel::mimeData( const QModelIndexList &indexes ) const
{
    DEBUG_BLOCK
    AmarokMimeData* mime = new AmarokMimeData();

    SqlPlaylistGroupList groups;
    Meta::PlaylistList playlists;

    foreach( const QModelIndex &index, indexes ) {

        SqlPlaylistViewItemPtr item = m_viewItems.value( index.internalId() );

        if ( typeid( * item ) == typeid( SqlPlaylistGroup ) ) {
            SqlPlaylistGroupPtr playlistGroup = SqlPlaylistGroupPtr::staticCast( item );
            groups << playlistGroup;
        }
        else
        {
            Meta::PlaylistPtr playlist = Meta::PlaylistPtr::dynamicCast( item );
            if( playlist )
                playlists << playlist;
        }
    }

    mime->setPlaylistGroups( groups );
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

    SqlPlaylistGroupPtr parentGroup;
    if ( !parent.isValid() )
    {
        parentGroup = m_root;
    }
    else
    {
        parentGroup = SqlPlaylistGroupPtr::staticCast( m_viewItems.value( parent.internalId() ) );
    }

    if( data->hasFormat( AmarokMimeData::PLAYLISTBROWSERGROUP_MIME ) )
    {
        debug() << "Found playlist group mime type";

        const AmarokMimeData* playlistGroupDrag = dynamic_cast<const AmarokMimeData*>( data );
        if( playlistGroupDrag )
        {

            SqlPlaylistGroupList groups = playlistGroupDrag->sqlPlaylistsGroups();

            foreach( SqlPlaylistGroupPtr group, groups ) {
                group->reparent( parentGroup );
            }

            reloadFromDb();

            return true;
        }
    }
    else if( data->hasFormat( AmarokMimeData::PLAYLIST_MIME ) )
    {
        debug() << "Found playlist mime type";

        const AmarokMimeData* dragList = dynamic_cast<const AmarokMimeData*>( data );
        if( dragList )
        {

            Meta::PlaylistList playlists = dragList->playlists();

            foreach( Meta::PlaylistPtr playlistPtr, playlists ) {

                Meta::SqlPlaylistPtr playlist = Meta::SqlPlaylistPtr::dynamicCast( playlistPtr );

                if( playlist ) 
                    playlist->reparent( parentGroup );
            }

            reloadFromDb();

            return true;
        }
    }

    return false;
}

void PlaylistBrowserNS::UserModel::createTables()
{
    DEBUG_BLOCK;

    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
    sqlStorage->query( QString( "CREATE TABLE playlist_groups ("
            " id " + sqlStorage->idType() +
            ", parent_id INTEGER"
            ", name " + sqlStorage->textColumnType() +
            ", description " + sqlStorage->textColumnType() + " );" ) );
    sqlStorage->query( "CREATE INDEX parent_podchannel ON playlist_groups( parent_id );" );


    sqlStorage->query( QString( "CREATE TABLE playlists ("
            " id " + sqlStorage->idType() +
            ", parent_id INTEGER"
            ", name " + sqlStorage->textColumnType() +
            ", description " + sqlStorage->textColumnType() + " );" ) );
    sqlStorage->query( "CREATE INDEX parent_playlist ON playlists( parent_id );" );

    sqlStorage->query( QString( "CREATE TABLE playlist_tracks ("
            " id " + sqlStorage->idType() +
            ", playlist_id INTEGER "
            ", track_num INTEGER "
            ", url " + sqlStorage->exactTextColumnType() +
            ", title " + sqlStorage->textColumnType() +
            ", album " + sqlStorage->textColumnType() +
            ", artist " + sqlStorage->textColumnType() +
            ", length INTEGER "
            ", uniqueid " + sqlStorage->textColumnType(128) + ");" ) );

    sqlStorage->query( "CREATE INDEX parent_playlist_tracks ON playlist_tracks( playlist_id );" );
    sqlStorage->query( "CREATE INDEX playlist_tracks_uniqueid ON playlist_tracks( uniqueid );" );

    sqlStorage->query( "INSERT INTO admin(component,version) "
            "VALUES('" + key + "'," + QString::number( USERPLAYLIST_DB_VERSION ) + ");" );

}

void
PlaylistBrowserNS::UserModel::reloadFromDb()
{
    DEBUG_BLOCK;
    reset();
    m_root->clear();
}

void
PlaylistBrowserNS::UserModel::editPlaylist( int id )
{

  //for now, assume that the newly added playlist is in the top level:
    int row = m_root->childGroups().count() - 1;
    foreach ( Meta::SqlPlaylistPtr playlist, m_root->childPlaylists() ) {
        row++;
        if ( playlist->id() == id ) {
            emit editIndex( createIndex( row , 0, SqlPlaylistViewItemPtr::staticCast( playlist ) ) );
        }
    }
}

void
PlaylistBrowserNS::UserModel::createNewGroup()
{
    DEBUG_BLOCK

    SqlPlaylistGroup * group = new SqlPlaylistGroup( i18n("New Group"), m_root );
    group->save();
    int id = group->id();
    delete group;

    reloadFromDb();

    int row = 0;
    foreach ( SqlPlaylistGroupPtr childGroup, m_root->childGroups() ) {
        if ( childGroup->id() == id )
        {
            debug() << "emmiting edit for " << childGroup->name() << " id " << childGroup->id() << " in row " << row;
            emit editIndex( createIndex( row , 0, SqlPlaylistViewItemPtr::staticCast( childGroup ) ) );
        }
        row++;
    }

} 

void
PlaylistBrowserNS::UserModel::createNewStream( const QString& streamName, const Meta::TrackPtr& streamTrack )
{
    Meta::TrackList list;
    list.append( streamTrack );
    Meta::SqlPlaylist *stream = new Meta::SqlPlaylist( streamName, list, m_root );
    delete stream;
    reloadFromDb();
}

#include "UserPlaylistModel.moc"
