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

#include "sqlcollection.h"

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
DatabaseUpdater::createTables() const
{
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
                    ",albums INTEGER"
                    ",genres INTEGER"
                    ",composers INTEGER"
                    ",years INTEGER"
                    ",title " + m_collection->textColumnType() +
                    ",comment " + m_collection->longTextColumnType() +
                    ",track INTEGER"
                    ",discnumber INTEGER"
                    ",bitrate INTEGER"
                    ",length INTEGER"
                    ",samplerate INTEGER"
                    ",filesize INTEGER"
                    ",filetype INTEGER"     //does this still make sense?
                    ",bpm FLOAT"
                    ",createdate INTEGER"   //are the two dates needed?
                    ",modifydate INTEGER);";

        m_collection->query( c );
        m_collection->query( "CREATE UNIQUE INDEX tracks_url ON tracks(url);" );
        m_collection->query( "CREATE INDEX tracks_artist ON tracks(artist);" );
        m_collection->query( "CREATE INDEX tracks_albums ON tracks(album);" );
    }
}
