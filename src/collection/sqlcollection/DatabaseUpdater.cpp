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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "DatabaseUpdater.h"

#include "Debug.h"
#include "MountPointManager.h"
#include "SqlCollection.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMultiMap>
#include <QTextStream>

#include <KGlobal>
#include <KMessageBox>

static const int DB_VERSION = 6;

DatabaseUpdater::DatabaseUpdater( SqlCollection *collection )
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

void
DatabaseUpdater::update()
{
    DEBUG_BLOCK
    int dbVersion = adminValue( "DB_VERSION" );
    debug() << "Database version: " << dbVersion;
    if( dbVersion == 0 )
    {
        createTables();
        QString query = QString( "INSERT INTO admin(component, version) VALUES ('DB_VERSION', %1);" ).arg( DB_VERSION );
        m_collection->query( query );
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
        QString query = QString( "UPDATE admin SET version = %1 WHERE component = 'DB_VERSION';" ).arg( dbVersion );
        m_collection->query( query );

        //NOTE: A rescan will be triggered automatically as a result of an upgrade.  Don't trigger it here, as the 
        //collection isn't fully initialized and this will trigger a crash/assert.
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
}

void
DatabaseUpdater::upgradeVersion1to2()
{
    DEBUG_BLOCK

    m_collection->query( "ALTER TABLE tracks "
                         "ADD COLUMN albumgain FLOAT, "
                         "ADD COLUMN albumpeakgain FLOAT, "
                         "ADD COLUMN trackgain FLOAT,"
                         "ADD COLUMN trackpeakgain FLOAT;" );
}

void
DatabaseUpdater::upgradeVersion2to3()
{
    DEBUG_BLOCK

    m_collection->query( "DROP TABLE devices;" );

    QString create = "CREATE TABLE devices "
                     "(id " + m_collection->idType() +
                     ",type " + m_collection->textColumnType() +
                     ",label " + m_collection->textColumnType() +
                     ",lastmountpoint " + m_collection->textColumnType() +
                     ",uuid " + m_collection->textColumnType() +
                     ",servername " + m_collection->textColumnType() +
                     ",sharename " + m_collection->textColumnType() + ");";
    m_collection->query( create );
    m_collection->query( "CREATE INDEX devices_type ON devices( type );" );
    m_collection->query( "CREATE UNIQUE INDEX devices_uuid ON devices( uuid );" );
    m_collection->query( "CREATE INDEX devices_rshare ON devices( servername, sharename );" );

}

void
DatabaseUpdater::upgradeVersion3to4()
{
    m_collection->query( "CREATE TABLE statistics_permanent "
                         "(url " + m_collection->exactTextColumnType() +
                         ",firstplayed DATETIME"
                         ",lastplayed DATETIME"
                         ",score FLOAT"
                         ",rating INTEGER DEFAULT 0"
                         ",playcount INTEGER)" );
    m_collection->query( "CREATE UNIQUE INDEX ON statistics_permanent(url)" );
    //Note: the above index query is invalid, but kept here for posterity

    m_collection->query( "CREATE TABLE statistics_tag "
                         "(name " + m_collection->textColumnType() +
                         ",artist " + m_collection->textColumnType() +
                         ",album " + m_collection->textColumnType() +
                         ",firstplayed DATETIME"
                         ",lastplayed DATETIME"
                         ",score FLOAT"
                         ",rating INTEGER DEFAULT 0"
                         ",playcount INTEGER)" );
    m_collection->query( "CREATE UNIQUE INDEX ON statistics_tag(name,artist,album)" );
    //Note: the above index query is invalid, but kept here for posterity
}

void
DatabaseUpdater::upgradeVersion4to5()
{
    DEBUG_BLOCK
    //first the database
    m_collection->query( "ALTER DATABASE amarok DEFAULT CHARACTER SET utf8 DEFAULT COLLATE utf8_unicode_ci" );

    //now the tables

    //first, drop tables that can easily be recreated by doing an update
    QStringList dropTables;
    dropTables << "jamendo_albums" << "jamendo_artists" << "jamendo_genre" << "jamendo_tracks";
    dropTables << "magnatune_albums" << "magnatune_artists" << "magnatune_genre" << "magnatune_moods" << "magnatune_tracks";
    dropTables << "opmldirectory_albums" << "opmldirectory_artists" << "opmldirectory_genre" << "opmldirectory_tracks";

    foreach( QString table, dropTables )
        m_collection->query( "DROP TABLE " + table );

    //now, the rest of them
    QStringList tables;
    tables << "admin" << "albums" << "amazon" << "artists" << "bookmark_groups" << "bookmarks";
    tables << "composers" << "devices" << "directories" << "genres" << "images" << "labels" << "lyrics";
    tables << "playlist_groups" << "playlist_tracks" << "playlists";
    tables << "podcastchannels" << "podcastepisodes";
    tables << "statistics" << "statistics_permanent" << "statistics_tag";
    tables << "tracks" << "urls" << "urls_labels" << "years";

    foreach( QString table, tables )
        m_collection->query( "ALTER TABLE " + table + " DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci" );

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

    QMultiMap<QString, vcpair>::const_iterator i;

    for( i = columns.begin(); i != columns.end(); ++i )
    {
        m_collection->query( "ALTER TABLE " + i.key() + " MODIFY " + i.value().first + " VARBINARY(" + QString::number( i.value().second ) + ")" );
        m_collection->query( "ALTER IGNORE TABLE " + i.key() + " MODIFY " + i.value().first + \
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

    m_collection->query( "DROP INDEX url_podchannel ON podcastchannels" );
    m_collection->query( "DROP INDEX url_podepisode ON podcastepisodes" );
    m_collection->query( "DROP INDEX localurl_podepisode ON podcastepisodes" );   
    for( i = columns.begin(); i != columns.end(); ++i )
    {
        m_collection->query( "ALTER TABLE " + i.key() + " MODIFY " + i.value().first + " BLOB" );
        m_collection->query( "ALTER IGNORE TABLE " + i.key() + " MODIFY " + i.value().first + " TEXT CHARACTER SET utf8 NOT NULL" );
    }
    m_collection->query( "CREATE FULLTEXT INDEX url_podchannel ON podcastchannels( url )" );
    m_collection->query( "CREATE FULLTEXT INDEX url_podepisode ON podcastepisodes( url )" );
    m_collection->query( "CREATE FULLTEXT INDEX localurl_podepisode ON podcastepisodes( localurl )" );
}

void
DatabaseUpdater::upgradeVersion5to6()
{
    DEBUG_BLOCK
    //first, drop tables that can easily be recreated by doing an update
    QStringList dropTables;
    dropTables << "jamendo_albums" << "jamendo_artists" << "jamendo_genre" << "jamendo_tracks";
    dropTables << "magnatune_albums" << "magnatune_artists" << "magnatune_genre" << "magnatune_moods" << "magnatune_tracks";
    dropTables << "opmldirectory_albums" << "opmldirectory_artists" << "opmldirectory_genre" << "opmldirectory_tracks";

    foreach( QString table, dropTables )
        m_collection->query( "DROP TABLE " + table );

    //now, the rest of them
    QStringList tables;
    tables << "admin" << "albums" << "amazon" << "artists" << "bookmark_groups" << "bookmarks";
    tables << "composers" << "devices" << "directories" << "genres" << "images" << "labels" << "lyrics";
    tables << "playlist_groups" << "playlist_tracks" << "playlists";
    tables << "podcastchannels" << "podcastepisodes";
    tables << "statistics" << "statistics_permanent" << "statistics_tag";
    tables << "tracks" << "urls" << "urls_labels" << "years";

    foreach( QString table, tables )
        m_collection->query( "ALTER TABLE " + table + " ENGINE = MyISAM" );

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

    QMultiMap<QString, vcpair>::const_iterator i;

    for( i = columns.begin(); i != columns.end(); ++i )
        m_collection->query( "ALTER IGNORE TABLE " + i.key() + " MODIFY " + i.value().first + " VARCHAR(" + QString::number( i.value().second ) + ") " );

    m_collection->query( "CREATE INDEX devices_rshare ON devices( servername, sharename );" );
    m_collection->query( "CREATE UNIQUE INDEX lyrics_url ON lyrics(url);" );
    m_collection->query( "CREATE UNIQUE INDEX urls_id_rpath ON urls(deviceid, rpath);" );
    m_collection->query( "CREATE UNIQUE INDEX stats_tag_name_artist_album ON statistics_tag(name,artist,album)" );
}

void
DatabaseUpdater::createTemporaryTables()
{
    DEBUG_BLOCK

    //debug stuff
    //removeTemporaryTables();

    //this is a copy of the relevant code in createTables()
    //TODO refactor this to make it easier to keep the tables created by those methods in sync
    {
        QString create = "CREATE TEMPORARY TABLE urls_temp "
                         "(id " + m_collection->idType() +
                         ",deviceid INTEGER"
                         ",rpath " + m_collection->exactIndexableTextColumnType() +
                         ",directory INTEGER"
                         ",uniqueid " + m_collection->exactTextColumnType(128) + " UNIQUE) ENGINE = MyISAM;";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX urls_id_rpath_temp ON urls_temp(deviceid, rpath);" );
        m_collection->query( "CREATE INDEX urls_temp_uniqueid ON urls_temp(uniqueid);" );
    }
    {
        QString create = "CREATE TEMPORARY TABLE directories_temp "
                         "(id " + m_collection->idType() +
                         ",deviceid INTEGER"
                         ",dir " + m_collection->exactTextColumnType() +
                         ",changedate INTEGER) ENGINE = MyISAM;";
        m_collection->query( create );
    }
    {
        QString create = "CREATE TEMPORARY TABLE artists_temp "
                         "(id " + m_collection->idType() +
                         ",name " + m_collection->textColumnType() + " NOT NULL) ENGINE = MyISAM;";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX artists_temp_name ON artists_temp(name);" );
    }
    {
        QString c = "CREATE TEMPORARY TABLE albums_temp "
                    "(id " + m_collection->idType() +
                    ",name " + m_collection->textColumnType() + " NOT NULL"
                    ",artist INTEGER" +
                    ",image INTEGER) ENGINE = MyISAM;";
        m_collection->query( c );
        m_collection->query( "CREATE INDEX albums_temp_name ON albums_temp(name);" );
        m_collection->query( "CREATE INDEX albums_temp_artist ON albums_temp(artist);" );
        m_collection->query( "CREATE INDEX albums_temp_image ON albums_temp(image);" );
        m_collection->query( "CREATE UNIQUE INDEX albums_temp_name_artist ON albums_temp(name,artist);" );
        //the index below should not be necessary. uncomment if a query plan shows it is
        //m_collection->query( "CREATE UNIQUE INDEX albums_artist_name ON albums(artist,name);" );
    }
    {
        QString create = "CREATE TEMPORARY TABLE genres_temp "
                         "(id " + m_collection->idType() +
                         ",name " + m_collection->textColumnType() + " NOT NULL) ENGINE = MyISAM;";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX genres_temp_name ON genres_temp(name);" );
    }
    {
        QString create = "CREATE TEMPORARY TABLE composers_temp "
                         "(id " + m_collection->idType() +
                         ",name " + m_collection->textColumnType() + " NOT NULL) ENGINE = MyISAM;";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX composers_temp_name ON composers_temp(name);" );
    }
    {
        QString create = "CREATE TEMPORARY TABLE years_temp "
                         "(id " + m_collection->idType() +
                         ",name " + m_collection->textColumnType() + " NOT NULL) ENGINE = MyISAM;";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX years_temp_name ON years_temp(name);" );
    }
    {
        QString create = "CREATE TEMPORARY TABLE images_temp "
                         "(id " + m_collection->idType() +
                         ",path " + m_collection->textColumnType() + " NOT NULL) ENGINE = MyISAM;";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX images_temp_name ON images_temp(path);" );
    }
    {
        QString c = "CREATE TEMPORARY TABLE tracks_temp "
                    "(id " + m_collection->idType() +
                    ",url INTEGER"
                    ",artist INTEGER"
                    ",album INTEGER"
                    ",genre INTEGER"
                    ",composer INTEGER"
                    ",year INTEGER"
                    ",title " + m_collection->textColumnType() +
                    ",comment " + m_collection->longTextColumnType() +
                    ",tracknumber INTEGER"
                    ",discnumber INTEGER"
                    ",bitrate INTEGER"
                    ",length INTEGER"
                    ",samplerate INTEGER"
                    ",filesize INTEGER"
                    ",filetype INTEGER"     //does this still make sense?
                    ",bpm FLOAT"
                    ",createdate INTEGER"   //are the two dates needed?
                    ",modifydate INTEGER"
                    ",albumgain FLOAT"
                    ",albumpeakgain FLOAT"
                    ",trackgain FLOAT"
                    ",trackpeakgain FLOAT"
                    ") ENGINE = MyISAM;";

        m_collection->query( c );
        m_collection->query( "CREATE UNIQUE INDEX tracks_temp_url ON tracks_temp(url);" );
    }
}

void
DatabaseUpdater::prepareTemporaryTables()
{
    DEBUG_BLOCK
    m_collection->query( "INSERT INTO directories_temp SELECT * FROM directories;" );
    m_collection->query( "INSERT INTO urls_temp SELECT * FROM urls;" );
    m_collection->query( "INSERT INTO artists_temp SELECT * FROM artists;" );
    m_collection->query( "INSERT INTO years_temp SELECT * FROM years;" );
    m_collection->query( "INSERT INTO albums_temp SELECT * FROM albums;" );
    m_collection->query( "INSERT INTO images_temp SELECT * FROM images;" );
    m_collection->query( "INSERT INTO genres_temp SELECT * FROM genres;" );
    m_collection->query( "INSERT INTO composers_temp SELECT * FROM composers;" );
    m_collection->query( "INSERT INTO tracks_temp SELECT * FROM tracks;" );
}

void
DatabaseUpdater::prepareTemporaryTablesForFullScan()
{
    m_collection->query( "INSERT INTO directories_temp SELECT * FROM directories;" );
    m_collection->query( "INSERT INTO urls_temp SELECT * FROM urls;" );
}

void
DatabaseUpdater::cleanPermanentTables()
{
    m_collection->query( "DELETE FROM composers;" );
    m_collection->query( "DELETE FROM genres;" );
    m_collection->query( "DELETE FROM images;" );
    m_collection->query( "DELETE FROM albums;" );
    m_collection->query( "DELETE FROM years;" );
    m_collection->query( "DELETE FROM artists;" );
    m_collection->query( "DELETE FROM tracks;" );
    m_collection->query( "DELETE FROM urls;" );
    m_collection->query( "DELETE FROM directories" );
}

void
DatabaseUpdater::removeTemporaryTables()
{
    DEBUG_BLOCK
    m_collection->query( "DROP TABLE tracks_temp;" );
    m_collection->query( "DROP TABLE images_temp;" );
    m_collection->query( "DROP TABLE albums_temp;" );
    m_collection->query( "DROP TABLE genres_temp;" );
    m_collection->query( "DROP TABLE years_temp;" );
    m_collection->query( "DROP TABLE composers_temp;" );
    m_collection->query( "DROP TABLE artists_temp;" );
    m_collection->query( "DROP TABLE urls_temp;" );
    m_collection->query( "DROP TABLE directories_temp" );
}

void
DatabaseUpdater::cleanupDatabase()
{
    QStringList result = m_collection->query( "SELECT COUNT(*) FROM INFORMATION_SCHEMA.TABLES WHERE table_name like '%_temp';" );
    if( result.count() > 0 && result.first().toInt() > 0 )
    {
        //looks like the temporary tables were not removed, probably because of a crash
        removeTemporaryTables();
    }
}

void
DatabaseUpdater::copyToPermanentTables()
{
    DEBUG_BLOCK

    writeCSVFile( "artists_temp", "artists_temp" );
    writeCSVFile( "albums_temp", "albums_temp" );
    writeCSVFile( "tracks_temp", "tracks_temp" );
    writeCSVFile( "genres_temp", "genres_temp" );
    writeCSVFile( "years_temp", "years_temp" );
    writeCSVFile( "composers_temp", "composers_temp" );
    writeCSVFile( "urls_temp", "urls_temp" );

    writeCSVFile( "artists", "artists_before" );
    writeCSVFile( "albums", "albums_before" );
    writeCSVFile( "tracks", "tracks_before" );
    writeCSVFile( "genres", "genres_before" );
    writeCSVFile( "years", "years_before" );
    writeCSVFile( "composers", "composers_before" );
    writeCSVFile( "urls", "urls_before" );


    //handle artists before albums
    QStringList artistIdList = m_collection->query( "SELECT artists.id FROM artists;" );
    QString artistIds = "-1";
    foreach( const QString &artistId, artistIdList )
    {
        artistIds += ',';
        artistIds += artistId;
    }
    m_collection->insert( QString ( "INSERT INTO artists SELECT * FROM artists_temp WHERE artists_temp.id NOT IN ( %1 );" ).arg( artistIds ), QString() );

    //handle images before albums
    m_collection->query( "DELETE FROM images;" );
    m_collection->insert( "INSERT INTO images SELECT * FROM images_temp;", NULL );

    QStringList albumIdList = m_collection->query( "SELECT albums.id FROM albums;" );
    //in an empty database, albumIdList is empty. This would result in a SQL query like NOT IN ( ) without
    //the -1 below which is invalid SQL. The auto generated values start at 1 so this is fine
    QString albumIds = "-1";
    foreach( const QString &albumId, albumIdList )
    {
        albumIds += ',';
        albumIds += albumId;
    }
    m_collection->insert( QString ( "INSERT INTO albums SELECT * FROM albums_temp WHERE albums_temp.id NOT IN ( %1 );" ).arg( albumIds ), QString() );

    QStringList composerIdList = m_collection->query( "SELECT composers.id FROM composers;" );
    QString composerIds = "-1";
    foreach( const QString &composerId, composerIdList )
    {
        composerIds += ',';
        composerIds += composerId;
    }
    m_collection->insert( QString ( "INSERT INTO composers SELECT * FROM composers_temp WHERE composers_temp.id NOT IN ( %1 );" ).arg( composerIds ), QString() );

    QStringList genreIdList = m_collection->query( "SELECT genres.id FROM genres;" );
    QString genreIds = "-1";
    foreach( const QString &genreId, genreIdList )
    {
        genreIds += ',';
        genreIds += genreId;
    }
    m_collection->insert( QString ( "INSERT INTO genres SELECT * FROM genres_temp WHERE genres_temp.id NOT IN ( %1 );" ).arg( genreIds ), QString() );

    QStringList yearIdList = m_collection->query( "SELECT years.id FROM years;" );
    QString yearIds = "-1";
    foreach( const QString &yearId, yearIdList )
    {
        yearIds += ',';
        yearIds += yearId;
    }
    m_collection->insert( QString ( "INSERT INTO years SELECT * FROM years_temp WHERE years_temp.id NOT IN ( %1 );" ).arg( yearIds ), QString() );

    //insert( "INSERT INTO embed SELECT * FROM embed_temp;", NULL );
    //m_collection->insert( "INSERT INTO directories SELECT * FROM directories_temp;", QString() );

    QStringList urlIdList = m_collection->query( "SELECT urls.id FROM urls;" );
    QString urlIds = "-1";
    foreach( const QString &urlId, urlIdList )
    {
        urlIds += ',';
        urlIds += urlId;
    }
    m_collection->insert( QString( "REPLACE INTO urls SELECT * FROM urls_temp;" ), QString() );

    //update the directories table
    //we don't know in which rows the changedate was updated, so we simply copy the whole
    //temporary table. We need a transaction here if we start to use foreign keys

    //why does this fail?
    m_collection->query( "DELETE FROM directories;" );
    m_collection->query( "INSERT INTO directories SELECT * FROM directories_temp;" );

    //copy tracks last so that we don't get problems with foreign key constraints
    QStringList trackIdList = m_collection->query( "SELECT tracks.id FROM tracks;" );
    QString trackIds = "-1";
    foreach( const QString &trackId, trackIdList )
    {
        trackIds += ',';
        trackIds += trackId;
    }
    m_collection->insert( QString( "REPLACE INTO tracks SELECT * FROM tracks_temp;" ), QString() );

    writeCSVFile( "artists", "artists_after" );
    writeCSVFile( "albums", "albums_after" );
    writeCSVFile( "tracks", "tracks_after" );
    writeCSVFile( "genres", "genres_after" );
    writeCSVFile( "years", "years_after" );
    writeCSVFile( "composers", "composers_after" );
    writeCSVFile( "urls", "urls_after" );

    m_collection->sendChangedSignal();
}

void
DatabaseUpdater::createTables() const
{
    DEBUG_BLOCK
    // see docs/database/amarokTables.svg for documentation about database layout
    {
        QString c = "CREATE TABLE admin (component " + m_collection->textColumnType() + ", version INTEGER) ENGINE = MyISAM;";
        m_collection->query( c );
    }
    {
        QString create = "CREATE TABLE devices "
                         "(id " + m_collection->idType() +
                         ",type " + m_collection->textColumnType() +
                         ",label " + m_collection->textColumnType() +
                         ",lastmountpoint " + m_collection->textColumnType() +
                         ",uuid " + m_collection->textColumnType() +
                         ",servername " + m_collection->textColumnType(80) +
                         ",sharename " + m_collection->textColumnType(240) + ") ENGINE = MyISAM;";
        m_collection->query( create );
        m_collection->query( "CREATE INDEX devices_type ON devices( type );" );
        m_collection->query( "CREATE UNIQUE INDEX devices_uuid ON devices( uuid );" );
        m_collection->query( "CREATE INDEX devices_rshare ON devices( servername, sharename );" );
    }
    {
        QString create = "CREATE TABLE urls "
                         "(id " + m_collection->idType() +
                         ",deviceid INTEGER"
                         ",rpath " + m_collection->exactIndexableTextColumnType() +
                         ",directory INTEGER"
                         ",uniqueid " + m_collection->exactTextColumnType(128) + " UNIQUE) ENGINE = MyISAM;";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX urls_id_rpath ON urls(deviceid, rpath);" );
        m_collection->query( "CREATE INDEX urls_uniqueid ON urls(uniqueid);" );
    }
    {
        QString create = "CREATE TABLE directories "
                         "(id " + m_collection->idType() +
                         ",deviceid INTEGER"
                         ",dir " + m_collection->exactTextColumnType() +
                         ",changedate INTEGER) ENGINE = MyISAM;";
        m_collection->query( create );
        m_collection->query( "CREATE INDEX directories_deviceid ON directories(deviceid);" );
    }
    {
        QString create = "CREATE TABLE artists "
                         "(id " + m_collection->idType() +
                         ",name " + m_collection->textColumnType() + " NOT NULL) ENGINE = MyISAM;";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX artists_name ON artists(name);" );
    }
    {
        QString create = "CREATE TABLE images "
                         "(id " + m_collection->idType() +
                         ",path " + m_collection->textColumnType() + " NOT NULL) ENGINE = MyISAM;";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX images_name ON images(path);" );
    }
    {
        QString c = "CREATE TABLE albums "
                    "(id " + m_collection->idType() +
                    ",name " + m_collection->textColumnType() + " NOT NULL"
                    ",artist INTEGER" +
                    ",image INTEGER) ENGINE = MyISAM;";
        m_collection->query( c );
        m_collection->query( "CREATE INDEX albums_name ON albums(name);" );
        m_collection->query( "CREATE INDEX albums_artist ON albums(artist);" );
        m_collection->query( "CREATE INDEX albums_image ON albums(image);" );
        m_collection->query( "CREATE UNIQUE INDEX albums_name_artist ON albums(name,artist);" );
        //the index below should not be necessary. uncomment if a query plan shows it is
        //m_collection->query( "CREATE UNIQUE INDEX albums_artist_name ON albums(artist,name);" );
    }
    {
        QString create = "CREATE TABLE genres "
                         "(id " + m_collection->idType() +
                         ",name " + m_collection->textColumnType() + " NOT NULL) ENGINE = MyISAM;";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX genres_name ON genres(name);" );
    }
    {
        QString create = "CREATE TABLE composers "
                         "(id " + m_collection->idType() +
                         ",name " + m_collection->textColumnType() + " NOT NULL) ENGINE = MyISAM;";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX composers_name ON composers(name);" );
    }
    {
        QString create = "CREATE TABLE years "
                         "(id " + m_collection->idType() +
                         ",name " + m_collection->textColumnType() + " NOT NULL) ENGINE = MyISAM;";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX years_name ON years(name);" );
    }
    {
        QString c = "CREATE TABLE tracks "
                    "(id " + m_collection->idType() +
                    ",url INTEGER"
                    ",artist INTEGER"
                    ",album INTEGER"
                    ",genre INTEGER"
                    ",composer INTEGER"
                    ",year INTEGER"
                    ",title " + m_collection->textColumnType() +
                    ",comment " + m_collection->longTextColumnType() +
                    ",tracknumber INTEGER"
                    ",discnumber INTEGER"
                    ",bitrate INTEGER"
                    ",length INTEGER"
                    ",samplerate INTEGER"
                    ",filesize INTEGER"
                    ",filetype INTEGER"     //does this still make sense?
                    ",bpm FLOAT"
                    ",createdate INTEGER"   //are the two dates needed?
                    ",modifydate INTEGER"
                    ",albumgain FLOAT"
                    ",albumpeakgain FLOAT" // decibels, relative to albumgain
                    ",trackgain FLOAT"
                    ",trackpeakgain FLOAT" // decibels, relative to trackgain
                    ") ENGINE = MyISAM;";

        m_collection->query( c );
        m_collection->query( "CREATE UNIQUE INDEX tracks_url ON tracks(url);" );

        QStringList indices;
        indices << "artist" << "album" << "genre" << "composer" << "year" << "title";
        indices << "discnumber" << "createdate" << "length" << "bitrate" << "filesize";
        foreach( const QString &index, indices )
        {
            QString query = QString( "CREATE INDEX tracks_%1 ON tracks(%2);" ).arg( index, index );
            m_collection->query( query );
        }
    }
    {
        QString c = "CREATE TABLE statistics "
                    "(id " + m_collection->idType() +
                    ",url INTEGER"
                    ",createdate INTEGER"
                    ",accessdate INTEGER"
                    ",score FLOAT"
                    ",rating INTEGER DEFAULT 0"
                    ",playcount INTEGER"
                    ",deleted BOOL DEFAULT " + m_collection->boolFalse() +
                    ") ENGINE = MyISAM;";
        m_collection->query( c );
        m_collection->query( "CREATE UNIQUE INDEX statistics_url ON statistics(url);" );
        QStringList indices;
        indices << "createdate" << "accessdate" << "score" << "rating" << "playcount";
        foreach( const QString &index, indices )
        {
            QString q = QString( "CREATE INDEX statistics_%1 ON statistics(%2);" ).arg( index, index );
            m_collection->query( q );
        }
    }
    {
        QString q = "CREATE TABLE labels "
                    "(id " + m_collection->idType() +
                    ",label " + m_collection->textColumnType() +
                    ") ENGINE = MyISAM;";
        m_collection->query( q );
        m_collection->query( "CREATE UNIQUE INDEX labels_label ON labels(label);" );

        QString r = "CREATE TABLE urls_labels(url INTEGER, label INTEGER);";
        m_collection->query( r );
        m_collection->query( "CREATE INDEX urlslabels_url ON urls_labels(url);" );
        m_collection->query( "CREATE INDEX urlslabels_label ON urls_labels(label);" );
    }
    {
        QString q = "CREATE TABLE amazon ("
                    "asin " + m_collection->textColumnType( 20 ) +
                    ",locale " + m_collection->textColumnType( 2 ) +
                    ",filename " + m_collection->textColumnType( 33 ) +
                    ",refetchdate INTEGER ) ENGINE = MyISAM;";
        m_collection->query( q );
        m_collection->query( "CREATE INDEX amazon_date ON amazon(refetchdate);" );
    }
    {
        QString q = "CREATE TABLE lyrics ("
                    "id " + m_collection->idType() +
                    ",url " + m_collection->exactIndexableTextColumnType() +
                    ",lyrics " + m_collection->longTextColumnType() +
                    ") ENGINE = MyISAM;";
        m_collection->query( q );
        m_collection->query( "CREATE UNIQUE INDEX lyrics_url ON lyrics(url);" );
    }
    m_collection->query( "INSERT INTO admin(component,version) "
                          "VALUES('AMAROK_TRACK'," + QString::number( DB_VERSION ) + ");" );
    {
         m_collection->query( "CREATE TABLE statistics_permanent "
                            "(url " + m_collection->exactIndexableTextColumnType() +
                            ",firstplayed DATETIME"
                            ",lastplayed DATETIME"
                            ",score FLOAT"
                            ",rating INTEGER DEFAULT 0"
                            ",playcount INTEGER) ENGINE = MyISAM;" );

        //Below query is invalid!  Fix it, and then put the proper query in an upgrade function!
        m_collection->query( "CREATE UNIQUE INDEX stats_perm_url ON statistics_permanent(url)" );

        m_collection->query( "CREATE TABLE statistics_tag "
                             "(name " + m_collection->textColumnType(108) +
                             ",artist " + m_collection->textColumnType(108) +
                             ",album " + m_collection->textColumnType(108) +
                             ",firstplayed DATETIME"
                             ",lastplayed DATETIME"
                             ",score FLOAT"
                             ",rating INTEGER DEFAULT 0"
                             ",playcount INTEGER) ENGINE = MyISAM" );

        //Below query is invalid!  Fix it, and then put the proper query in an upgrade function!
        m_collection->query( "CREATE UNIQUE INDEX stats_tag_name_artist_album ON statistics_tag(name,artist,album)" );
    }
}

int
DatabaseUpdater::adminValue( const QString &key ) const
{
    QStringList values;
    values = m_collection->query( QString( "SELECT version FROM admin WHERE component = '%1';").arg(m_collection->escape( key ) ) );
    return values.isEmpty() ? 0 : values.first().toInt();
}

void
DatabaseUpdater::deleteAllRedundant( const QString &type )
{
    const QString tablename = type + 's';
    m_collection->query( QString( "DELETE FROM %1 WHERE id NOT IN ( SELECT %2 FROM tracks )" ).arg( tablename, type ) );
}

void
DatabaseUpdater::removeFilesInDir( int deviceid, const QString &rdir )
{
    QString select = QString( "SELECT urls.id FROM urls LEFT JOIN directories ON urls.directory = directories.id "
                              "WHERE directories.deviceid = %1 AND directories.dir = '%2';" )
                                .arg( QString::number( deviceid ), m_collection->escape( rdir ) );
    QStringList idResult = m_collection->query( select );
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
        QString drop = QString( "DELETE FROM tracks WHERE id IN (%1);" ).arg( ids );
        m_collection->query( drop );
    }
}

void
DatabaseUpdater::removeFilesInDirFromTemporaryTables( int deviceid, const QString &rdir )
{
    QString select = QString( "SELECT urls.id FROM urls_temp AS urls LEFT JOIN directories_temp AS directories ON urls.directory = directories.id "
                              "WHERE directories.deviceid = %1 AND directories.dir = '%2';" )
                                .arg( QString::number( deviceid ), m_collection->escape( rdir ) );
    QStringList idResult = m_collection->query( select );
    if( !idResult.isEmpty() )
    {
        QString ids;
        foreach( const QString &id, idResult )
        {
            if( !ids.isEmpty() )
                ids += ',';
            ids += id;
        }
        QString drop = QString( "DELETE FROM tracks_temp WHERE id IN (%1);" ).arg( ids );
        m_collection->query( drop );
    }
}

void
DatabaseUpdater::writeCSVFile( const QString &table, const QString &filename, bool forceDebug )
{
    if( !forceDebug && !m_debugDatabaseContent )
        return;

    QStringList columns = m_collection->query(
            QString( "SELECT column_name FROM INFORMATION_SCHEMA.columns WHERE table_schema='amarok' and table_name='%1'" )
            .arg( m_collection->escape( table ) ) );

    if( columns.isEmpty() )
        return; //no table with that name

    QString select;
    foreach( const QString &column, columns )
    {
        if( !select.isEmpty() )
            select.append( ',' );
        select.append( column );
    }

    QString query = "SELECT %1 FROM amarok.%2";

    QStringList result = m_collection->query( query.arg( select, m_collection->escape( table ) ) );
    QString filePath =
            QDir::home().absoluteFilePath( filename + '-' + QDateTime::currentDateTime().toString( Qt::ISODate ) + ".csv" );
    QFile::remove( filePath );
    QFile file( filePath );
    if( file.open( QFile::WriteOnly | QFile::Text ) )
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

