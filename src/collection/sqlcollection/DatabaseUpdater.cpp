/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "DatabaseUpdater.h"

#include "Debug.h"
#include "MountPointManager.h"
#include "SqlCollection.h"

static const int DB_VERSION = 1;

DatabaseUpdater::DatabaseUpdater( SqlCollection *collection )
    : m_collection( collection )
{
    //nothing to do
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
    const int dbVersion = adminValue( "DB_VERSION" );
    if( dbVersion == 0 )
    {
        createTables();
        m_collection->query( "INSERT INTO admin(component, version) VALUES ('DB_VERSION', 1);" );
    }
    else if( dbVersion > DB_VERSION )
    {
        cleanPermanentTables();
        createTables();
        m_collection->query( "INSERT INTO admin(component, version) VALUES( 'DB_VERSION', 1);" );
    }
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
                         ",rpath " + m_collection->exactTextColumnType() +
                         ",directory INTEGER"
                         ",uniqueid " + m_collection->exactTextColumnType(128) + " UNIQUE);";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX urls_id_rpath_temp ON urls_temp(deviceid, rpath);" );
        m_collection->query( "CREATE INDEX urls_temp_uniqueid ON urls_temp(uniqueid);" );
    }
    {
        QString create = "CREATE TEMPORARY TABLE directories_temp "
                         "(id " + m_collection->idType() +
                         ",deviceid INTEGER"
                         ",dir " + m_collection->exactTextColumnType() + 
                         ",changedate INTEGER);";
        m_collection->query( create );
    }
    {
        QString create = "CREATE TEMPORARY TABLE artists_temp "
                         "(id " + m_collection->idType() +
                         ",name " + m_collection->textColumnType() + " NOT NULL);";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX artists_temp_name ON artists_temp(name);" );
    }
    {
        QString c = "CREATE TEMPORARY TABLE albums_temp "
                    "(id " + m_collection->idType() +
                    ",name " + m_collection->textColumnType() + " NOT NULL"
                    ",artist INTEGER" +
                    ",image INTEGER);";
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
                         ",name " + m_collection->textColumnType() + " NOT NULL);";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX genres_temp_name ON genres_temp(name);" );
    }
    {
        QString create = "CREATE TEMPORARY TABLE composers_temp "
                         "(id " + m_collection->idType() +
                         ",name " + m_collection->textColumnType() + " NOT NULL);";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX composers_temp_name ON composers_temp(name);" );
    }
    {
        QString create = "CREATE TEMPORARY TABLE years_temp "
                         "(id " + m_collection->idType() +
                         ",name " + m_collection->textColumnType() + " NOT NULL);";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX years_temp_name ON years_temp(name);" );
    }
    {
        QString create = "CREATE TEMPORARY TABLE images_temp "
                         "(id " + m_collection->idType() +
                         ",path " + m_collection->textColumnType() + " NOT NULL);";
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
                    ");";

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
DatabaseUpdater::copyToPermanentTables()
{
    DEBUG_BLOCK

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

    m_collection->sendChangedSignal();
}

void
DatabaseUpdater::createTables() const
{
    DEBUG_BLOCK
    // see docs/database/amarokTables.svg for documentation about database layout
    {
        QString c = "CREATE TABLE admin (component " + m_collection->textColumnType() + ", version INTEGER);";
        m_collection->query( c );
    }
    {
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
        m_collection->query( "CREATE INDEX devices_uuid ON devices( uuid );" );
        m_collection->query( "CREATE INDEX devices_rshare ON devices( servername, sharename );" );
    }
    {
        QString create = "CREATE TABLE urls "
                         "(id " + m_collection->idType() +
                         ",deviceid INTEGER"
                         ",rpath " + m_collection->exactTextColumnType() + 
                         ",directory INTEGER"
                         ",uniqueid " + m_collection->exactTextColumnType(128) + " UNIQUE);";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX urls_id_rpath ON urls(deviceid, rpath);" );
        m_collection->query( "CREATE INDEX urls_uniqueid ON urls(uniqueid);" );
    }
    {
        QString create = "CREATE TABLE directories "
                         "(id " + m_collection->idType() +
                         ",deviceid INTEGER"
                         ",dir " + m_collection->exactTextColumnType() + 
                         ",changedate INTEGER);";
        m_collection->query( create );
        m_collection->query( "CREATE INDEX directories_deviceid ON directories(deviceid);" );
    }
    {
        QString create = "CREATE TABLE artists "
                         "(id " + m_collection->idType() +
                         ",name " + m_collection->textColumnType() + " NOT NULL);";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX artists_name ON artists(name);" );
    }
    {
        QString create = "CREATE TABLE images "
                         "(id " + m_collection->idType() +
                         ",path " + m_collection->textColumnType() + " NOT NULL);";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX images_name ON images(path);" );
    }
    {
        QString c = "CREATE TABLE albums "
                    "(id " + m_collection->idType() +
                    ",name " + m_collection->textColumnType() + " NOT NULL"
                    ",artist INTEGER" +
                    ",image INTEGER);";
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
                         ",name " + m_collection->textColumnType() + " NOT NULL);";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX genres_name ON genres(name);" );
    }
    {
        QString create = "CREATE TABLE composers "
                         "(id " + m_collection->idType() +
                         ",name " + m_collection->textColumnType() + " NOT NULL);";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX composers_name ON composers(name);" );
    }
    {
        QString create = "CREATE TABLE years "
                         "(id " + m_collection->idType() +
                         ",name " + m_collection->textColumnType() + " NOT NULL);";
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
                    ");";

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
                    ");";
        m_collection->query( c );
        m_collection->query( "CREATE UNIQUE INDEX statistics_url ON statistics(url);" );
        QStringList indices;
        indices << "createdate" << "accessdate" << "score" << "rating" << "playcount" << "uniqueid";
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
                    ");";
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
                    ",refetchdate INTEGER );";
        m_collection->query( q );
        m_collection->query( "CREATE INDEX amazon_date ON amazon(refetchdate);" );
    }
    {
        QString q = "CREATE TABLE lyrics ("
                    "id " + m_collection->idType() +
                    ",url " + m_collection->exactTextColumnType() +
                    ",lyrics " + m_collection->longTextColumnType() +
                    ");";
        m_collection->query( q );
        m_collection->query( "CREATE UNIQUE INDEX lyrics_url ON lyrics(url);" );
    }
    m_collection->query( "INSERT INTO admin(component,version) "
                          "VALUES('AMAROK_TRACK'," + QString::number( DB_VERSION ) + ");" );
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
        QStringList::ConstIterator it = idResult.begin(), end = idResult.end();
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

