/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "DatabaseUpdater.h"

#include "amarokconfig.h"
#include "core/support/Debug.h"
#include "core/collections/support/SqlStorage.h"
#include "SqlCollection.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMultiMap>
#include <QTextStream>

#include <KGlobal>
#include <KMessageBox>

static const int DB_VERSION = 13;

int
DatabaseUpdater::expectedDatabaseVersion()
{
    return DB_VERSION;
}

DatabaseUpdater::DatabaseUpdater( Collections::SqlCollection *collection )
    : m_collection( collection )
    , m_debugDatabaseContent( false )
{
    m_debugDatabaseContent = KGlobal::config()->group( "SqlCollection" ).readEntry( "DebugDatabaseContent", false );
}

DatabaseUpdater::~DatabaseUpdater()
{
    //nothing to do
}

bool
DatabaseUpdater::needsUpdate() const
{
    return adminValue( "DB_VERSION" ) != DB_VERSION;
}

bool
DatabaseUpdater::update()
{
    DEBUG_BLOCK
    int dbVersion = adminValue( "DB_VERSION" );

    debug() << "Database version: " << dbVersion;

    if( dbVersion == 0 )
    {
        createTables();
        QString query = QString( "INSERT INTO admin(component, version) VALUES ('DB_VERSION', %1);" ).arg( DB_VERSION );
        m_collection->sqlStorage()->query( query );
    }
    else if( dbVersion < DB_VERSION )
    {
        debug() << "Database out of date: database version is" << dbVersion << ", current version is" << DB_VERSION;
        if ( dbVersion == 1 && dbVersion < DB_VERSION )
        {
            upgradeVersion1to2();
            dbVersion = 2;
        }
        if( dbVersion == 2 && dbVersion < DB_VERSION )
        {
            upgradeVersion2to3();
            dbVersion = 3;
        }
        if( dbVersion == 3 && dbVersion < DB_VERSION )
        {
            upgradeVersion3to4();
            dbVersion = 4;
        }
        if( dbVersion == 4 && dbVersion < DB_VERSION )
        {
            upgradeVersion4to5();
            dbVersion = 5;
        }
        if( dbVersion == 5 && dbVersion < DB_VERSION )
        {
            upgradeVersion5to6();
            dbVersion = 6;
        }
        if( dbVersion == 6 && dbVersion < DB_VERSION )
        {
            upgradeVersion6to7();
            dbVersion = 7;
        }
        if( dbVersion == 7 && dbVersion < DB_VERSION )
        {
            upgradeVersion7to8();
            dbVersion = 8;
        }
        if( dbVersion == 8 && dbVersion < DB_VERSION )
        {
            //removes stray rows from albums that were caused by the initial full scan
            upgradeVersion8to9();
            dbVersion = 9;
        }
        if( dbVersion == 9 && dbVersion < DB_VERSION )
        {
            //removes stray rows from albums that were caused by the initial full scan
            upgradeVersion9to10();
            dbVersion = 10;
        }
        if( dbVersion == 10 && dbVersion < DB_VERSION )
        {
            upgradeVersion10to11();
            dbVersion = 11;
        }
        if( dbVersion == 11 && dbVersion < DB_VERSION )
        {
            upgradeVersion11to12();
            dbVersion = 12;
        }
        if( dbVersion == 12 && dbVersion < DB_VERSION )
        {
            upgradeVersion12to13();
            dbVersion = 13;
        }
        /*
        if( dbVersion == X && dbVersion < DB_VERSION )
        {
            upgradeVersionXtoY();
            dbVersion = Y;
        }
        */
        QString query = QString( "UPDATE admin SET version = %1 WHERE component = 'DB_VERSION';" ).arg( dbVersion );
        m_collection->sqlStorage()->query( query );

        //NOTE: A rescan will be triggered automatically as a result of an upgrade.  Don't trigger it here, as the
        //collection isn't fully initialized and this will trigger a crash/assert.
    }
    else if( dbVersion == DB_VERSION )
    {
        return false; // no update needed
    }
    else if( dbVersion > DB_VERSION )
    {
        KMessageBox::error(0,
                "<p>The Amarok collection database was created by a newer version of Amarok, "
                "and this version of Amarok cannot use it.</p>",
                "Database Type Unknown");
        // FIXME: maybe we should tell them how to delete the database?
        // FIXME: exit() may be a little harsh, but QCoreApplication::exit() doesn't seem to work
        exit(1);
    }

    return true;
}

void
DatabaseUpdater::upgradeVersion1to2()
{
    DEBUG_BLOCK

    m_collection->sqlStorage()->query( "ALTER TABLE tracks "
                         "ADD COLUMN albumgain FLOAT, "
                         "ADD COLUMN albumpeakgain FLOAT, "
                         "ADD COLUMN trackgain FLOAT,"
                         "ADD COLUMN trackpeakgain FLOAT;" );
}

void
DatabaseUpdater::upgradeVersion2to3()
{
    DEBUG_BLOCK;

    SqlStorage *storage = m_collection->sqlStorage();
    storage->query( "DROP TABLE devices;" );

    QString create = "CREATE TABLE devices "
                     "(id " + storage->idType() +
                     ",type " + storage->textColumnType() +
                     ",label " + storage->textColumnType() +
                     ",lastmountpoint " + storage->textColumnType() +
                     ",uuid " + storage->textColumnType() +
                     ",servername " + storage->textColumnType() +
                     ",sharename " + storage->textColumnType() + ");";
    storage->query( create );
    storage->query( "CREATE INDEX devices_type ON devices( type );" );
    storage->query( "CREATE UNIQUE INDEX devices_uuid ON devices( uuid );" );
    storage->query( "CREATE INDEX devices_rshare ON devices( servername, sharename );" );

}

void
DatabaseUpdater::upgradeVersion3to4()
{
    SqlStorage *storage = m_collection->sqlStorage();

    storage->query( "CREATE TABLE statistics_permanent "
                         "(url " + storage->exactTextColumnType() +
                         ",firstplayed DATETIME"
                         ",lastplayed DATETIME"
                         ",score FLOAT"
                         ",rating INTEGER DEFAULT 0"
                         ",playcount INTEGER)" );
    storage->query( "CREATE UNIQUE INDEX ON statistics_permanent(url)" );
    //Note: the above index query is invalid, but kept here for posterity

    storage->query( "CREATE TABLE statistics_tag "
                         "(name " + storage->textColumnType() +
                         ",artist " + storage->textColumnType() +
                         ",album " + storage->textColumnType() +
                         ",firstplayed DATETIME"
                         ",lastplayed DATETIME"
                         ",score FLOAT"
                         ",rating INTEGER DEFAULT 0"
                         ",playcount INTEGER)" );
    storage->query( "CREATE UNIQUE INDEX ON statistics_tag(name,artist,album)" );
    //Note: the above index query is invalid, but kept here for posterity
}

void
DatabaseUpdater::upgradeVersion4to5()
{
    SqlStorage *storage = m_collection->sqlStorage();

    //first the database
    storage->query( "ALTER DATABASE amarok DEFAULT CHARACTER SET utf8 DEFAULT COLLATE utf8_unicode_ci" );

    //now the tables

    //first, drop tables that can easily be recreated by doing an update
    QStringList dropTables;
    dropTables << "jamendo_albums" << "jamendo_artists" << "jamendo_genre" << "jamendo_tracks";
    dropTables << "magnatune_albums" << "magnatune_artists" << "magnatune_genre" << "magnatune_moods" << "magnatune_tracks";
    dropTables << "opmldirectory_albums" << "opmldirectory_artists" << "opmldirectory_genre" << "opmldirectory_tracks";

    foreach( const QString &table, dropTables )
        storage->query( "DROP TABLE " + table );

    //now, the rest of them
    QStringList tables;
    tables << "admin" << "albums" << "amazon" << "artists" << "bookmark_groups" << "bookmarks";
    tables << "composers" << "devices" << "directories" << "genres" << "images" << "labels" << "lyrics";
    tables << "playlist_groups" << "playlist_tracks" << "playlists";
    tables << "podcastchannels" << "podcastepisodes";
    tables << "statistics" << "statistics_permanent" << "statistics_tag";
    tables << "tracks" << "urls" << "urls_labels" << "years";

    foreach( const QString &table, tables )
        storage->query( "ALTER TABLE " + table + " DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci" );

    //now the columns (ugh)
    //first, varchar
    typedef QPair<QString, int> vcpair;
    QMultiMap<QString, vcpair> columns;
    columns.insert( "admin", vcpair( "component", 255 ) );
    columns.insert( "albums", vcpair( "name", 255 ) );
    columns.insert( "amazon", vcpair( "asin", 20 ) );
    columns.insert( "amazon", vcpair( "locale", 2 ) );
    columns.insert( "amazon", vcpair( "filename", 33 ) );
    columns.insert( "artists", vcpair( "name", 255 ) );
    columns.insert( "bookmark_groups", vcpair( "name", 255 ) );
    columns.insert( "bookmark_groups", vcpair( "description", 255 ) );
    columns.insert( "bookmark_groups", vcpair( "custom", 255 ) );
    columns.insert( "bookmarks", vcpair( "name", 255 ) );
    columns.insert( "bookmarks", vcpair( "url", 1024 ) );
    columns.insert( "bookmarks", vcpair( "description", 1024 ) );
    columns.insert( "bookmarks", vcpair( "custom", 255 ) );
    columns.insert( "composers", vcpair( "name", 255 ) );
    columns.insert( "devices", vcpair( "type", 255 ) );
    columns.insert( "devices", vcpair( "label", 255 ) );
    columns.insert( "devices", vcpair( "lastmountpoint", 255 ) );
    columns.insert( "devices", vcpair( "uuid", 255 ) );
    columns.insert( "devices", vcpair( "servername", 255 ) );
    columns.insert( "devices", vcpair( "sharename", 255 ) );
    columns.insert( "directories", vcpair( "dir", 1024 ) );
    columns.insert( "genres", vcpair( "name", 255 ) );
    columns.insert( "images", vcpair( "path", 255 ) );
    columns.insert( "labels", vcpair( "label", 255 ) );
    columns.insert( "lyrics", vcpair( "url", 1024 ) );
    columns.insert( "playlist_groups", vcpair( "name", 255 ) );
    columns.insert( "playlist_groups", vcpair( "description", 255 ) );
    columns.insert( "playlist_tracks", vcpair( "url", 1024 ) );
    columns.insert( "playlist_tracks", vcpair( "title", 255 ) );
    columns.insert( "playlist_tracks", vcpair( "album", 255 ) );
    columns.insert( "playlist_tracks", vcpair( "artist", 255 ) );
    columns.insert( "playlist_tracks", vcpair( "uniqueid", 128 ) );
    columns.insert( "playlists", vcpair( "name", 255 ) );
    columns.insert( "playlists", vcpair( "description", 255 ) );
    columns.insert( "playlists", vcpair( "urlid", 1024 ) );
    columns.insert( "podcastchannels", vcpair( "copyright", 255 ) );
    columns.insert( "podcastchannels", vcpair( "directory", 255 ) );
    columns.insert( "podcastchannels", vcpair( "labels", 255 ) );
    columns.insert( "podcastchannels", vcpair( "subscribedate", 255 ) );
    columns.insert( "podcastepisodes", vcpair( "guid", 1024 ) );
    columns.insert( "podcastepisodes", vcpair( "mimetype", 255 ) );
    columns.insert( "podcastepisodes", vcpair( "pubdate", 255 ) );
    columns.insert( "statistics_permanent", vcpair( "url", 1024 ) );
    columns.insert( "statistics_tag", vcpair( "name", 255 ) );
    columns.insert( "statistics_tag", vcpair( "artist", 255 ) );
    columns.insert( "tracks", vcpair( "title", 255 ) );
    columns.insert( "urls", vcpair( "rpath", 1024 ) );
    columns.insert( "urls", vcpair( "uniqueid", 128 ) );
    columns.insert( "years", vcpair( "name", 255 ) );

    QMultiMap<QString, vcpair>::const_iterator i, iEnd;
    for( i = columns.constBegin(), iEnd = columns.constEnd(); i != iEnd; ++i )
    {
        storage->query( "ALTER TABLE " + i.key() + " MODIFY " + i.value().first + " VARBINARY(" + QString::number( i.value().second ) + ')' );
        storage->query( "ALTER IGNORE TABLE " + i.key() + " MODIFY " + i.value().first + \
            " VARCHAR(" + QString::number( i.value().second ) + ") CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL" );
    }

    columns.clear();

    //text fields, not varchars
    columns.insert( "lyrics", vcpair( "lyrics", 0 ) );
    columns.insert( "podcastchannels", vcpair( "url", 0 ) );
    columns.insert( "podcastchannels", vcpair( "title", 0 ) );
    columns.insert( "podcastchannels", vcpair( "weblink", 0 ) );
    columns.insert( "podcastchannels", vcpair( "image", 0 ) );
    columns.insert( "podcastchannels", vcpair( "description", 0 ) );
    columns.insert( "podcastepisodes", vcpair( "url", 0 ) );
    columns.insert( "podcastepisodes", vcpair( "localurl", 0 ) );
    columns.insert( "podcastepisodes", vcpair( "title", 0 ) );
    columns.insert( "podcastepisodes", vcpair( "subtitle", 0 ) );
    columns.insert( "podcastepisodes", vcpair( "description", 0 ) );
    columns.insert( "tracks", vcpair( "comment", 0 ) );

    storage->query( "DROP INDEX url_podchannel ON podcastchannels" );
    storage->query( "DROP INDEX url_podepisode ON podcastepisodes" );
    storage->query( "DROP INDEX localurl_podepisode ON podcastepisodes" );
    for( i = columns.constBegin(), iEnd = columns.constEnd(); i != iEnd; ++i )
    {
        storage->query( "ALTER TABLE " + i.key() + " MODIFY " + i.value().first + " BLOB" );
        storage->query( "ALTER IGNORE TABLE " + i.key() + " MODIFY " + i.value().first + " TEXT CHARACTER SET utf8 NOT NULL" );
    }
    storage->query( "CREATE FULLTEXT INDEX url_podchannel ON podcastchannels( url )" );
    storage->query( "CREATE FULLTEXT INDEX url_podepisode ON podcastepisodes( url )" );
    storage->query( "CREATE FULLTEXT INDEX localurl_podepisode ON podcastepisodes( localurl )" );
}

void
DatabaseUpdater::upgradeVersion5to6()
{
    DEBUG_BLOCK

    SqlStorage *storage = m_collection->sqlStorage();

    //first, drop tables that can easily be recreated by doing an update
    QStringList dropTables;
    dropTables << "jamendo_albums" << "jamendo_artists" << "jamendo_genre" << "jamendo_tracks";
    dropTables << "magnatune_albums" << "magnatune_artists" << "magnatune_genre" << "magnatune_moods" << "magnatune_tracks";
    dropTables << "opmldirectory_albums" << "opmldirectory_artists" << "opmldirectory_genre" << "opmldirectory_tracks";

    foreach( const QString &table, dropTables )
        storage->query( "DROP TABLE " + table );

    //now, the rest of them
    QStringList tables;
    tables << "admin" << "albums" << "amazon" << "artists" << "bookmark_groups" << "bookmarks";
    tables << "composers" << "devices" << "directories" << "genres" << "images" << "labels" << "lyrics";
    tables << "playlist_groups" << "playlist_tracks" << "playlists";
    tables << "podcastchannels" << "podcastepisodes";
    tables << "statistics" << "statistics_permanent" << "statistics_tag";
    tables << "tracks" << "urls" << "urls_labels" << "years";

    foreach( const QString &table, tables )
        storage->query( "ALTER TABLE " + table + " ENGINE = MyISAM" );

    typedef QPair<QString, int> vcpair;
    QMultiMap<QString, vcpair> columns;
    columns.insert( "bookmarks", vcpair( "url", 1000 ) );
    columns.insert( "bookmarks", vcpair( "description", 1000 ) );
    columns.insert( "directories", vcpair( "dir", 1000 ) );
    columns.insert( "lyrics", vcpair( "url", 324 ) );
    columns.insert( "playlist_tracks", vcpair( "url", 1000 ) );
    columns.insert( "playlists", vcpair( "urlid", 1000 ) );
    columns.insert( "podcastepisodes", vcpair( "guid", 1000 ) );
    columns.insert( "statistics_permanent", vcpair( "url", 324 ) );
    columns.insert( "urls", vcpair( "rpath", 324 ) );
    columns.insert( "devices", vcpair( "servername", 80 ) );
    columns.insert( "devices", vcpair( "sharename", 240 ) );
    columns.insert( "statistics_tag", vcpair( "name", 108 ) );
    columns.insert( "statistics_tag", vcpair( "artist", 108 ) );
    columns.insert( "statistics_tag", vcpair( "album", 108 ) );

    QMultiMap<QString, vcpair>::const_iterator i, iEnd;

    for( i = columns.constBegin(), iEnd = columns.constEnd(); i != iEnd; ++i )
        storage->query( "ALTER IGNORE TABLE " + i.key() + " MODIFY " + i.value().first + " VARCHAR(" + QString::number( i.value().second ) + ") " );

    storage->query( "CREATE INDEX devices_rshare ON devices( servername, sharename );" );
    storage->query( "CREATE UNIQUE INDEX lyrics_url ON lyrics(url);" );
    storage->query( "CREATE UNIQUE INDEX urls_id_rpath ON urls(deviceid, rpath);" );
    storage->query( "CREATE UNIQUE INDEX stats_tag_name_artist_album ON statistics_tag(name,artist,album)" );
}

void
DatabaseUpdater::upgradeVersion6to7()
{
    DEBUG_BLOCK

    SqlStorage *storage = m_collection->sqlStorage();

    typedef QPair<QString, int> vcpair;
    QMultiMap<QString, vcpair> columns;
    columns.insert( "directories", vcpair( "dir", 1000 ) );
    columns.insert( "urls", vcpair( "rpath", 324 ) );
    columns.insert( "statistics_permanent", vcpair( "url", 324 ) );

    QMultiMap<QString, vcpair>::const_iterator i, iEnd;

    for( i = columns.constBegin(), iEnd = columns.constEnd(); i != iEnd; ++i )
    {
        storage->query( "ALTER IGNORE TABLE " + i.key() + " MODIFY " + i.value().first + \
            " VARCHAR(" + QString::number( i.value().second ) + ") COLLATE utf8_bin NOT NULL" );
    }

    columns.clear();

}


void
DatabaseUpdater::upgradeVersion7to8()
{
    DEBUG_BLOCK

    SqlStorage *storage = m_collection->sqlStorage();

    QHash< int, int > trackLengthHash;

    // First, get the lengths from the db and insert them into a hash
    const QStringList result = storage->query( "SELECT id, length FROM tracks" );

    QListIterator<QString> iter(result);
    while( iter.hasNext() )
        trackLengthHash.insert( iter.next().toInt(), iter.next().toInt() );

    // Now Iterate over the hash, and insert each track back in, changing the length to milliseconds
    QHashIterator<int,int> iter2( trackLengthHash );
    const QString updateString = QString( "UPDATE tracks SET length=%1 WHERE id=%2 ;");
    while( iter2.hasNext() )
    {
        iter2.next();
        debug() << "Running the following query: " << updateString.arg( QString::number( iter2.value() * 1000 ), QString::number( iter2.key() ) );
        storage->query( updateString.arg( QString::number( iter2.value() * 1000 ), QString::number( iter2.key() ) ) );
    }
}

void
DatabaseUpdater::upgradeVersion8to9()
{
    deleteAllRedundant( "album" );
}

void
DatabaseUpdater::upgradeVersion9to10()
{
    DEBUG_BLOCK

    SqlStorage *storage = m_collection->sqlStorage();

    //first the database
    storage->query( "ALTER DATABASE amarok DEFAULT CHARACTER SET utf8 DEFAULT COLLATE utf8_bin" );

    //now the tables

    //first, drop tables that can easily be recreated by doing an update
    QStringList dropTables;
    dropTables << "jamendo_albums" << "jamendo_artists" << "jamendo_genre" << "jamendo_tracks";
    dropTables << "magnatune_albums" << "magnatune_artists" << "magnatune_genre" << "magnatune_moods" << "magnatune_tracks";
    dropTables << "opmldirectory_albums" << "opmldirectory_artists" << "opmldirectory_genre" << "opmldirectory_tracks";

    foreach( const QString &table, dropTables )
        storage->query( "DROP TABLE " + table );

    //now, the rest of them
    QStringList tables;
    tables << "admin" << "albums" << "amazon" << "artists" << "bookmark_groups" << "bookmarks";
    tables << "composers" << "devices" << "directories" << "genres" << "images" << "labels" << "lyrics";
    tables << "playlist_groups" << "playlist_tracks" << "playlists";
    tables << "podcastchannels" << "podcastepisodes";
    tables << "statistics" << "statistics_permanent" << "statistics_tag";
    tables << "tracks" << "urls" << "urls_labels" << "years";

    foreach( const QString &table, tables )
        storage->query( "ALTER TABLE " + table + " DEFAULT CHARACTER SET utf8 DEFAULT COLLATE utf8_bin COLLATE utf8_bin ENGINE = MyISAM" );

    //now the columns (ugh)
    //first, varchar
    typedef QPair<QString, int> vcpair;
    QMultiMap<QString, vcpair> columns;
    columns.insert( "admin", vcpair( "component", 255 ) );
    columns.insert( "albums", vcpair( "name", 255 ) );
    columns.insert( "amazon", vcpair( "asin", 20 ) );
    columns.insert( "amazon", vcpair( "locale", 2 ) );
    columns.insert( "amazon", vcpair( "filename", 33 ) );
    columns.insert( "artists", vcpair( "name", 255 ) );
    columns.insert( "bookmark_groups", vcpair( "name", 255 ) );
    columns.insert( "bookmark_groups", vcpair( "description", 255 ) );
    columns.insert( "bookmark_groups", vcpair( "custom", 255 ) );
    columns.insert( "bookmarks", vcpair( "name", 255 ) );
    columns.insert( "bookmarks", vcpair( "url", 1000 ) );
    columns.insert( "bookmarks", vcpair( "description", 1000 ) );
    columns.insert( "bookmarks", vcpair( "custom", 255 ) );
    columns.insert( "composers", vcpair( "name", 255 ) );
    columns.insert( "devices", vcpair( "type", 255 ) );
    columns.insert( "devices", vcpair( "label", 255 ) );
    columns.insert( "devices", vcpair( "lastmountpoint", 255 ) );
    columns.insert( "devices", vcpair( "uuid", 255 ) );
    columns.insert( "devices", vcpair( "servername", 80 ) );
    columns.insert( "devices", vcpair( "sharename", 240 ) );
    columns.insert( "directories", vcpair( "dir", 1000 ) );
    columns.insert( "genres", vcpair( "name", 255 ) );
    columns.insert( "images", vcpair( "path", 255 ) );
    columns.insert( "labels", vcpair( "label", 255 ) );
    columns.insert( "lyrics", vcpair( "url", 324 ) );
    columns.insert( "playlist_groups", vcpair( "name", 255 ) );
    columns.insert( "playlist_groups", vcpair( "description", 255 ) );
    columns.insert( "playlist_tracks", vcpair( "url", 1000 ) );
    columns.insert( "playlist_tracks", vcpair( "title", 255 ) );
    columns.insert( "playlist_tracks", vcpair( "album", 255 ) );
    columns.insert( "playlist_tracks", vcpair( "artist", 255 ) );
    columns.insert( "playlist_tracks", vcpair( "uniqueid", 128 ) );
    columns.insert( "playlists", vcpair( "name", 255 ) );
    columns.insert( "playlists", vcpair( "description", 255 ) );
    columns.insert( "playlists", vcpair( "urlid", 1000 ) );
    columns.insert( "podcastchannels", vcpair( "copyright", 255 ) );
    columns.insert( "podcastchannels", vcpair( "directory", 255 ) );
    columns.insert( "podcastchannels", vcpair( "labels", 255 ) );
    columns.insert( "podcastchannels", vcpair( "subscribedate", 255 ) );
    columns.insert( "podcastepisodes", vcpair( "guid", 1000 ) );
    columns.insert( "podcastepisodes", vcpair( "mimetype", 255 ) );
    columns.insert( "podcastepisodes", vcpair( "pubdate", 255 ) );
    columns.insert( "statistics_permanent", vcpair( "url", 324 ) );
    columns.insert( "statistics_tag", vcpair( "name", 108 ) );
    columns.insert( "statistics_tag", vcpair( "artist", 108 ) );
    columns.insert( "statistics_tag", vcpair( "album", 108 ) );
    columns.insert( "tracks", vcpair( "title", 255 ) );
    columns.insert( "urls", vcpair( "rpath", 324 ) );
    columns.insert( "urls", vcpair( "uniqueid", 128 ) );
    columns.insert( "years", vcpair( "name", 255 ) );

    QMultiMap<QString, vcpair>::const_iterator i, iEnd;

    for( i = columns.constBegin(), iEnd = columns.constEnd(); i != iEnd; ++i )
    {
        storage->query( "ALTER TABLE " + i.key() + " MODIFY " + i.value().first + " VARBINARY(" + QString::number( i.value().second ) + ')' );
        storage->query( "ALTER IGNORE TABLE " + i.key() + " MODIFY " + i.value().first + \
            " VARCHAR(" + QString::number( i.value().second ) + ") CHARACTER SET utf8 COLLATE utf8_bin NOT NULL" );
    }

    storage->query( "CREATE INDEX devices_rshare ON devices( servername, sharename );" );
    storage->query( "CREATE UNIQUE INDEX lyrics_url ON lyrics(url);" );
    storage->query( "CREATE UNIQUE INDEX urls_id_rpath ON urls(deviceid, rpath);" );
    storage->query( "CREATE UNIQUE INDEX stats_tag_name_artist_album ON statistics_tag(name,artist,album)" );

    columns.clear();

    //text fields, not varchars
    columns.insert( "lyrics", vcpair( "lyrics", 0 ) );
    columns.insert( "podcastchannels", vcpair( "url", 0 ) );
    columns.insert( "podcastchannels", vcpair( "title", 0 ) );
    columns.insert( "podcastchannels", vcpair( "weblink", 0 ) );
    columns.insert( "podcastchannels", vcpair( "image", 0 ) );
    columns.insert( "podcastchannels", vcpair( "description", 0 ) );
    columns.insert( "podcastepisodes", vcpair( "url", 0 ) );
    columns.insert( "podcastepisodes", vcpair( "localurl", 0 ) );
    columns.insert( "podcastepisodes", vcpair( "title", 0 ) );
    columns.insert( "podcastepisodes", vcpair( "subtitle", 0 ) );
    columns.insert( "podcastepisodes", vcpair( "description", 0 ) );
    columns.insert( "tracks", vcpair( "comment", 0 ) );

    storage->query( "DROP INDEX url_podchannel ON podcastchannels" );
    storage->query( "DROP INDEX url_podepisode ON podcastepisodes" );
    storage->query( "DROP INDEX localurl_podepisode ON podcastepisodes" );
    for( i = columns.constBegin(), iEnd = columns.constEnd(); i != iEnd; ++i )
    {
        storage->query( "ALTER TABLE " + i.key() + " MODIFY " + i.value().first + " BLOB" );
        storage->query( "ALTER IGNORE TABLE " + i.key() + " MODIFY " + i.value().first + " TEXT CHARACTER SET utf8 COLLATE utf8_bin NOT NULL" );
    }
    storage->query( "CREATE FULLTEXT INDEX url_podchannel ON podcastchannels( url )" );
    storage->query( "CREATE FULLTEXT INDEX url_podepisode ON podcastepisodes( url )" );
    storage->query( "CREATE FULLTEXT INDEX localurl_podepisode ON podcastepisodes( localurl )" );
}

void
DatabaseUpdater::upgradeVersion10to11()
{
    DEBUG_BLOCK
    //OK, this isn't really a database upgrade, but it does affect scanning.
    //New default is for the charset detector not to run; but those that have existing collection
    //won't like it if suddenly that changes their behavior, so set to true for existing collections
    AmarokConfig::setUseCharsetDetector( true );
}

void
DatabaseUpdater::upgradeVersion11to12()
{
    DEBUG_BLOCK
    //Counteract the above -- force it off for everyone except those explicitly enabling it.
    AmarokConfig::setUseCharsetDetector( false );
}

void
DatabaseUpdater::upgradeVersion12to13()
{
    DEBUG_BLOCK
    m_collection->sqlStorage()->query( "UPDATE urls SET uniqueid = REPLACE(uniqueid, 'MB_', 'mb-');" );
}


void
DatabaseUpdater::cleanupDatabase()
{
    // maybe clean up redundant information here?
}

void
DatabaseUpdater::checkTables( bool full )
{
    DEBUG_BLOCK

    SqlStorage *storage = m_collection->sqlStorage();

    QStringList res = storage->query( "SHOW TABLES" );
    if( res.count() > 0 )
    {
        foreach( const QString &table, res )
            storage->query( "CHECK TABLE " + table + ( full ? " EXTENDED;" : " MEDIUM;" ) );
    }
}


void
DatabaseUpdater::createTables() const
{
    DEBUG_BLOCK

    SqlStorage *storage = m_collection->sqlStorage();

    // see docs/database/amarokTables.svg for documentation about database layout
    {
        QString c = "CREATE TABLE admin (component " + storage->textColumnType() + ", version INTEGER) COLLATE = utf8_bin ENGINE = MyISAM;";
        storage->query( c );
    }
    {
        QString create = "CREATE TABLE devices "
                         "(id " + storage->idType() +
                         ",type " + storage->textColumnType() +
                         ",label " + storage->textColumnType() +
                         ",lastmountpoint " + storage->textColumnType() +
                         ",uuid " + storage->textColumnType() +
                         ",servername " + storage->textColumnType(80) +
                         ",sharename " + storage->textColumnType(240) + ") COLLATE = utf8_bin ENGINE = MyISAM;";
        storage->query( create );
        storage->query( "CREATE INDEX devices_type ON devices( type );" );
        storage->query( "CREATE UNIQUE INDEX devices_uuid ON devices( uuid );" );
        storage->query( "CREATE INDEX devices_rshare ON devices( servername, sharename );" );
    }
    {
        QString create = "CREATE TABLE urls "
                         "(id " + storage->idType() +
                         ",deviceid INTEGER"
                         ",rpath " + storage->exactIndexableTextColumnType() + " NOT NULL" +
                         ",directory INTEGER"
                         ",uniqueid " + storage->exactTextColumnType(128) + " UNIQUE) COLLATE = utf8_bin ENGINE = MyISAM;";
        storage->query( create );
        storage->query( "CREATE UNIQUE INDEX urls_id_rpath ON urls(deviceid, rpath);" );
        storage->query( "CREATE INDEX urls_uniqueid ON urls(uniqueid);" );
    }
    {
        QString create = "CREATE TABLE directories "
                         "(id " + storage->idType() +
                         ",deviceid INTEGER"
                         ",dir " + storage->exactTextColumnType() + " NOT NULL" +
                         ",changedate INTEGER) COLLATE = utf8_bin ENGINE = MyISAM;";
        storage->query( create );
        storage->query( "CREATE INDEX directories_deviceid ON directories(deviceid);" );
    }
    {
        QString create = "CREATE TABLE artists "
                         "(id " + storage->idType() +
                         ",name " + storage->textColumnType() + " NOT NULL) COLLATE = utf8_bin ENGINE = MyISAM;";
        storage->query( create );
        storage->query( "CREATE UNIQUE INDEX artists_name ON artists(name);" );
    }
    {
        QString create = "CREATE TABLE images "
                         "(id " + storage->idType() +
                         ",path " + storage->textColumnType() + " NOT NULL) COLLATE = utf8_bin ENGINE = MyISAM;";
        storage->query( create );
        storage->query( "CREATE UNIQUE INDEX images_name ON images(path);" );
    }
    {
        QString c = "CREATE TABLE albums "
                    "(id " + storage->idType() +
                    ",name " + storage->textColumnType() + " NOT NULL"
                    ",artist INTEGER" +
                    ",image INTEGER) COLLATE = utf8_bin ENGINE = MyISAM;";
        storage->query( c );
        storage->query( "CREATE INDEX albums_name ON albums(name);" );
        storage->query( "CREATE INDEX albums_artist ON albums(artist);" );
        storage->query( "CREATE INDEX albums_image ON albums(image);" );
        storage->query( "CREATE UNIQUE INDEX albums_name_artist ON albums(name,artist);" );
        //the index below should not be necessary. uncomment if a query plan shows it is
        //storage->query( "CREATE UNIQUE INDEX albums_artist_name ON albums(artist,name);" );
    }
    {
        QString create = "CREATE TABLE genres "
                         "(id " + storage->idType() +
                         ",name " + storage->textColumnType() + " NOT NULL) COLLATE = utf8_bin ENGINE = MyISAM;";
        storage->query( create );
        storage->query( "CREATE UNIQUE INDEX genres_name ON genres(name);" );
    }
    {
        QString create = "CREATE TABLE composers "
                         "(id " + storage->idType() +
                         ",name " + storage->textColumnType() + " NOT NULL) COLLATE = utf8_bin ENGINE = MyISAM;";
        storage->query( create );
        storage->query( "CREATE UNIQUE INDEX composers_name ON composers(name);" );
    }
    {
        QString create = "CREATE TABLE years "
                         "(id " + storage->idType() +
                         ",name " + storage->textColumnType() + " NOT NULL) COLLATE = utf8_bin ENGINE = MyISAM;";
        storage->query( create );
        storage->query( "CREATE UNIQUE INDEX years_name ON years(name);" );
    }
    {
        QString c = "CREATE TABLE tracks "
                    "(id " + storage->idType() +
                    ",url INTEGER"
                    ",artist INTEGER"
                    ",album INTEGER"
                    ",genre INTEGER"
                    ",composer INTEGER"
                    ",year INTEGER"
                    ",title " + storage->textColumnType() +
                    ",comment " + storage->longTextColumnType() +
                    ",tracknumber INTEGER"
                    ",discnumber INTEGER"
                    ",bitrate INTEGER"
                    ",length INTEGER"
                    ",samplerate INTEGER"
                    ",filesize INTEGER"
                    ",filetype INTEGER"     //does this still make sense?
                    ",bpm FLOAT"
                    ",createdate INTEGER"   // this is the track creation time
                    ",modifydate INTEGER"   // UNUSED currently
                    ",albumgain FLOAT"
                    ",albumpeakgain FLOAT" // decibels, relative to albumgain
                    ",trackgain FLOAT"
                    ",trackpeakgain FLOAT" // decibels, relative to trackgain
                    ") COLLATE = utf8_bin ENGINE = MyISAM;";

        storage->query( c );
        storage->query( "CREATE UNIQUE INDEX tracks_url ON tracks(url);" );

        QStringList indices;
        indices << "id" << "artist" << "album" << "genre" << "composer" << "year" << "title";
        indices << "discnumber" << "createdate" << "length" << "bitrate" << "filesize";
        foreach( const QString &index, indices )
        {
            QString query = QString( "CREATE INDEX tracks_%1 ON tracks(%2);" ).arg( index, index );
            storage->query( query );
        }
    }
    {
        QString c = "CREATE TABLE statistics "
                    "(id " + storage->idType() +
                    ",url INTEGER"
                    ",createdate INTEGER" // this is the first played time
                    ",accessdate INTEGER" // this is the last played time
                    ",score FLOAT"
                    ",rating INTEGER DEFAULT 0"
                    ",playcount INTEGER"
                    ",deleted BOOL DEFAULT " + storage->boolFalse() +
                    ") COLLATE = utf8_bin ENGINE = MyISAM;";
        storage->query( c );
        storage->query( "CREATE UNIQUE INDEX statistics_url ON statistics(url);" );
        QStringList indices;
        indices << "createdate" << "accessdate" << "score" << "rating" << "playcount";
        foreach( const QString &index, indices )
        {
            QString q = QString( "CREATE INDEX statistics_%1 ON statistics(%2);" ).arg( index, index );
            storage->query( q );
        }
    }
    {
        QString q = "CREATE TABLE labels "
                    "(id " + storage->idType() +
                    ",label " + storage->textColumnType() +
                    ") COLLATE = utf8_bin ENGINE = MyISAM;";
        storage->query( q );
        storage->query( "CREATE UNIQUE INDEX labels_label ON labels(label);" );

        QString r = "CREATE TABLE urls_labels(url INTEGER, label INTEGER);";
        storage->query( r );
        storage->query( "CREATE INDEX urlslabels_url ON urls_labels(url);" );
        storage->query( "CREATE INDEX urlslabels_label ON urls_labels(label);" );
    }
    {
        QString q = "CREATE TABLE amazon ("
                    "asin " + storage->textColumnType( 20 ) +
                    ",locale " + storage->textColumnType( 2 ) +
                    ",filename " + storage->textColumnType( 33 ) +
                    ",refetchdate INTEGER ) COLLATE = utf8_bin ENGINE = MyISAM;";
        storage->query( q );
        storage->query( "CREATE INDEX amazon_date ON amazon(refetchdate);" );
    }
    {
        QString q = "CREATE TABLE lyrics ("
                    "id " + storage->idType() +
                    ",url " + storage->exactIndexableTextColumnType() +
                    ",lyrics " + storage->longTextColumnType() +
                    ") COLLATE = utf8_bin ENGINE = MyISAM;";
        storage->query( q );
        storage->query( "CREATE UNIQUE INDEX lyrics_url ON lyrics(url);" );
    }
    storage->query( "INSERT INTO admin(component,version) "
                          "VALUES('AMAROK_TRACK'," + QString::number( DB_VERSION ) + ");" );
    {
         storage->query( "CREATE TABLE statistics_permanent "
                            "(url " + storage->exactIndexableTextColumnType() + " NOT NULL" +
                            ",firstplayed DATETIME"
                            ",lastplayed DATETIME"
                            ",score FLOAT"
                            ",rating INTEGER DEFAULT 0"
                            ",playcount INTEGER) COLLATE = utf8_bin ENGINE = MyISAM;" );

        //Below query is invalid!  Fix it, and then put the proper query in an upgrade function!
        storage->query( "CREATE UNIQUE INDEX stats_perm_url ON statistics_permanent(url)" );

        storage->query( "CREATE TABLE statistics_tag "
                             "(name " + storage->textColumnType(108) +
                             ",artist " + storage->textColumnType(108) +
                             ",album " + storage->textColumnType(108) +
                             ",firstplayed DATETIME"
                             ",lastplayed DATETIME"
                             ",score FLOAT"
                             ",rating INTEGER DEFAULT 0"
                             ",playcount INTEGER) COLLATE = utf8_bin ENGINE = MyISAM" );

        //Below query is invalid!  Fix it, and then put the proper query in an upgrade function!
        storage->query( "CREATE UNIQUE INDEX stats_tag_name_artist_album ON statistics_tag(name,artist,album)" );
    }
}

int
DatabaseUpdater::adminValue( const QString &key ) const
{
    SqlStorage *storage = m_collection->sqlStorage();

    QStringList values;
    values = storage->query( QString( "SELECT version FROM admin WHERE component = '%1';").arg(storage->escape( key ) ) );
    return values.isEmpty() ? 0 : values.first().toInt();
}

void
DatabaseUpdater::deleteAllRedundant( const QString &type )
{
    SqlStorage *storage = m_collection->sqlStorage();

    const QString tablename = type + 's';
    if( type == "artist" )
        storage->query( QString( "DELETE FROM artists "
                                 "WHERE id NOT IN ( SELECT artist FROM tracks ) AND "
                                 "id NOT IN ( SELECT artist FROM albums )") );
    else
        storage->query( QString( "DELETE FROM %1 "
                                 "WHERE id NOT IN ( SELECT %2 FROM tracks )" ).
                        arg( tablename, type ) );
}

void
DatabaseUpdater::removeFilesInDir( int deviceid, const QString &rdir )
{
    SqlStorage *storage = m_collection->sqlStorage();

    QString select = QString( "SELECT urls.id FROM urls LEFT JOIN directories ON urls.directory = directories.id "
                              "WHERE directories.deviceid = %1 AND directories.dir = '%2';" )
                                .arg( QString::number( deviceid ), storage->escape( rdir ) );
    QStringList idResult = storage->query( select );
    if( !idResult.isEmpty() )
    {
        QString id;
        QString ids;
        QStringList::ConstIterator it = idResult.constBegin(), end = idResult.constEnd();
        while( it != end )
        {
            id = (*(it++));
            if( !ids.isEmpty() )
                ids += ',';
            ids += id;
        }
        QString drop = QString( "DELETE FROM tracks WHERE url IN (%1);" ).arg( ids );
        storage->query( drop );
    }
}

void
DatabaseUpdater::writeCSVFile( const QString &table, const QString &filename, bool forceDebug )
{
    SqlStorage *storage = m_collection->sqlStorage();

    if( !forceDebug && !m_debugDatabaseContent )
        return;

    QString ctable = table;
    QStringList columns = storage->query(
            QString( "SELECT column_name FROM INFORMATION_SCHEMA.columns WHERE table_name='%1'" )
            .arg( storage->escape( ctable ) ) );

    if( columns.isEmpty() )
        return; //no table with that name

    // ok. it was probably a little bit unlucky to name a table statistics
    // that clashes with INFORMATION_SCHEMA.statistics, a build in table.
    if( table == "statistics" && columns.count() > 15 )
    {
        // delete all columns with full upper case name. Those are the buildins.
        for( int i = columns.count()-1; i>= 0; --i )
        {
            if( columns[i].toUpper() == columns[i] )
                columns.removeAt( i );
        }
    }

    QString select;
    foreach( const QString &column, columns )
    {
        if( !select.isEmpty() )
            select.append( ',' );
        select.append( column );
    }

    QString query = "SELECT %1 FROM %2";

    QStringList result = storage->query( query.arg( select, storage->escape( table ) ) );

    QFile file( filename );
    if( file.open( QFile::WriteOnly | QFile::Text | QFile::Truncate ) )
    {
        QTextStream stream( &file );
        int i = 0;
        QString line;
        //write header
        foreach( const QString &column, columns )
        {
            stream << column;
            stream << ';';
        }
        stream << '\n';

        foreach( const QString &data, result )
        {
            stream << data;
            stream << ';';
            ++i;
            if( i % columns.count() == 0 )
                stream << '\n';
        }
        file.close();
    }
}

