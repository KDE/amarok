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

#include "debug.h"
#include "sqlcollection.h"

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
    return false;
}

void
DatabaseUpdater::update()
{
    //setup database
}

void
DatabaseUpdater::createTemporaryTables()
{
    //TODO
}

void
DatabaseUpdater::prepareTemporaryTables()
{
    //TODO
}

void
DatabaseUpdater::removeTemporaryTables()
{
    //TODO
}

void
DatabaseUpdater::copyToPermanentTables()
{
    DEBUG_BLOCK

    m_collection->insert( "INSERT INTO tracks SELECT * FROM tracks_temp;", QString() );

    //handle artists before albums
    QStringList artistIdList = m_collection->query( "SELECT artist.id FROM artist;" );
    QString artistIds = "-1";
    foreach( QString artistId, artistIdList )
    {
        artistIds += ',';
        artistIds += artistId;
    }
    m_collection->insert( QString ( "INSERT INTO artist SELECT * FROM artist_temp WHERE artist_temp.id NOT IN ( %1 );" ).arg( artistIds ), QString() );

    QStringList albumIdList = m_collection->query( "SELECT album.id FROM album;" );
    //in an empty database, albumIdList is empty. This would result in a SQL query like NOT IN ( ) without
    //the -1 below which is invalid SQL. The auto generated values start at 1 so this is fine
    QString albumIds = "-1";
    foreach( QString albumId, albumIdList )
    {
        albumIds += ',';
        albumIds += albumId;
    }
    m_collection->insert( QString ( "INSERT INTO album SELECT * FROM album_temp WHERE album_temp.id NOT IN ( %1 );" ).arg( albumIds ), QString() );

    QStringList composerIdList = m_collection->query( "SELECT composer.id FROM composer;" );
    QString composerIds = "-1";
    foreach( QString composerId, composerIdList )
    {
        composerIds += ',';
        composerIds += composerId;
    }
    m_collection->insert( QString ( "INSERT INTO composer SELECT * FROM composer_temp WHERE composer_temp.id NOT IN ( %1 );" ).arg( composerIds ), QString() );

    QStringList genreIdList = m_collection->query( "SELECT genre.id FROM genre;" );
    QString genreIds = "-1";
    foreach( QString genreId, genreIdList )
    {
        genreIds += ',';
        genreIds += genreId;
    }
    m_collection->insert( QString ( "INSERT INTO genre SELECT * FROM genre_temp WHERE genre_temp.id NOT IN ( %1 );" ).arg( genreIds ), QString() );

    QStringList yearIdList = m_collection->query( "SELECT year.id FROM year;" );
    QString yearIds = "-1";
    foreach( QString yearId, yearIdList )
    {
        yearIds += ',';
        yearIds += yearId;
    }
    m_collection->insert( QString ( "INSERT INTO year SELECT * FROM year_temp WHERE year_temp.id NOT IN ( %1 );" ).arg( yearIds ), QString() );

    //insert( "INSERT INTO images SELECT * FROM images_temp;", NULL );
    //insert( "INSERT INTO embed SELECT * FROM embed_temp;", NULL );
    m_collection->insert( "INSERT INTO directories SELECT * FROM directories_temp;", QString() );
    //insert( "INSERT INTO uniqueid SELECT * FROM uniqueid_temp;", NULL );
}

void
DatabaseUpdater::createTables() const
{
    // see docs/database/amarokTables.svg for documentation about database layout
    {
        QString c = "CREATE TABLE admin (key " + m_collection->textColumnType() + ", version INTEGER);";
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
                         ",rpath " + m_collection->exactTextColumnType() + ");";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX urls_id_rpath ON urls(deviceid, rpath);" );
    }
    {
        QString create = "CREATE TABLE artists "
                         "(id " + m_collection->idType() +
                         ",name " + m_collection->textColumnType() + " NOT NULL);";
        m_collection->query( create );
        m_collection->query( "CREATE UNIQUE INDEX artists_name ON artists(name);" );
    }
    {
        QString c = "CREATE TABLE albums "
                    "(id " + m_collection->idType() +
                    ",name " + m_collection->textColumnType() + " NOT NULL"
                    ",artist INTEGER);";
        m_collection->query( c );
        m_collection->query( "CREATE INDEX albums_name ON albums(name);" );
        m_collection->query( "CREATE INDEX albums_artist ON albums(artist);" );
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
        indices << "track" << "discnumber" << "createdate" << "length" << "bitrate" << "filesize";
        foreach( QString index, indices )
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
                    ");";
        m_collection->query( c );
        m_collection->query( "CREATE UNIQUE INDEX statistics_url ON statistics(url);" );
        QStringList indices;
        indices << "createdate" << "accessdate" << "score" << "rating" << "playcount";
        foreach( QString index, indices )
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
    m_collection->query( "INSERT INTO admin(key,version) "
                          "VALUES('AMAROK_TRACK'," + QString::number( DB_VERSION ) + ");" );
}

void
DatabaseUpdater::deleteAllRedundant( const QString &table )
{
    m_collection->query( QString( "DELETE FROM %1 WHERE id NOT IN ( SELECT %2 FROM tracks )" ).arg( table, table ) );
}

