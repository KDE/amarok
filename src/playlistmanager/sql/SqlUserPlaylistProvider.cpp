/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "SqlUserPlaylistProvider.h"

#include <core/storage/SqlStorage.h>
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/storage/StorageManager.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"

#include <QMap>

#include <KLocalizedString>
#include <KMessageBox>

static const int USERPLAYLIST_DB_VERSION = 3;
// a database updater has been added in checkTables(). Use that when updating db version
static const QString key(QStringLiteral("AMAROK_USERPLAYLIST"));

namespace Playlists {

SqlUserPlaylistProvider::SqlUserPlaylistProvider( bool debug )
    : UserPlaylistProvider()
    , m_debug( debug )
{
    checkTables();
    m_root = Playlists::SqlPlaylistGroupPtr( new Playlists::SqlPlaylistGroup( QString(),
            Playlists::SqlPlaylistGroupPtr(), this ) );
}

SqlUserPlaylistProvider::~SqlUserPlaylistProvider()
{
}

int
SqlUserPlaylistProvider::playlistCount() const
{
    return m_root->childSqlPlaylists().count();
}

Playlists::PlaylistList
SqlUserPlaylistProvider::playlists()
{
    Playlists::PlaylistList playlists;
    for( Playlists::SqlPlaylistPtr sqlPlaylist : m_root->allChildPlaylists() )
    {
        playlists << Playlists::PlaylistPtr::staticCast( sqlPlaylist );
    }
    return playlists;
}

void
SqlUserPlaylistProvider::renamePlaylist(PlaylistPtr playlist, const QString &newName )
{
    playlist->setName( newName.trimmed() );
}

bool
SqlUserPlaylistProvider::isWritable()
{
    return true;
}

bool
SqlUserPlaylistProvider::deletePlaylists( const Playlists::PlaylistList &playlistList )
{
    Playlists::SqlPlaylistList sqlPlaylists;
    for( Playlists::PlaylistPtr playlist : playlistList )
    {
        Playlists::SqlPlaylistPtr sqlPlaylist =
            Playlists::SqlPlaylistPtr::dynamicCast( playlist );
        if( !sqlPlaylist.isNull() )
            sqlPlaylists << sqlPlaylist;
    }
    return deleteSqlPlaylists( sqlPlaylists );
}

bool
SqlUserPlaylistProvider::deleteSqlPlaylists( Playlists::SqlPlaylistList playlistList )
{
    //this delete is not confirmed, has to be done by the slot connected to the delete action.
    for( Playlists::SqlPlaylistPtr sqlPlaylist : playlistList )
    {
        if( sqlPlaylist )
        {
            debug() << "deleting " << sqlPlaylist->name();
            m_root->m_childPlaylists.removeAll( sqlPlaylist );
            Q_EMIT playlistRemoved( Playlists::PlaylistPtr::dynamicCast( sqlPlaylist ) );
            sqlPlaylist->removeFromDb();
        }
    }

    return true;
}

Playlists::PlaylistPtr
SqlUserPlaylistProvider::save( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    QString name = QLocale().toString( QDateTime::currentDateTime(), QLocale::LongFormat );
    return save( tracks, name );
}

Playlists::PlaylistPtr
SqlUserPlaylistProvider::save( const Meta::TrackList &tracks, const QString& name )
{
    DEBUG_BLOCK
    debug() << "saving " << tracks.count() << " tracks to db with name" << name;
    Playlists::SqlPlaylistPtr sqlPlaylist = Playlists::SqlPlaylistPtr(
            new Playlists::SqlPlaylist( name, tracks,
                Playlists::SqlPlaylistGroupPtr(),
                this )
            );
    m_root->m_childPlaylists << sqlPlaylist;
    Playlists::PlaylistPtr playlist( sqlPlaylist.data() );

    Q_EMIT playlistAdded( playlist );
    return playlist; // assumes insertion in db was successful!
}

void
SqlUserPlaylistProvider::reloadFromDb()
{
    DEBUG_BLOCK;
    m_root->clear();
    Q_EMIT updated();
}

Playlists::SqlPlaylistGroupPtr
SqlUserPlaylistProvider::group( const QString &name )
{
    DEBUG_BLOCK
    Playlists::SqlPlaylistGroupPtr newGroup;

    if( name.isEmpty() )
        return m_root;

    //clear the root first to force a reload.
    m_root->clear();

    for( const Playlists::SqlPlaylistGroupPtr &group : m_root->allChildGroups() )
    {
        debug() << group->name();
        if( group->name() == name )
        {
            debug() << "match";
            return group;
        }
    }

    debug() << "Creating a new group " << name;
    newGroup = new Playlists::SqlPlaylistGroup( name, m_root, this );
    newGroup->save();

    return newGroup;
}

void
SqlUserPlaylistProvider::createTables()
{
    DEBUG_BLOCK

    auto sqlStorage = StorageManager::instance()->sqlStorage();
    if( !sqlStorage )
    {
        debug() << "No SQL Storage available!";
        return;
    }
    sqlStorage->query( QStringLiteral( "CREATE TABLE playlist_groups ("
            " id ") + sqlStorage->idType() +
            QStringLiteral(", parent_id INTEGER"
            ", name ") + sqlStorage->textColumnType() +
            QStringLiteral(", description ") + sqlStorage->textColumnType() + QStringLiteral(" ) ENGINE = MyISAM;" ) );
    sqlStorage->query( QStringLiteral("CREATE INDEX parent_podchannel ON playlist_groups( parent_id );") );


    sqlStorage->query( QStringLiteral( "CREATE TABLE playlists ("
            " id ") + sqlStorage->idType() +
            QStringLiteral(", parent_id INTEGER"
            ", name ") + sqlStorage->textColumnType() +
            QStringLiteral(", urlid ") + sqlStorage->exactTextColumnType() + QStringLiteral(" ) ENGINE = MyISAM;" ) );
    sqlStorage->query( QStringLiteral("CREATE INDEX parent_playlist ON playlists( parent_id );") );

    sqlStorage->query( QStringLiteral( "CREATE TABLE playlist_tracks ("
            " id ") + sqlStorage->idType() +
            QStringLiteral(", playlist_id INTEGER "
            ", track_num INTEGER "
            ", url ") + sqlStorage->exactTextColumnType() +
            QStringLiteral(", title ") + sqlStorage->textColumnType() +
            QStringLiteral(", album ") + sqlStorage->textColumnType() +
            QStringLiteral(", artist ") + sqlStorage->textColumnType() +
            QStringLiteral(", length INTEGER "
            ", uniqueid ") + sqlStorage->textColumnType(128) + QStringLiteral(") ENGINE = MyISAM;" ) );

    sqlStorage->query( QStringLiteral("CREATE INDEX parent_playlist_tracks ON playlist_tracks( playlist_id );") );
    sqlStorage->query( QStringLiteral("CREATE INDEX playlist_tracks_uniqueid ON playlist_tracks( uniqueid );") );
}

void
SqlUserPlaylistProvider::deleteTables()
{
    DEBUG_BLOCK

    auto sqlStorage = StorageManager::instance()->sqlStorage();

    if( !sqlStorage )
    {
        debug() << "No SQL Storage available!";
        return;
    }

    sqlStorage->query( QStringLiteral("DROP INDEX parent_podchannel ON playlist_groups;") );
    sqlStorage->query( QStringLiteral("DROP INDEX parent_playlist ON playlists;") );
    sqlStorage->query( QStringLiteral("DROP INDEX parent_playlist_tracks ON playlist_tracks;") );
    sqlStorage->query( QStringLiteral("DROP INDEX playlist_tracks_uniqueid ON playlist_tracks;") );

    sqlStorage->query( QStringLiteral("DROP TABLE IF EXISTS playlist_groups;") );
    sqlStorage->query( QStringLiteral("DROP TABLE IF EXISTS playlists;") );
    sqlStorage->query( QStringLiteral("DROP TABLE IF EXISTS playlist_tracks;") );

}

void
SqlUserPlaylistProvider::checkTables()
{
    DEBUG_BLOCK

    auto sqlStorage = StorageManager::instance()->sqlStorage();
    QStringList values;

    //Prevents amarok from crashing on bad DB
    if ( !sqlStorage )
	    return;

    values = sqlStorage->query( QStringLiteral("SELECT version FROM admin WHERE component = '%1';").arg(sqlStorage->escape( key ) ) );
    
    if( values.isEmpty() )
    {
        //debug() << "creating Playlist Tables";
        createTables();

        sqlStorage->query( QStringLiteral("INSERT INTO admin(component,version) "
                "VALUES('") + key + QStringLiteral("',") + QString::number( USERPLAYLIST_DB_VERSION ) + QStringLiteral(");") );
    }
    else
    {
        int dbVersion = values.at( 0 ).toInt();
        switch ( dbVersion )
        {
            case 2:
                upgradeVersion2to3();
                sqlStorage->query( QStringLiteral("UPDATE admin SET version = '") + QString::number( USERPLAYLIST_DB_VERSION )  + QStringLiteral("' WHERE component = '") + key + QStringLiteral("';") );
            case 3: // current version
               break;
            default:
                KMessageBox::error(
                    nullptr, // QWidget *parent
                    i18n( "Version %1 of playlist database schema encountered, however this "
                        "Amarok version only supports version %2 (and previous versions "
                        "starting with %2). Playlists saved in the Amarok Database probably "
                        "will not work and any write operations with them may result in losing "
                        "them. Perhaps you have started an older version of Amarok with a "
                        "database written by newer version?", dbVersion, USERPLAYLIST_DB_VERSION ),
                    i18nc( "the user's 'database version' is newer and unsupported by this software version",
                           "Future version of Playlist Database?" ) );
         }
     }
 }

void
SqlUserPlaylistProvider::upgradeVersion2to3()
{
    DEBUG_BLOCK
    auto sqlStorage = StorageManager::instance()->sqlStorage();
    sqlStorage->query( QStringLiteral("ALTER TABLE playlists DROP COLUMN description") );
}

Playlists::SqlPlaylistList
SqlUserPlaylistProvider::toSqlPlaylists( Playlists::PlaylistList playlists )
{
    Playlists::SqlPlaylistList sqlPlaylists;
    for( Playlists::PlaylistPtr playlist : playlists )
    {
        Playlists::SqlPlaylistPtr sqlPlaylist =
            Playlists::SqlPlaylistPtr::dynamicCast( playlist );
        if( !sqlPlaylist.isNull() )
            sqlPlaylists << sqlPlaylist;
    }
    return sqlPlaylists;
}

} //namespace Playlists

