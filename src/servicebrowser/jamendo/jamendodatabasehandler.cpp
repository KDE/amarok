/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "jamendodatabasehandler.h"

#include "collectionmanager.h"
#include "debug.h"


JamendoDatabaseHandler::JamendoDatabaseHandler()
{}


JamendoDatabaseHandler::~JamendoDatabaseHandler()
{}

void 
JamendoDatabaseHandler::createDatabase( )
{
    //Get database instance
    CollectionDB *db = CollectionDB::instance();

    QString genreAutoIncrement = "";

    if ( db->getDbConnectionType() == DbConnection::postgresql )
    {
        db->query( QString( "CREATE SEQUENCE jamendo_genre_seq;" ) );

        genreAutoIncrement  = QString( "DEFAULT nextval('jamendo_genre_seq')" );

    }
    else if ( db->getDbConnectionType() == DbConnection::mysql )
    {
        genreAutoIncrement = "AUTO_INCREMENT";
    }

    // create table containing tracks
    QString queryString = "CREATE TABLE jamendo_tracks ("
                          "id INTEGER PRIMARY KEY, "
                          "name " + db->textColumnType() + ',' +
                          "track_number INTEGER,"
                          "length INTEGER,"
                          "preview_url " + db->exactTextColumnType() + ',' +
                          "album_id INTEGER,"
                          "album_name " + db->textColumnType() + ',' +
                          "artist_id INTEGER,"
                          "artist_name " + db->textColumnType() + ");";

    debug() << "Creating jamendo_tracks: " << queryString << endl;


    QStringList result = db->query( queryString );

    //Create album table
    queryString = "CREATE TABLE jamendo_albums ("
                  "id INTEGER PRIMARY KEY, "
                  "name " + db->textColumnType() + ',' +
                  "description " + db->exactTextColumnType() + ',' +
                  "artist_id INTEGER,"
                  "artist_name " + db->textColumnType() +  ");";


    debug() << "Creating jamendo_albums: " << queryString << endl;

    result = db->query( queryString );

    //Create artist table
    queryString = "CREATE TABLE jamendo_artists ("
                  "id INTEGER PRIMARY KEY, "
                  "name " + db->textColumnType() + ',' +
                  "description " + db->textColumnType() + ");";

    debug() << "Creating jamendo_artists: " << queryString << endl;

    result = db->query( queryString );

    //create genre table
    queryString = "CREATE TABLE jamendo_genre ("
                  "id INTEGER PRIMARY KEY " + genreAutoIncrement + ',' +
                  "name " + db->textColumnType() + ',' +
                  "album_id INTEGER" + ");";

    debug() << "Creating jamendo_genres: " << queryString << endl;

    result = db->query( queryString );

    //create a few indexes ( its all about the SPEEED baby! )



    queryString = "CREATE INDEX jamendo_album_album_id on jamendo_albums (id);";
    result = db->query( queryString );

    queryString = "CREATE INDEX jamendo_album_artist_id on jamendo_albums (artist_id);";
    result = db->query( queryString );

    queryString = "CREATE INDEX jamendo_artist_artist_id on jamendo_artists (id);";
    result = db->query( queryString );

    queryString = "CREATE INDEX jamendo_genre_album_id on jamendo_genre (album_id);";
    result = db->query( queryString );

    queryString = "CREATE INDEX jamendo_genre_name on jamendo_genre (name);";
    result = db->query( queryString );

}

void 
JamendoDatabaseHandler::destroyDatabase( )
{

    debug() << "Destroy Jamendo database " << endl;

    CollectionDB *db = CollectionDB::instance();
    QStringList result = db->query( "DROP TABLE jamendo_tracks;" );
    result = db->query( "DROP TABLE jamendo_albums;" );
    result = db->query( "DROP TABLE jamendo_artists;" );
    result = db->query( "DROP TABLE jamendo_genre;" );


    result = db->query( "DROP INDEX jamendo_album_album_id;");
    result = db->query( "DROP INDEX jamendo_album_artist_id;");
    result = db->query( "DROP INDEX jamendo_artist_artist_id;");
    result = db->query( "DROP INDEX jamendo_genre_album_id;");
    result = db->query( "DROP INDEX jamenod_genre_name;");

    if ( db->getDbConnectionType() == DbConnection::postgresql )
    {
        db->query( QString( "DROP SEQUENCE jamendo_track_seq;" ) );
        db->query( QString( "DROP SEQUENCE jamendo_album_seq;" ) );
        db->query( QString( "DROP SEQUENCE jamendo_artist_seq;" ) );
        //db->query( QString( "DROP SEQUENCE jamendo_tags_seq;" ) );
    }
}

int 
JamendoDatabaseHandler::insertTrack( ServiceTrack *track )
{

    JamendoTrack * jTrack = dynamic_cast<JamendoTrack *> ( track );

    QString numberString;

    CollectionDB *db = CollectionDB::instance();
    QString queryString = "INSERT INTO jamendo_tracks ( id, name, track_number, length, "
                          "album_id, album_name, artist_id, artist_name, preview_url ) VALUES ( "
                          + QString::number( jTrack->id() ) + ", '"
                          + db->escapeString( jTrack->name() ) + "', "
                          + QString::number( jTrack->trackNumber() ) + ", "
                          + QString::number( jTrack->length() ) + ", "
                          + QString::number( jTrack->albumId() ) + ", '"
                          + db->escapeString( jTrack->albumName() ) + "', "
                          + QString::number( jTrack->artistId() ) + ", '"
                          + db->escapeString( jTrack->artistName() ) + "', '"
                          + db->escapeString( jTrack->url() ) + "' );";


    // debug() << "Adding Jamendo track " << queryString << endl;
    int trackId = db->insert( queryString, NULL );

    // Process moods:

   /* QStringList moods = track->getMoods();

    foreach( QString mood, moods ) {
        queryString = "INSERT INTO jamendo_moods ( track_id, mood ) VALUES ( "
                      + QString::number( trackId ) + ", '"
                      + db->escapeString( mood ) +  "' );";


        //debug() << "Adding Jamendo mood: " << queryString << endl;
        db->insert( queryString, NULL ); 
    }
*/
    return trackId;
}

int 
JamendoDatabaseHandler::insertAlbum( ServiceAlbum *album )
{

    JamendoAlbum * jAlbum = dynamic_cast<JamendoAlbum *> ( album );

    QString queryString;
    CollectionDB *db = CollectionDB::instance();
    queryString = "INSERT INTO jamendo_albums ( id, name, description, "
                  " artist_id, artist_name ) VALUES ( "
                  + QString::number( jAlbum->id() ) + ", '"
                  + db->escapeString(  jAlbum->name() ) + "', '"
                  + db->escapeString( jAlbum->description() )+ "', "
                  + QString::number( jAlbum->artistId() ) + ", '"
                  + db->escapeString( jAlbum->artistName() ) + "' );";

    //debug() << "Adding Jamendo album " << queryString << endl;

    return db->insert( queryString, 0 );
}



int 
JamendoDatabaseHandler::insertArtist( ServiceArtist *artist )
{
    JamendoArtist * jArtist = dynamic_cast<JamendoArtist *> ( artist );

    QString queryString;
    CollectionDB *db = CollectionDB::instance();
    queryString = "INSERT INTO jamendo_artists ( id, name, description "
                  ") VALUES ( "
                  + QString::number( jArtist->id() ) + ", '"
                  + db->escapeString( jArtist->name() ) + "', '"
                  + db->escapeString( jArtist->description() ) + "' );";

    //debug() << "Adding Jamendo artist " << queryString << endl;

    return db->insert( queryString, 0 );
}

int JamendoDatabaseHandler::insertGenre(ServiceGenre * genre)
{
    QString queryString;
    CollectionDB *db = CollectionDB::instance();
    queryString = "INSERT INTO jamendo_genre ( album_id, name "
                  ") VALUES ( "
                  + QString::number ( genre->albumId() ) + ", '"
                  + db->escapeString( genre->name() ) + "' );";

    //debug() << "Adding Jamendo genre " << queryString << endl;

    return db->insert( queryString, 0 );
}



void 
JamendoDatabaseHandler::begin( )
{
    CollectionManager *mgr = CollectionManager::instance();
    QString queryString = "BEGIN;";
    mgr->sqlQuery(  queryString );
}

void 
JamendoDatabaseHandler::commit( )
{
    CollectionManager *mgr = CollectionManager::instance();
    QString queryString = "COMMIT;";
    mgr->sqlQuery( queryString );
}










