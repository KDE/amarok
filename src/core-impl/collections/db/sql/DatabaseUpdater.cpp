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
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include <core/storage/SqlStorage.h>
#include "SqlCollection.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMultiMap>
#include <QTextStream>

#include <KMessageBox>

static const int DB_VERSION = 15;

int
DatabaseUpdater::expectedDatabaseVersion()
{
    return DB_VERSION;
}

DatabaseUpdater::DatabaseUpdater( Collections::SqlCollection *collection )
    : m_collection( collection )
    , m_debugDatabaseContent( false )
{
    m_debugDatabaseContent = Amarok::config( "SqlCollection" ).readEntry( "DebugDatabaseContent", false );
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
DatabaseUpdater::schemaExists() const
{
    return adminValue( "DB_VERSION" ) != 0;
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
        return true;
    }

    if( dbVersion < DB_VERSION )
    {
        debug() << "Database out of date: database version is" << dbVersion << ", current version is" << DB_VERSION;
        switch( dbVersion )
        {
            case 1:
                upgradeVersion1to2();
                Q_FALLTHROUGH();
            case 2:
                upgradeVersion2to3();
                Q_FALLTHROUGH();
            case 3:
                upgradeVersion3to4();
                Q_FALLTHROUGH();
            case 4:
                upgradeVersion4to5();
                Q_FALLTHROUGH();
            case 5:
                upgradeVersion5to6();
                Q_FALLTHROUGH();
            case 6:
                upgradeVersion6to7();
                Q_FALLTHROUGH();
            case 7:
                upgradeVersion7to8();
                Q_FALLTHROUGH();
            case 8:
                //removes stray rows from albums that were caused by the initial full scan
                upgradeVersion8to9();
                Q_FALLTHROUGH();
            case 9:
                //removes stray rows from albums that were caused by the initial full scan
                upgradeVersion9to10();
                Q_FALLTHROUGH();
            case 10:
                upgradeVersion10to11();
                Q_FALLTHROUGH();
            case 11:
                upgradeVersion11to12();
                Q_FALLTHROUGH();
            case 12:
                upgradeVersion12to13();
                Q_FALLTHROUGH();
            case 13:
                upgradeVersion13to14();
                Q_FALLTHROUGH();
            case 14:
                upgradeVersion14to15();
                dbVersion = 15; // be sure to update this manually when introducing new version!
        }

        QString query = QString( "UPDATE admin SET version = %1 WHERE component = 'DB_VERSION';" ).arg( dbVersion );
        m_collection->sqlStorage()->query( query );

        //NOTE: A rescan will be triggered automatically as a result of an upgrade.  Don't trigger it here, as the
        //collection isn't fully initialized and this will trigger a crash/assert.
        return true;
    }

    if( dbVersion > DB_VERSION )
    {
        KMessageBox::error(nullptr,
                "<p>The Amarok collection database was created by a newer version of Amarok, "
                "and this version of Amarok cannot use it.</p>",
                "Database Type Unknown");
        // FIXME: maybe we should tell them how to delete the database?
        // FIXME: exit() may be a little harsh, but QCoreApplication::exit() doesn't seem to work
        exit(1);
    }

    return false;
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

    auto storage = m_collection->sqlStorage();
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
    auto storage = m_collection->sqlStorage();

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
    auto storage = m_collection->sqlStorage();

    //first the database
    storage->query( "ALTER DATABASE amarok DEFAULT CHARACTER SET utf8 DEFAULT COLLATE utf8_unicode_ci" );

    //now the tables

    //first, drop tables that can easily be recreated by doing an update
    QStringList dropTables;
    dropTables << "jamendo_albums" << "jamendo_artists" << "jamendo_genre" << "jamendo_tracks";
    dropTables << "magnatune_albums" << "magnatune_artists" << "magnatune_genre" << "magnatune_moods" << "magnatune_tracks";
    dropTables << "opmldirectory_albums" << "opmldirectory_artists" << "opmldirectory_genre" << "opmldirectory_tracks";

    for( const QString &table : dropTables )
        storage->query( "DROP TABLE " + table );

    //now, the rest of them
    QStringList tables;
    tables << "admin" << "albums" << "amazon" << "artists" << "bookmark_groups" << "bookmarks";
    tables << "composers" << "devices" << "directories" << "genres" << "images" << "labels" << "lyrics";
    tables << "playlist_groups" << "playlist_tracks" << "playlists";
    tables << "podcastchannels" << "podcastepisodes";
    tables << "statistics" << "statistics_permanent" << "statistics_tag";
    tables << "tracks" << "urls" << "urls_labels" << "years";

    for( const QString &table : tables )
        storage->query( "ALTER TABLE " + table + " DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci" );

    //now the columns (ugh)
    //first, varchar
    typedef QPair<QString, int> vcpair;
    QMultiMap<QString, vcpair> columns;
    columns.insert( "admin", vcpair( "component", 255 ) );
    columns.insert( "albums", vcpair( "name", textColumnLength() ) );
    columns.insert( "amazon", vcpair( "asin", 20 ) );
    columns.insert( "amazon", vcpair( "locale", 2 ) );
    columns.insert( "amazon", vcpair( "filename", 33 ) );
    columns.insert( "artists", vcpair( "name", textColumnLength() ) );
    columns.insert( "bookmark_groups", vcpair( "name", 255 ) );
    columns.insert( "bookmark_groups", vcpair( "description", 255 ) );
    columns.insert( "bookmark_groups", vcpair( "custom", 255 ) );
    columns.insert( "bookmarks", vcpair( "name", 255 ) );
    columns.insert( "bookmarks", vcpair( "url", 1024 ) );
    columns.insert( "bookmarks", vcpair( "description", 1024 ) );
    columns.insert( "bookmarks", vcpair( "custom", 255 ) );
    columns.insert( "composers", vcpair( "name", textColumnLength() ) );
    columns.insert( "devices", vcpair( "type", 255 ) );
    columns.insert( "devices", vcpair( "label", 255 ) );
    columns.insert( "devices", vcpair( "lastmountpoint", 255 ) );
    columns.insert( "devices", vcpair( "uuid", 255 ) );
    columns.insert( "devices", vcpair( "servername", 255 ) );
    columns.insert( "devices", vcpair( "sharename", 255 ) );
    columns.insert( "directories", vcpair( "dir", 1024 ) );
    columns.insert( "genres", vcpair( "name", 255 ) );
    columns.insert( "images", vcpair( "path", 255 ) );
    columns.insert( "labels", vcpair( "label", textColumnLength() ) );
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
    columns.insert( "tracks", vcpair( "title", textColumnLength() ) );
    columns.insert( "urls", vcpair( "rpath", 1024 ) );
    columns.insert( "urls", vcpair( "uniqueid", 128 ) );
    columns.insert( "years", vcpair( "name", textColumnLength() ) );

    QMultiMap<QString, vcpair>::const_iterator i, iEnd;
    for( i = columns.constBegin(), iEnd = columns.constEnd(); i != iEnd; ++i )
    {
        storage->query( "ALTER TABLE " + i.key() + " MODIFY " + i.value().first + " VARBINARY(" + QString::number( i.value().second ) + ')' );
        storage->query( "ALTER IGNORE TABLE " + i.key() + " MODIFY " + i.value().first +
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

    auto storage = m_collection->sqlStorage();

    //first, drop tables that can easily be recreated by doing an update
    QStringList dropTables;
    dropTables << "jamendo_albums" << "jamendo_artists" << "jamendo_genre" << "jamendo_tracks";
    dropTables << "magnatune_albums" << "magnatune_artists" << "magnatune_genre" << "magnatune_moods" << "magnatune_tracks";
    dropTables << "opmldirectory_albums" << "opmldirectory_artists" << "opmldirectory_genre" << "opmldirectory_tracks";

    for( const QString &table : dropTables )
        storage->query( "DROP TABLE " + table );

    //now, the rest of them
    QStringList tables;
    tables << "admin" << "albums" << "amazon" << "artists" << "bookmark_groups" << "bookmarks";
    tables << "composers" << "devices" << "directories" << "genres" << "images" << "labels" << "lyrics";
    tables << "playlist_groups" << "playlist_tracks" << "playlists";
    tables << "podcastchannels" << "podcastepisodes";
    tables << "statistics" << "statistics_permanent" << "statistics_tag";
    tables << "tracks" << "urls" << "urls_labels" << "years";

    for( const QString &table : tables )
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

    auto storage = m_collection->sqlStorage();

    typedef QPair<QString, int> vcpair;
    QMultiMap<QString, vcpair> columns;
    columns.insert( "directories", vcpair( "dir", 1000 ) );
    columns.insert( "urls", vcpair( "rpath", 324 ) );
    columns.insert( "statistics_permanent", vcpair( "url", 324 ) );

    QMultiMap<QString, vcpair>::const_iterator i, iEnd;

    for( i = columns.constBegin(), iEnd = columns.constEnd(); i != iEnd; ++i )
    {
        storage->query( "ALTER IGNORE TABLE " + i.key() + " MODIFY " + i.value().first +
            " VARCHAR(" + QString::number( i.value().second ) + ") COLLATE utf8_bin NOT NULL" );
    }

    columns.clear();

}


void
DatabaseUpdater::upgradeVersion7to8()
{
    DEBUG_BLOCK

    auto storage = m_collection->sqlStorage();

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

    auto storage = m_collection->sqlStorage();

    //first the database
    storage->query( "ALTER DATABASE amarok DEFAULT CHARACTER SET utf8 DEFAULT COLLATE utf8_bin" );

    //now the tables

    //first, drop tables that can easily be recreated by doing an update
    QStringList dropTables;
    dropTables << "jamendo_albums" << "jamendo_artists" << "jamendo_genre" << "jamendo_tracks";
    dropTables << "magnatune_albums" << "magnatune_artists" << "magnatune_genre" << "magnatune_moods" << "magnatune_tracks";
    dropTables << "opmldirectory_albums" << "opmldirectory_artists" << "opmldirectory_genre" << "opmldirectory_tracks";

    for( const QString &table : dropTables )
        storage->query( "DROP TABLE " + table );

    //now, the rest of them
    QStringList tables;
    tables << "admin" << "albums" << "amazon" << "artists" << "bookmark_groups" << "bookmarks";
    tables << "composers" << "devices" << "directories" << "genres" << "images" << "labels" << "lyrics";
    tables << "playlist_groups" << "playlist_tracks" << "playlists";
    tables << "podcastchannels" << "podcastepisodes";
    tables << "statistics" << "statistics_permanent" << "statistics_tag";
    tables << "tracks" << "urls" << "urls_labels" << "years";

    for( const QString &table : tables )
        storage->query( "ALTER TABLE " + table + " DEFAULT CHARACTER SET utf8 DEFAULT COLLATE utf8_bin COLLATE utf8_bin ENGINE = MyISAM" );

    //now the columns (ugh)
    //first, varchar
    typedef QPair<QString, int> vcpair;
    QMultiMap<QString, vcpair> columns;
    columns.insert( "admin", vcpair( "component", 255 ) );
    columns.insert( "albums", vcpair( "name", textColumnLength() ) );
    columns.insert( "amazon", vcpair( "asin", 20 ) );
    columns.insert( "amazon", vcpair( "locale", 2 ) );
    columns.insert( "amazon", vcpair( "filename", 33 ) );
    columns.insert( "artists", vcpair( "name", textColumnLength() ) );
    columns.insert( "bookmark_groups", vcpair( "name", 255 ) );
    columns.insert( "bookmark_groups", vcpair( "description", 255 ) );
    columns.insert( "bookmark_groups", vcpair( "custom", 255 ) );
    columns.insert( "bookmarks", vcpair( "name", 255 ) );
    columns.insert( "bookmarks", vcpair( "url", 1000 ) );
    columns.insert( "bookmarks", vcpair( "description", 1000 ) );
    columns.insert( "bookmarks", vcpair( "custom", 255 ) );
    columns.insert( "composers", vcpair( "name", textColumnLength() ) );
    columns.insert( "devices", vcpair( "type", 255 ) );
    columns.insert( "devices", vcpair( "label", 255 ) );
    columns.insert( "devices", vcpair( "lastmountpoint", 255 ) );
    columns.insert( "devices", vcpair( "uuid", 255 ) );
    columns.insert( "devices", vcpair( "servername", 80 ) );
    columns.insert( "devices", vcpair( "sharename", 240 ) );
    columns.insert( "directories", vcpair( "dir", 1000 ) );
    columns.insert( "genres", vcpair( "name", textColumnLength() ) );
    columns.insert( "images", vcpair( "path", 255 ) );
    columns.insert( "labels", vcpair( "label", textColumnLength() ) );
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
    columns.insert( "tracks", vcpair( "title", textColumnLength() ) );
    columns.insert( "urls", vcpair( "rpath", 324 ) );
    columns.insert( "urls", vcpair( "uniqueid", 128 ) );
    columns.insert( "years", vcpair( "name", textColumnLength() ) );

    QMultiMap<QString, vcpair>::const_iterator i, iEnd;

    for( i = columns.constBegin(), iEnd = columns.constEnd(); i != iEnd; ++i )
    {
        storage->query( "ALTER TABLE " + i.key() + " MODIFY " + i.value().first + " VARBINARY(" + QString::number( i.value().second ) + ')' );
        storage->query( "ALTER IGNORE TABLE " + i.key() + " MODIFY " + i.value().first +
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
DatabaseUpdater::upgradeVersion13to14()
{
    DEBUG_BLOCK
    auto storage = m_collection->sqlStorage();

    /* Following commands transition lyrics table from text-based urls (in fact just rpath
     * parts) to references to urls table. */

    // first, rename column
    storage->query( "ALTER TABLE lyrics CHANGE url rpath VARCHAR(324) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL" );
    // add integer column for url id
    storage->query( "ALTER TABLE lyrics ADD COLUMN url INT NULL DEFAULT NULL FIRST" );
    // try to extract url id from urls table using rpath
    storage->query( "UPDATE lyrics l SET l.url = (SELECT u.id FROM urls u WHERE u.rpath = l.rpath LIMIT 1)" );
    // delete entries with no matches in urls table; these should be just stale ones
    storage->query( "DELETE FROM lyrics WHERE url IS NULL" );
    // make the url column non-null
    storage->query( "ALTER TABLE lyrics MODIFY url INT NOT NULL" );
    // select duplicate ids into temporary table
    storage->query( "CREATE TEMPORARY TABLE duplicate_lyrics_ids ( id INT NOT NULL ) "
        "ENGINE=MEMORY SELECT dupl.id FROM lyrics orig "
        "LEFT JOIN lyrics dupl ON dupl.url = orig.url AND dupl.id > orig.id" );
    // delete duplicate lyrics entries
    storage->query( "DELETE FROM lyrics WHERE id IN (SELECT id FROM duplicate_lyrics_ids)" );
    // drop unwanted columns along with indexes defined on them
    storage->query( "ALTER TABLE lyrics DROP id, DROP rpath" );
    // add primary key; should definitely not fail as we have removed duplicate entries
    storage->query( "ALTER TABLE lyrics ADD PRIMARY KEY(url)" );
}

void
DatabaseUpdater::upgradeVersion14to15()
{
    /* This update solves bug 302837. In short, updates
     * 4 -> 5, 5 -> 6, 6 -> 7 and 9 -> 10 ignored NULL status of some columns and replaced
     * them with NOT NULL columns, causing various consequences, one of them is Dynamic
     * Collection not working. Fix it back.
     *
     * A list of columns to fix was obtained by comparing a database created by
     * Amarok 2.1.1 and then upgraded to current version with a db freshly created by
     * Amarok 2.6-git.
     */
    DEBUG_BLOCK
    auto storage = m_collection->sqlStorage();

    // zero length = TEXT datatype
    typedef QPair<QString, int> vcpair;
    QMultiMap<QString, vcpair> columns;

    columns.insert( "admin", vcpair( "component", 255 ) );
    columns.insert( "devices", vcpair( "type", 255 ) );
    columns.insert( "devices", vcpair( "label", 255 ) );
    columns.insert( "devices", vcpair( "lastmountpoint", 255 ) );
    columns.insert( "devices", vcpair( "uuid", 255 ) );
    columns.insert( "devices", vcpair( "servername", 80 ) );
    columns.insert( "devices", vcpair( "sharename", 240 ) );
    columns.insert( "labels", vcpair( "label", textColumnLength() ) );
    columns.insert( "lyrics", vcpair( "lyrics", 0 ) );
    columns.insert( "playlists", vcpair( "name", 255 ) );
    columns.insert( "playlists", vcpair( "description", 255 ) );
    columns.insert( "playlists", vcpair( "urlid", 1000 ) );
    columns.insert( "playlist_groups", vcpair( "name", 255 ) );
    columns.insert( "playlist_groups", vcpair( "description", 255 ) );
    columns.insert( "playlist_tracks", vcpair( "url", 1000 ) );
    columns.insert( "playlist_tracks", vcpair( "title", 255 ) );
    columns.insert( "playlist_tracks", vcpair( "album", 255 ) );
    columns.insert( "playlist_tracks", vcpair( "artist", 255 ) );
    columns.insert( "playlist_tracks", vcpair( "uniqueid", 128 ) );
    columns.insert( "podcastchannels", vcpair( "url", 0 ) );
    columns.insert( "podcastchannels", vcpair( "title", 0 ) );
    columns.insert( "podcastchannels", vcpair( "weblink", 0 ) );
    columns.insert( "podcastchannels", vcpair( "image", 0 ) );
    columns.insert( "podcastchannels", vcpair( "description", 0 ) );
    columns.insert( "podcastchannels", vcpair( "copyright", 255 ) );
    columns.insert( "podcastchannels", vcpair( "directory", 255 ) );
    columns.insert( "podcastchannels", vcpair( "labels", 255 ) );
    columns.insert( "podcastchannels", vcpair( "subscribedate", 255 ) );
    columns.insert( "podcastepisodes", vcpair( "url", 0 ) );
    columns.insert( "podcastepisodes", vcpair( "localurl", 0 ) );
    columns.insert( "podcastepisodes", vcpair( "guid", 1000 ) );
    columns.insert( "podcastepisodes", vcpair( "title", 0 ) );
    columns.insert( "podcastepisodes", vcpair( "subtitle", 0 ) );
    columns.insert( "podcastepisodes", vcpair( "description", 0 ) );
    columns.insert( "podcastepisodes", vcpair( "mimetype", 255 ) );
    columns.insert( "podcastepisodes", vcpair( "pubdate", 255 ) );
    columns.insert( "statistics_tag", vcpair( "name", 108 ) );
    columns.insert( "statistics_tag", vcpair( "artist", 108 ) );
    columns.insert( "statistics_tag", vcpair( "album", 108 ) );
    columns.insert( "tracks", vcpair( "title", textColumnLength() ) );
    columns.insert( "tracks", vcpair( "comment", 0 ) );
    columns.insert( "urls", vcpair( "uniqueid", 128 ) );

    QMapIterator<QString, vcpair> it( columns );
    while( it.hasNext() )
    {
        it.next();
        QString table = it.key();
        QString column = it.value().first;
        int length = it.value().second;

        QString query;
        if( length > 0 )
            query = QString( "ALTER TABLE `%1` CHANGE `%2` `%2` VARCHAR(%3) CHARACTER SET utf8 "
                    "COLLATE utf8_bin NULL DEFAULT NULL" ).arg( table, column ).arg( length );
        else
            query = QString( "ALTER TABLE `%1` CHANGE `%2` `%2` TEXT CHARACTER SET utf8 "
                    "COLLATE utf8_bin" ).arg( table, column );
        storage->query( query );
    }

    // there may be a stale unique index on the urls table, remove it if it is there:
    QStringList results = storage->query( "SHOW CREATE TABLE urls" );
    bool oldIndexFound = results.value( 1 ).contains( "UNIQUE KEY `uniqueid`" );
    if( oldIndexFound )
    {
        debug() << "dropping obsolete INDEX uniqueid on table urls";
        storage->query( "DROP INDEX uniqueid ON urls" );
    }
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

    auto storage = m_collection->sqlStorage();

    QStringList res = storage->query( "SHOW TABLES" );
    if( res.count() > 0 )
    {
        for( const QString &table : res )
            storage->query( "CHECK TABLE " + table + ( full ? " EXTENDED;" : " MEDIUM;" ) );
    }
}


void
DatabaseUpdater::createTables() const
{
    DEBUG_BLOCK

    auto storage = m_collection->sqlStorage();

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
        storage->query( "CREATE INDEX urls_directory ON urls(directory);" );
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
        for( const QString &index : indices )
        {
            QString query = QString( "CREATE INDEX tracks_%1 ON tracks(%2);" ).arg( index, index );
            storage->query( query );
        }
    }
    {
        QString c = "CREATE TABLE statistics "
                    "(id " + storage->idType() +
                    ",url INTEGER NOT NULL"
                    ",createdate INTEGER" // this is the first played time
                    ",accessdate INTEGER" // this is the last played time
                    ",score FLOAT"
                    ",rating INTEGER NOT NULL DEFAULT 0" // the "default" undefined rating is 0. We cannot display anything else.
                    ",playcount INTEGER NOT NULL DEFAULT 0" // a track is either played or not.
                    ",deleted BOOL NOT NULL DEFAULT " + storage->boolFalse() +
                    ") COLLATE = utf8_bin ENGINE = MyISAM;";
        storage->query( c );
        storage->query( "CREATE UNIQUE INDEX statistics_url ON statistics(url);" );
        QStringList indices;
        indices << "createdate" << "accessdate" << "score" << "rating" << "playcount";
        for( const QString &index : indices )
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
                    "url INTEGER PRIMARY KEY"
                    ",lyrics " + storage->longTextColumnType() +
                    ") COLLATE = utf8_bin ENGINE = MyISAM;";
        storage->query( q );
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
    auto storage = m_collection->sqlStorage();

    QStringList columns = storage->query(
            QString( "SELECT column_name FROM INFORMATION_SCHEMA.columns "
                     "WHERE table_name='admin'" ) );
    if( columns.isEmpty() )
        return 0; //no table with that name

    QStringList values = storage->query(
            QString( "SELECT version FROM admin WHERE component = '%1';")
            .arg(storage->escape( key ) ) );
    if( values.isEmpty() )
        return 0;

    return values.first().toInt();
}

void
DatabaseUpdater::deleteAllRedundant( const QString &type )
{
    auto storage = m_collection->sqlStorage();

    const QString tablename = type + 's';
    if( type == "artist" )
        storage->query( QString( "DELETE FROM artists "
                                 "WHERE id NOT IN ( SELECT artist FROM tracks WHERE artist IS NOT NULL ) AND "
                                 "id NOT IN ( SELECT artist FROM albums WHERE artist IS NOT NULL )") );
    else
        storage->query( QString( "DELETE FROM %1 "
                                 "WHERE id NOT IN ( SELECT %2 FROM tracks WHERE %2 IS NOT NULL )" ).
                        arg( tablename, type ) );
}

void
DatabaseUpdater::deleteOrphanedByDirectory( const QString &table )
{
    auto storage = m_collection->sqlStorage();
    QString query( "DELETE FROM %1 WHERE directory NOT IN ( SELECT id FROM directories )" );
    storage->query( query.arg( table ) );
}

void
DatabaseUpdater::deleteOrphanedByUrl( const QString &table )
{
    auto storage = m_collection->sqlStorage();
    QString query( "DELETE FROM %1 WHERE url NOT IN ( SELECT id FROM urls )" );
    storage->query( query.arg( table ) );
}

void
DatabaseUpdater::removeFilesInDir( int deviceid, const QString &rdir )
{
    auto storage = m_collection->sqlStorage();

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
    auto storage = m_collection->sqlStorage();

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
        // delete all columns with full upper case name. Those are the builtins.
        for( int i = columns.count()-1; i>= 0; --i )
        {
            if( columns[i].toUpper() == columns[i] )
                columns.removeAt( i );
        }
    }

    QString select;
    for( const QString &column : columns )
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
        //write header
        for( const QString &column : columns )
        {
            stream << column;
            stream << ';';
        }
        stream << '\n';

        for( const QString &data : result )
        {
            stream << data;
            stream << ';';
            ++i;
            if( i % columns.isEmpty() )
                stream << '\n';
        }
        file.close();
    }
}

