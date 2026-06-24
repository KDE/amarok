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

static const int DB_VERSION = 16;

static MySqlUpgradeFunc s_mySqlUpgradeFunc = nullptr;

void
DatabaseUpdater::setMySqlUpgradeFunc( MySqlUpgradeFunc func )
{
    s_mySqlUpgradeFunc = func;
}

MySqlUpgradeFunc
DatabaseUpdater::mySqlUpgradeFunc()
{
    return s_mySqlUpgradeFunc;
}

int
DatabaseUpdater::expectedDatabaseVersion()
{
    return DB_VERSION;
}

DatabaseUpdater::DatabaseUpdater( Collections::SqlCollection *collection )
    : m_collection( collection )
    , m_debugDatabaseContent( false )
{
    m_debugDatabaseContent = Amarok::config( QStringLiteral("SqlCollection") ).readEntry( "DebugDatabaseContent", false );
}

DatabaseUpdater::~DatabaseUpdater()
{
    //nothing to do
}

bool
DatabaseUpdater::needsUpdate() const
{
    return adminValue( QStringLiteral("DB_VERSION") ) != DB_VERSION;
}

bool
DatabaseUpdater::schemaExists() const
{
    return adminValue( QStringLiteral("DB_VERSION") ) != 0;
}

bool
DatabaseUpdater::update()
{
    DEBUG_BLOCK
    int dbVersion = adminValue( QStringLiteral("DB_VERSION") );

    debug() << "Database version: " << dbVersion;

    if( dbVersion == 0 )
    {
        createTables();
        QString query = QStringLiteral( "INSERT INTO admin(component, version) VALUES ('DB_VERSION', %1);" ).arg( DB_VERSION );
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
                Q_FALLTHROUGH();
            case 15:
                upgradeVersion15to16();
                dbVersion = 16; // be sure to update this manually when introducing new version!
        }

        // Apply MySQL-specific upgrade operations (charset/engine conversions, etc.)
        if( s_mySqlUpgradeFunc )
            s_mySqlUpgradeFunc( m_collection->sqlStorage().data(), dbVersion );

        QString query = QStringLiteral( "UPDATE admin SET version = %1 WHERE component = 'DB_VERSION';" ).arg( dbVersion );
        m_collection->sqlStorage()->query( query );

        //NOTE: A rescan will be triggered automatically as a result of an upgrade.  Don't trigger it here, as the
        //collection isn't fully initialized and this will trigger a crash/assert.
        return true;
    }

    if( dbVersion > DB_VERSION )
    {
        KMessageBox::error(nullptr,
                QStringLiteral("<p>The Amarok collection database was created by a newer version of Amarok, "
                "and this version of Amarok cannot use it.</p>"),
                QStringLiteral("Database Type Unknown") );
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

    m_collection->sqlStorage()->query( QStringLiteral("ALTER TABLE tracks "
                         "ADD COLUMN albumgain FLOAT, "
                         "ADD COLUMN albumpeakgain FLOAT, "
                         "ADD COLUMN trackgain FLOAT,"
                         "ADD COLUMN trackpeakgain FLOAT;") );
}

void
DatabaseUpdater::upgradeVersion2to3()
{
    DEBUG_BLOCK;

    auto storage = m_collection->sqlStorage();
    storage->query( QStringLiteral("DROP TABLE devices;") );

    QString create = QStringLiteral("CREATE TABLE devices "
                     "(id ") + storage->idType() +
                     QStringLiteral(",type ") + storage->textColumnType() +
                     QStringLiteral(",label ") + storage->textColumnType() +
                     QStringLiteral(",lastmountpoint ") + storage->textColumnType() +
                     QStringLiteral(",uuid ") + storage->textColumnType() +
                     QStringLiteral(",servername ") + storage->textColumnType() +
                     QStringLiteral(",sharename ") + storage->textColumnType() + QStringLiteral(");");
    storage->query( create );
    storage->query( QStringLiteral("CREATE INDEX devices_type ON devices( type );") );
    storage->query( QStringLiteral("CREATE UNIQUE INDEX devices_uuid ON devices( uuid );") );
    storage->query( QStringLiteral("CREATE INDEX devices_rshare ON devices( servername, sharename );") );

}

void
DatabaseUpdater::upgradeVersion3to4()
{
    auto storage = m_collection->sqlStorage();

    storage->query( QStringLiteral("CREATE TABLE statistics_permanent "
                         "(url ") + storage->exactTextColumnType() +
                         QStringLiteral(",firstplayed DATETIME"
                         ",lastplayed DATETIME"
                         ",score FLOAT"
                         ",rating INTEGER DEFAULT 0"
                         ",playcount INTEGER)") );
    storage->query( QStringLiteral("CREATE UNIQUE INDEX ON statistics_permanent(url)") );
    //Note: the above index query is invalid, but kept here for posterity

    storage->query( QStringLiteral("CREATE TABLE statistics_tag "
                         "(name ") + storage->textColumnType() +
                         QStringLiteral(",artist ") + storage->textColumnType() +
                         QStringLiteral(",album ") + storage->textColumnType() +
                         QStringLiteral(",firstplayed DATETIME"
                         ",lastplayed DATETIME"
                         ",score FLOAT"
                         ",rating INTEGER DEFAULT 0"
                         ",playcount INTEGER)") );
    storage->query( QStringLiteral("CREATE UNIQUE INDEX ON statistics_tag(name,artist,album)") );
    //Note: the above index query is invalid, but kept here for posterity
}

void
DatabaseUpdater::upgradeVersion4to5()
{
    auto storage = m_collection->sqlStorage();

    //first, drop tables that can easily be recreated by doing an update
    QStringList dropTables;
    dropTables << QStringLiteral("jamendo_albums") << QStringLiteral("jamendo_artists") << QStringLiteral("jamendo_genre") << QStringLiteral("jamendo_tracks");
    dropTables << QStringLiteral("magnatune_albums") << QStringLiteral("magnatune_artists") << QStringLiteral("magnatune_genre") << QStringLiteral("magnatune_moods") << QStringLiteral("magnatune_tracks");
    dropTables << QStringLiteral("opmldirectory_albums") << QStringLiteral("opmldirectory_artists") << QStringLiteral("opmldirectory_genre") << QStringLiteral("opmldirectory_tracks");

    for( const QString &table : dropTables )
        storage->query( QStringLiteral("DROP TABLE ") + table );

}

void
DatabaseUpdater::upgradeVersion5to6()
{
    DEBUG_BLOCK

    auto storage = m_collection->sqlStorage();

    //first, drop tables that can easily be recreated by doing an update
    QStringList dropTables;
    dropTables << QStringLiteral("jamendo_albums") << QStringLiteral("jamendo_artists") << QStringLiteral("jamendo_genre") << QStringLiteral("jamendo_tracks");
    dropTables << QStringLiteral("magnatune_albums") << QStringLiteral("magnatune_artists") << QStringLiteral("magnatune_genre") << QStringLiteral("magnatune_moods") << QStringLiteral("magnatune_tracks");
    dropTables << QStringLiteral("opmldirectory_albums") << QStringLiteral("opmldirectory_artists") << QStringLiteral("opmldirectory_genre") << QStringLiteral("opmldirectory_tracks");

    for( const QString &table : dropTables )
        storage->query( QStringLiteral("DROP TABLE ") + table );

    //now, the rest of them
    QStringList tables;
    tables << QStringLiteral("admin") << QStringLiteral("albums") << QStringLiteral("amazon") << QStringLiteral("artists") << QStringLiteral("bookmark_groups") << QStringLiteral("bookmarks");
    tables << QStringLiteral("composers") << QStringLiteral("devices") << QStringLiteral("directories") << QStringLiteral("genres") << QStringLiteral("images") << QStringLiteral("labels") << QStringLiteral("lyrics");
    tables << QStringLiteral("playlist_groups") << QStringLiteral("playlist_tracks") << QStringLiteral("playlists");
    tables << QStringLiteral("podcastchannels") << QStringLiteral("podcastepisodes");
    tables << QStringLiteral("statistics") << QStringLiteral("statistics_permanent") << QStringLiteral("statistics_tag");
    tables << QStringLiteral("tracks") << QStringLiteral("urls") << QStringLiteral("urls_labels") << QStringLiteral("years");

    storage->query( QStringLiteral("CREATE INDEX devices_rshare ON devices( servername, sharename );") );
    storage->query( QStringLiteral("CREATE UNIQUE INDEX lyrics_url ON lyrics(url);") );
    storage->query( QStringLiteral("CREATE UNIQUE INDEX urls_id_rpath ON urls(deviceid, rpath);") );
    storage->query( QStringLiteral("CREATE UNIQUE INDEX stats_tag_name_artist_album ON statistics_tag(name,artist,album)") );
}

void
DatabaseUpdater::upgradeVersion6to7()
{
    DEBUG_BLOCK

    Q_UNUSED( m_collection )
}


void
DatabaseUpdater::upgradeVersion7to8()
{
    DEBUG_BLOCK

    auto storage = m_collection->sqlStorage();

    QHash< int, int > trackLengthHash;

    // First, get the lengths from the db and insert them into a hash
    const QStringList result = storage->query( QStringLiteral("SELECT id, length FROM tracks") );

    QListIterator<QString> iter(result);
    while( iter.hasNext() )
        trackLengthHash.insert( iter.next().toInt(), iter.next().toInt() );

    // Now Iterate over the hash, and insert each track back in, changing the length to milliseconds
    QHashIterator<int,int> iter2( trackLengthHash );
    const QString updateString = QStringLiteral( "UPDATE tracks SET length=%1 WHERE id=%2 ;");
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
    deleteAllRedundant( QStringLiteral("album") );
}

void
DatabaseUpdater::upgradeVersion9to10()
{
    DEBUG_BLOCK

    auto storage = m_collection->sqlStorage();

    //first, drop tables that can easily be recreated by doing an update
    QStringList dropTables;
    dropTables << QStringLiteral("jamendo_albums") << QStringLiteral("jamendo_artists") << QStringLiteral("jamendo_genre") << QStringLiteral("jamendo_tracks");
    dropTables << QStringLiteral("magnatune_albums") << QStringLiteral("magnatune_artists") << QStringLiteral("magnatune_genre") << QStringLiteral("magnatune_moods") << QStringLiteral("magnatune_tracks");
    dropTables << QStringLiteral("opmldirectory_albums") << QStringLiteral("opmldirectory_artists") << QStringLiteral("opmldirectory_genre") << QStringLiteral("opmldirectory_tracks");

    for( const QString &table : dropTables )
        storage->query( QStringLiteral("DROP TABLE ") + table );

    storage->query( QStringLiteral("CREATE INDEX devices_rshare ON devices( servername, sharename );") );
    storage->query( QStringLiteral("CREATE UNIQUE INDEX lyrics_url ON lyrics(url);") );
    storage->query( QStringLiteral("CREATE UNIQUE INDEX urls_id_rpath ON urls(deviceid, rpath);") );
    storage->query( QStringLiteral("CREATE UNIQUE INDEX stats_tag_name_artist_album ON statistics_tag(name,artist,album)") );
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
    m_collection->sqlStorage()->query( QStringLiteral("UPDATE urls SET uniqueid = REPLACE(uniqueid, 'MB_', 'mb-');") );
}

void
DatabaseUpdater::upgradeVersion13to14()
{
    DEBUG_BLOCK
    Q_UNUSED( m_collection )
}

void
DatabaseUpdater::upgradeVersion14to15()
{
    DEBUG_BLOCK
    auto storage = m_collection->sqlStorage();

    // there may be a stale unique index on the urls table, remove it if it is there:
    debug() << "dropping obsolete INDEX uniqueid on table urls (if exists)";
    storage->dropIndex( QStringLiteral("uniqueid"), QStringLiteral("urls") );
}

void
DatabaseUpdater::upgradeVersion15to16()
{
    auto storage = m_collection->sqlStorage();

    // Recreate devices_rshare index (works on both backends via virtual dispatch)
    storage->dropIndex( QStringLiteral("devices_rshare"), QStringLiteral("devices") );
    storage->query( QStringLiteral("CREATE INDEX devices_rshare ON devices( servername, sharename )") );
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

    QStringList tables = storage->queryTables();
    for( const QString &table : tables )
        storage->checkTable( table, full );
}


void
DatabaseUpdater::createTables() const
{
    DEBUG_BLOCK

    auto storage = m_collection->sqlStorage();

    // see docs/database/amarokTables.svg for documentation about database layout
    {
        QString c = QStringLiteral("CREATE TABLE admin (component ") + storage->textColumnType() + QStringLiteral(", version INTEGER)") + storage->sqlCreateTableOptions() + QStringLiteral(";");
        storage->query( c );
    }
    {
        QString create = QStringLiteral("CREATE TABLE devices "
                         "(id ") + storage->idType() +
                         QStringLiteral(",type ") + storage->textColumnType() +
                         QStringLiteral(",label ") + storage->textColumnType() +
                         QStringLiteral(",lastmountpoint ") + storage->textColumnType() +
                         QStringLiteral(",uuid ") + storage->textColumnType() +
                         QStringLiteral(",servername ") + storage->textColumnType(80) +
                         QStringLiteral(",sharename ") + storage->textColumnType(240) + QStringLiteral(")") + storage->sqlCreateTableOptions() + QStringLiteral(";");
        storage->query( create );
        storage->query( QStringLiteral("CREATE INDEX devices_type ON devices( type );") );
        storage->query( QStringLiteral("CREATE UNIQUE INDEX devices_uuid ON devices( uuid );") );
        // changing from utf8 to utf8mb4 increased bytes per character, so servername(80)+sharename(240) doesn't fit in 1000 byte max key
        // avoid by creating only prefix indexes. this index is not unique so shouldn't probably cause problems in any case
        if (storage->supportsPrefixIndexes())
            storage->query( QStringLiteral("CREATE INDEX devices_rshare ON devices( servername(60), sharename(180) );") );
        else
            storage->query( QStringLiteral("CREATE INDEX devices_rshare ON devices( servername, sharename );") );
    }
    {
        QString create = QStringLiteral("CREATE TABLE urls "
                         "(id ") + storage->idType() +
                         QStringLiteral(",deviceid INTEGER"
                         ",rpath ") + storage->exactIndexableTextColumnType() + QStringLiteral(" NOT NULL") +
                         QStringLiteral(",directory INTEGER"
                          ",uniqueid ") + storage->exactTextColumnType(128) + QStringLiteral(" UNIQUE)") + storage->sqlCreateTableOptions() + QStringLiteral(";");
        storage->query( create );
        storage->query( QStringLiteral("CREATE UNIQUE INDEX urls_id_rpath ON urls(deviceid, rpath);") );
        storage->query( QStringLiteral("CREATE INDEX urls_uniqueid ON urls(uniqueid);") );
        storage->query( QStringLiteral("CREATE INDEX urls_directory ON urls(directory);") );
    }
    {
        QString create = QStringLiteral("CREATE TABLE directories "
                         "(id ") + storage->idType() +
                         QStringLiteral(",deviceid INTEGER"
                         ",dir ") + storage->exactTextColumnType() + QStringLiteral(" NOT NULL") +
                          QStringLiteral(",changedate BIGINT)") + storage->sqlCreateTableOptions() + QStringLiteral(";");
        storage->query( create );
        storage->query( QStringLiteral("CREATE INDEX directories_deviceid ON directories(deviceid);") );
    }
    {
        QString create = QStringLiteral("CREATE TABLE artists "
                         "(id ") + storage->idType() +
                          QStringLiteral(",name ") + storage->textColumnType() + QStringLiteral(" NOT NULL)") + storage->sqlCreateTableOptions() + QStringLiteral(";");
        storage->query( create );
        storage->query( QStringLiteral("CREATE UNIQUE INDEX artists_name ON artists(name);") );
    }
    {
        QString create = QStringLiteral("CREATE TABLE images "
                         "(id ") + storage->idType() +
                         QStringLiteral(",path ") + storage->textColumnType() + QStringLiteral(" NOT NULL)") + storage->sqlCreateTableOptions() + QStringLiteral(";");
        storage->query( create );
        storage->query( QStringLiteral("CREATE UNIQUE INDEX images_name ON images(path);") );
    }
    {
        QString c = QStringLiteral("CREATE TABLE albums "
                    "(id ") + storage->idType() +
                    QStringLiteral(",name ") + storage->textColumnType() + QStringLiteral(" NOT NULL"
                    ",artist INTEGER") +
                    QStringLiteral(",image INTEGER)") + storage->sqlCreateTableOptions() + QStringLiteral(";");
        storage->query( c );
        storage->query( QStringLiteral("CREATE INDEX albums_name ON albums(name);") );
        storage->query( QStringLiteral("CREATE INDEX albums_artist ON albums(artist);") );
        storage->query( QStringLiteral("CREATE INDEX albums_image ON albums(image);") );
        storage->query( QStringLiteral("CREATE UNIQUE INDEX albums_name_artist ON albums(name,artist);") );
        //the index below should not be necessary. uncomment if a query plan shows it is
        //storage->query( QStringLiteral("CREATE UNIQUE INDEX albums_artist_name ON albums(artist,name);" );
    }
    {
        QString create = QStringLiteral("CREATE TABLE genres "
                         "(id ") + storage->idType() +
                          QStringLiteral(",name ") + storage->textColumnType() + QStringLiteral(" NOT NULL)") + storage->sqlCreateTableOptions() + QStringLiteral(";");
        storage->query( create );
        storage->query( QStringLiteral("CREATE UNIQUE INDEX genres_name ON genres(name);") );
    }
    {
        QString create = QStringLiteral("CREATE TABLE composers "
                         "(id ") + storage->idType() +
                         QStringLiteral(",name ") + storage->textColumnType() + QStringLiteral(" NOT NULL)") + storage->sqlCreateTableOptions() + QStringLiteral(";");
        storage->query( create );
        storage->query( QStringLiteral("CREATE UNIQUE INDEX composers_name ON composers(name);") );
    }
    {
        QString create = QStringLiteral("CREATE TABLE years "
                         "(id ") + storage->idType() +
                          QStringLiteral(",name ") + storage->textColumnType() + QStringLiteral(" NOT NULL)") + storage->sqlCreateTableOptions() + QStringLiteral(";");
        storage->query( create );
        storage->query( QStringLiteral("CREATE UNIQUE INDEX years_name ON years(name);") );
    }
    {
        QString c = QStringLiteral("CREATE TABLE tracks "
                    "(id ") + storage->idType() +
                    QStringLiteral(",url INTEGER"
                    ",artist INTEGER"
                    ",album INTEGER"
                    ",genre INTEGER"
                    ",composer INTEGER"
                    ",year INTEGER"
                    ",title ") + storage->textColumnType() +
                    QStringLiteral(",comment ") + storage->longTextColumnType() +
                    QStringLiteral(",tracknumber INTEGER"
                    ",discnumber INTEGER"
                    ",bitrate INTEGER"
                    ",length INTEGER"
                    ",samplerate INTEGER"
                    ",filesize INTEGER"
                    ",filetype INTEGER"     //does this still make sense?
                    ",bpm FLOAT"
                    ",createdate BIGINT"   // this is the track creation time
                    ",modifydate BIGINT"   // UNUSED currently
                    ",albumgain FLOAT"
                    ",albumpeakgain FLOAT" // decibels, relative to albumgain
                    ",trackgain FLOAT"
                    ",trackpeakgain FLOAT" // decibels, relative to trackgain
                    ")") + storage->sqlCreateTableOptions() + QStringLiteral(";");

        storage->query( c );
        storage->query( QStringLiteral("CREATE UNIQUE INDEX tracks_url ON tracks(url);") );

        QStringList indices;
        indices << QStringLiteral("id") << QStringLiteral("artist") << QStringLiteral("album") << QStringLiteral("genre") << QStringLiteral("composer") << QStringLiteral("year") << QStringLiteral("title");
        indices << QStringLiteral("discnumber") << QStringLiteral("createdate") << QStringLiteral("length") << QStringLiteral("bitrate") << QStringLiteral("filesize");
        for( const QString &index : indices )
        {
            QString query = QStringLiteral( "CREATE INDEX tracks_%1 ON tracks(%2);" ).arg( index, index );
            storage->query( query );
        }
    }
    {
        QString c = QStringLiteral("CREATE TABLE statistics "
                    "(id ") + storage->idType() +
                    QStringLiteral(",url INTEGER NOT NULL"
                    ",createdate INTEGER" // this is the first played time
                    ",accessdate INTEGER" // this is the last played time
                    ",score FLOAT"
                    ",rating INTEGER NOT NULL DEFAULT 0" // the "default" undefined rating is 0. We cannot display anything else.
                    ",playcount INTEGER NOT NULL DEFAULT 0" // a track is either played or not.
                    ",deleted BOOL NOT NULL DEFAULT ") + storage->boolFalse() +
                     QStringLiteral(")") + storage->sqlCreateTableOptions() + QStringLiteral(";");
        storage->query( c );
        storage->query( QStringLiteral("CREATE UNIQUE INDEX statistics_url ON statistics(url);") );
        QStringList indices;
        indices << QStringLiteral("createdate") << QStringLiteral("accessdate") << QStringLiteral("score") << QStringLiteral("rating") << QStringLiteral("playcount");
        for( const QString &index : indices )
        {
            QString q = QStringLiteral( "CREATE INDEX statistics_%1 ON statistics(%2);" ).arg( index, index );
            storage->query( q );
        }
    }
    {
        QString q = QStringLiteral("CREATE TABLE labels "
                    "(id ") + storage->idType() +
                    QStringLiteral(",label ") + storage->textColumnType() +
                    QStringLiteral(")") + storage->sqlCreateTableOptions() + QStringLiteral(";");
        storage->query( q );
        storage->query( QStringLiteral("CREATE UNIQUE INDEX labels_label ON labels(label);") );

        QString r = QStringLiteral("CREATE TABLE urls_labels(url INTEGER, label INTEGER);");
        storage->query( r );
        storage->query( QStringLiteral("CREATE INDEX urlslabels_url ON urls_labels(url);") );
        storage->query( QStringLiteral("CREATE INDEX urlslabels_label ON urls_labels(label);") );
    }
    {
        QString q = QStringLiteral("CREATE TABLE amazon ("
                    "asin ") + storage->textColumnType( 20 ) +
                    QStringLiteral(",locale ") + storage->textColumnType( 2 ) +
                    QStringLiteral(",filename ") + storage->textColumnType( 33 ) +
                    QStringLiteral(",refetchdate INTEGER )") + storage->sqlCreateTableOptions() + QStringLiteral(";");
        storage->query( q );
        storage->query( QStringLiteral("CREATE INDEX amazon_date ON amazon(refetchdate);") );
    }
    {
        QString q = QStringLiteral("CREATE TABLE lyrics ("
                    "url INTEGER PRIMARY KEY"
                    ",lyrics ") + storage->longTextColumnType() +
                    QStringLiteral(")") + storage->sqlCreateTableOptions() + QStringLiteral(";");
        storage->query( q );
    }
    storage->query( QStringLiteral("INSERT INTO admin(component,version) " 
                          "VALUES('AMAROK_TRACK',") + QString::number( DB_VERSION ) + QStringLiteral(");") );
    {
         storage->query( QStringLiteral("CREATE TABLE statistics_permanent "
                            "(url ") + storage->exactIndexableTextColumnType() + QStringLiteral(" NOT NULL") +
                            QStringLiteral(",firstplayed DATETIME"
                            ",lastplayed DATETIME"
                            ",score FLOAT"
                            ",rating INTEGER DEFAULT 0"
                            ",playcount INTEGER)") + storage->sqlCreateTableOptions() + QStringLiteral(";") );
        storage->query( QStringLiteral("CREATE UNIQUE INDEX stats_perm_url ON statistics_permanent(url)") );

        storage->query( QStringLiteral("CREATE TABLE statistics_tag "
                             "(name ") + storage->textColumnType(108) +
                             QStringLiteral(",artist ") + storage->textColumnType(108) +
                             QStringLiteral(",album ") + storage->textColumnType(108) +
                             QStringLiteral(",firstplayed DATETIME"
                             ",lastplayed DATETIME"
                             ",score FLOAT"
                             ",rating INTEGER DEFAULT 0"
                             ",playcount INTEGER)") + storage->sqlCreateTableOptions() + QStringLiteral("") );
        storage->query( QStringLiteral("CREATE UNIQUE INDEX stats_tag_name_artist_album ON statistics_tag(name,artist,album)") );
    }
}

int
DatabaseUpdater::adminValue( const QString &key ) const
{
    auto storage = m_collection->sqlStorage();

    QStringList values = storage->query(
            QStringLiteral( "SELECT version FROM admin WHERE component = '%1';")
            .arg(storage->escape( key ) ) );
    if( values.isEmpty() )
        return 0;

    return values.first().toInt();
}

void
DatabaseUpdater::deleteAllRedundant( const QString &type )
{
    auto storage = m_collection->sqlStorage();

    const QString tablename = type + QLatin1Char('s');
    if( type == QStringLiteral("artist") )
        storage->query( QStringLiteral( "DELETE FROM artists "
                                 "WHERE id NOT IN ( SELECT artist FROM tracks WHERE artist IS NOT NULL ) AND "
                                 "id NOT IN ( SELECT artist FROM albums WHERE artist IS NOT NULL )") );
    else
        storage->query( QStringLiteral( "DELETE FROM %1 "
                                 "WHERE id NOT IN ( SELECT %2 FROM tracks WHERE %2 IS NOT NULL )" ).
                        arg( tablename, type ) );
}

void
DatabaseUpdater::deleteOrphanedByDirectory( const QString &table )
{
    auto storage = m_collection->sqlStorage();
    QString query( QStringLiteral("DELETE FROM %1 WHERE directory NOT IN ( SELECT id FROM directories )") );
    storage->query( query.arg( table ) );
}

void
DatabaseUpdater::deleteOrphanedByUrl( const QString &table )
{
    auto storage = m_collection->sqlStorage();
    QString query( QStringLiteral("DELETE FROM %1 WHERE url NOT IN ( SELECT id FROM urls )") );
    storage->query( query.arg( table ) );
}

void
DatabaseUpdater::removeFilesInDir( int deviceid, const QString &rdir )
{
    auto storage = m_collection->sqlStorage();

    QString select = QStringLiteral( "SELECT urls.id FROM urls LEFT JOIN directories ON urls.directory = directories.id "
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
                ids += QLatin1Char(',');
            ids += id;
        }
        QString drop = QStringLiteral( "DELETE FROM tracks WHERE url IN (%1);" ).arg( ids );
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
    QStringList columns = storage->queryColumns( ctable );

    if( columns.isEmpty() )
        return; //no table with that name

    // ok. it was probably a little bit unlucky to name a table statistics
    // that clashes with INFORMATION_SCHEMA.statistics, a build in table.
    if( table == QStringLiteral("statistics") && columns.count() > 15 )
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
            select.append( QLatin1Char(',') );
        select.append( column );
    }

    QString query = QStringLiteral("SELECT %1 FROM %2");

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

