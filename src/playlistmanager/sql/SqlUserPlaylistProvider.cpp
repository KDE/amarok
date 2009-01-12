/***************************************************************************
 *   Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>     *
 *   Copyright (c) 2008 Bart Cerneels <bart.cerneels@kde.org>              *
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

#include "SqlUserPlaylistProvider.h"

#include "Amarok.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "SqlStorage.h"

#include <KUrl>

static const int USERPLAYLIST_DB_VERSION = 2;
static const QString key("AMAROK_USERPLAYLIST");

SqlUserPlaylistProvider::SqlUserPlaylistProvider()
    : UserPlaylistProvider()
{
}

SqlUserPlaylistProvider::~SqlUserPlaylistProvider()
{
}

Meta::PlaylistList
SqlUserPlaylistProvider::playlists()
{
    Meta::PlaylistList playlists;
    foreach( Meta::SqlPlaylistPtr sqlPlaylist, m_playlists )
    {
        playlists << Meta::PlaylistPtr::staticCast( sqlPlaylist );
    }
    return playlists;
}

void
SqlUserPlaylistProvider::save( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
}

void
SqlUserPlaylistProvider::createTables()
{
    DEBUG_BLOCK

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
            ", description " + sqlStorage->textColumnType() +
            ", urlid " + sqlStorage->exactTextColumnType() + " );" ) );
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
}

void
SqlUserPlaylistProvider::deleteTables()
{

    DEBUG_BLOCK

    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();

    sqlStorage->query( "DROP INDEX parent_podchannel ON playlist_groups;" );
    sqlStorage->query( "DROP INDEX parent_playlist ON playlists;" );
    sqlStorage->query( "DROP INDEX parent_playlist_tracks ON playlist_tracks;" );
    sqlStorage->query( "DROP INDEX playlist_tracks_uniqueid ON playlist_tracks;" );

    sqlStorage->query( "DROP TABLE playlist_groups;" );
    sqlStorage->query( "DROP TABLE playlists;" );
    sqlStorage->query( "DROP TABLE playlist_tracks;" );

}

void
SqlUserPlaylistProvider::checkTables()
{
    DEBUG_BLOCK

    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
    QStringList values = sqlStorage->query( QString("SELECT version FROM admin WHERE component = '%1';").arg(sqlStorage->escape( key ) ) );
    if( values.isEmpty() )
    {
        //debug() << "creating Playlist Tables";
        createTables();

        sqlStorage->query( "INSERT INTO admin(component,version) "
                "VALUES('" + key + "'," + QString::number( USERPLAYLIST_DB_VERSION ) + ");" );
    }
    else
    {
        int dbVersion = values.at( 0 ).toInt();
        if ( dbVersion != USERPLAYLIST_DB_VERSION ) {
            //ah screw it, we do not have any stable releases of this out, so just redo the db. This wil also make sure that we do not
            //get duplicate playlists from files due to one having a urlid and the other not having one
            deleteTables();
            createTables();

            sqlStorage->query( "UPDATE admin SET version = '" + QString::number( USERPLAYLIST_DB_VERSION )  + "' WHERE component = '" + key + "';" );
        }
    }
}

void
SqlUserPlaylistProvider::reloadFromDb()
{
    DEBUG_BLOCK;
//     reset();
//     m_root->clear();
}

#include "SqlUserPlaylistProvider.moc"
