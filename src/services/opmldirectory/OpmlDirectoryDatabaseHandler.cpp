/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "OpmlDirectoryDatabaseHandler.h"

#include "collection/CollectionManager.h"
#include "Debug.h"
#include "collection/SqlStorage.h"

using namespace Meta;

OpmlDirectoryDatabaseHandler::OpmlDirectoryDatabaseHandler()
{}

OpmlDirectoryDatabaseHandler::~OpmlDirectoryDatabaseHandler()
{}

void
OpmlDirectoryDatabaseHandler::createDatabase()
{
    //Get database instance
    SqlStorage *db = CollectionManager::instance()->sqlStorage();

    QString genreAutoIncrement = "";

    // create table containing feeds
    QString queryString = "CREATE TABLE opmldirectory_tracks ("
                          "id INTEGER PRIMARY KEY AUTO_INCREMENT, "
                          "name " + db->textColumnType() + ',' +
                          "track_number INTEGER,"
                          "length INTEGER,"
                          "preview_url " + db->exactTextColumnType() + ',' +
                          "album_id INTEGER,"
                          "artist_id INTEGER ) ENGINE = MyISAM;";

    debug() << "Creating opmldirectory_tracks: " << queryString;

    QStringList result = db->query( queryString );

    db->query( "CREATE INDEX opmldirectory_tracks_id ON opmldirectory_tracks(id);" );
    db->query( "CREATE INDEX opmldirectory_tracks_album_id ON opmldirectory_tracks(album_id);" );

    // create table containing categories
    queryString = "CREATE TABLE opmldirectory_albums ("
                  "id INTEGER PRIMARY KEY AUTO_INCREMENT, "
                  "name " + db->textColumnType() + ',' +
                  "description " + db->exactTextColumnType() + ',' +
                  "artist_id INTEGER ) ENGINE = MyISAM;";

    result = db->query( queryString );
    db->query( "CREATE INDEX opmldirectory_albums_name ON opmldirectory_albums(name);" );


    //HACK!! monster hack actually! We really need a default dummy artist or the service query maker screws up big time-
    // we also need a dummy genre it would seem....

    queryString = "CREATE TABLE opmldirectory_artists ("
            "id INTEGER PRIMARY KEY AUTO_INCREMENT, "
            "name " + db->textColumnType() + ',' +
            "description " + db->exactTextColumnType() + ") ENGINE = MyISAM;";

    result = db->query( queryString );

    //now, insert a default artist
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
    queryString = "INSERT INTO opmldirectory_artists ( id, name, description "
            ") VALUES ( 1, 'dummy', 'dummy' );";

    sqlDb->insert( queryString, QString() );

    //create genre table
    queryString = "CREATE TABLE opmldirectory_genre ("
            "id INTEGER PRIMARY KEY AUTO_INCREMENT, "
            "name " + db->textColumnType() + ',' +
            "album_id INTEGER ) ENGINE = MyISAM;";

    result = db->query( queryString );
}

void
OpmlDirectoryDatabaseHandler::destroyDatabase()
{
    SqlStorage *db = CollectionManager::instance()->sqlStorage();
    QStringList result = db->query( "DROP TABLE opmldirectory_tracks;" );
    result = db->query( "DROP TABLE opmldirectory_albums;" );
    result = db->query( "DROP TABLE opmldirectory_artists;" );
    result = db->query( "DROP TABLE opmldirectory_genre;");

    result = db->query( "DROP INDEX opmldirectory_tracks_id;");
    result = db->query( "DROP INDEX opmldirectory_tracks_artist_id;");
    result = db->query( "DROP INDEX opmldirectory_album_name;");
}

int
OpmlDirectoryDatabaseHandler::insertTrack( ServiceTrackPtr track )
{
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
    QString queryString = "INSERT INTO opmldirectory_tracks ( name, track_number, length, "
                          "album_id, artist_id, preview_url ) VALUES ( '"
                          + sqlDb->escape( track->name() ) + "', "
                          + QString::number( 0 ) + ", "
                          + QString::number( 0 ) + ", "
                          + QString::number( track->albumId() ) + ", "
                          + QString::number( 1 ) + ", '"
                          + sqlDb->escape( track->uidUrl() ) + "' );";

    int trackId = sqlDb->insert( queryString, NULL );

    return trackId;
}

int
OpmlDirectoryDatabaseHandler::insertAlbum( ServiceAlbumPtr album )
{
    QString queryString;
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
    queryString = "INSERT INTO opmldirectory_albums ( name, description, "
                  "artist_id ) VALUES ( '"
                  + sqlDb->escape( album->name() ) + "', '"
                  + sqlDb->escape( album->description() ) + "', "
                  + QString::number( 1 ) + ");";

    int newAlbumId = sqlDb->insert( queryString, QString() );

    //create a dummy genre for this album
    queryString = "INSERT INTO opmldirectory_genre ( album_id, name "
                  ") VALUES ( " + QString::number ( newAlbumId ) + ", 'dummy');";

    return sqlDb->insert( queryString, 0 );
}

void
OpmlDirectoryDatabaseHandler::begin()
{
    CollectionManager *mgr = CollectionManager::instance();
    QString queryString = "BEGIN;";
    mgr->sqlStorage()->query( queryString );
}

void
OpmlDirectoryDatabaseHandler::commit()
{
    CollectionManager *mgr = CollectionManager::instance();
    QString queryString = "COMMIT;";
    mgr->sqlStorage()->query( queryString );
}

