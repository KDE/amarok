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

#include "debug.h"


JamendoDatabaseHandler *JamendoDatabaseHandler::m_pInstance = 0;

JamendoDatabaseHandler* 
JamendoDatabaseHandler::instance()
{
    if ( m_pInstance == 0 )
    {
        m_pInstance = new JamendoDatabaseHandler();
    }
    return m_pInstance;
}


JamendoDatabaseHandler::JamendoDatabaseHandler()
{}


JamendoDatabaseHandler::~JamendoDatabaseHandler()
{}

void 
JamendoDatabaseHandler::createDatabase( )
{
    //Get database instance
    CollectionDB *db = CollectionDB::instance();

    QString tagsAutoIncrement = "";

   /* if ( db->getDbConnectionType() == DbConnection::postgresql )
    {
        db->query( QString( "CREATE SEQUENCE jamendo_tags_seq;" ) );

        tagsAutoIncrement  = QString( "DEFAULT nextval('jamendo_tags_seq')" );

    }
    else if ( db->getDbConnectionType() == DbConnection::mysql )
    {
        tagsAutoIncrement = "AUTO_INCREMENT";
    }*/

    // create table containing tracks
    QString queryString = "CREATE TABLE jamendo_tracks ("
                          "id INTEGER PRIMARY KEY, "
                          "name " + db->textColumnType() + ',' +
                          "track_number INTEGER,"
                          "length INTEGER,"
                          "album_id INTEGER,"
                          "preview " + db->exactTextColumnType() + ");";

    debug() << "Creating mangnatune_tracks: " << queryString << endl;


    QStringList result = db->query( queryString );

    //Create album table
    queryString = "CREATE TABLE jamendo_albums ("
                  "id INTEGER PRIMARY KEY, "
                  "name " + db->textColumnType() + ',' +
                  "artist_id INTEGER,"
                  "genre " + db->textColumnType() + ',' +
                  "description " + db->exactTextColumnType() + ");";

    debug() << "Creating Mangnatune_albums: " << queryString << endl;

    result = db->query( queryString );

    //Create artist table
    queryString = "CREATE TABLE jamendo_artists ("
                  "id INTEGER PRIMARY KEY, "
                  "name " + db->textColumnType() + ',' +
                  "jamendo_page " + db->exactTextColumnType() + ',' +
                  "artist_page " + db->exactTextColumnType() + ',' +
                  "description " + db->textColumnType() + ");";

    debug() << "Creating mangnatune_artist: " << queryString << endl;

    result = db->query( queryString );

    //create moods table
     /*queryString = "CREATE TABLE jamendo_tags ("
                  "id INTEGER PRIMARY KEY " + moodsAutoIncrement + ',' +
                  "track_id INTEGER," +
                  "tag " + db->textColumnType() + ");";

    debug() << "Creating mangnatune_moods: " << queryString << endl;

    result = db->query( queryString );
*/


}

void 
JamendoDatabaseHandler::destroyDatabase( )
{
    CollectionDB *db = CollectionDB::instance();
    QStringList result = db->query( "DROP TABLE jamendo_tracks;" );
    result = db->query( "DROP TABLE jamendo_albums;" );
    result = db->query( "DROP TABLE jamendo_artists;" );
    //result = db->query( "DROP TABLE jamendo_tags;" );

    if ( db->getDbConnectionType() == DbConnection::postgresql )
    {
        db->query( QString( "DROP SEQUENCE jamendo_track_seq;" ) );
        db->query( QString( "DROP SEQUENCE jamendo_album_seq;" ) );
        db->query( QString( "DROP SEQUENCE jamendo_artist_seq;" ) );
        //db->query( QString( "DROP SEQUENCE jamendo_tags_seq;" ) );
    }
}

int 
JamendoDatabaseHandler::insertTrack( JamendoTrack *track )
{
    QString numberString;

    CollectionDB *db = CollectionDB::instance();
    QString queryString = "INSERT INTO jamendo_tracks ( id, name, track_number, length, "
                          "album_id, preview ) VALUES ( "
                          + QString::number( track->getId() ) + ", '"
                          + db->escapeString( track->getName() ) + "', "
                          + QString::number( track->getTrackNumber() ) + ", "
                          + QString::number( track->getDuration() ) + ", "
                          + QString::number( track->getAlbumId() ) + ", '"
                          + db->escapeString( track->getLofiURL() ) + "' );";


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
JamendoDatabaseHandler::insertAlbum( JamendoAlbum *album )
{
    QString queryString;
    CollectionDB *db = CollectionDB::instance();
    queryString = "INSERT INTO jamendo_albums ( id, name, artist_id, "
                  "genre, description ) VALUES ( "
                  + QString::number( album->getId() ) + ", '"
                  + db->escapeString( db->escapeString( album->getName() ) ) + "', "
                  + QString::number( album->getArtistId() ) + ", '"
                  + db->escapeString( album->getGenre() ) + "', '"
                  + db->escapeString( album->getDescription() )+ "' );";

    //debug() << "Adding Jamendo album " << queryString << endl;

    return db->insert( queryString, 0 );
}



int 
JamendoDatabaseHandler::insertArtist( JamendoArtist *artist )
{
    QString queryString;
    CollectionDB *db = CollectionDB::instance();
    queryString = "INSERT INTO jamendo_artists ( id, name, jamendo_page, artist_page, description "
                  ") VALUES ( "
                  + QString::number( artist->getId() ) + ", '"
                  + db->escapeString( db->escapeString( artist->getName() ) ) + "', '"
                  + db->escapeString( artist->getJamendoURL() ) + "', '"
                  + db->escapeString( artist->getHomeURL() ) + "', '"
                  + db->escapeString( artist->getDescription() ) + "' );";

    //debug() << "Adding Jamendo artist " << queryString << endl;

    return db->insert( queryString, 0 );
}


int 
JamendoDatabaseHandler::getArtistIdByExactName( const QString &name )
{
    CollectionDB *db = CollectionDB::instance();

    QString queryString = "SELECT id from jamendo_artists WHERE name='" + db->escapeString( name ) + "';";
    QStringList result = db->query( queryString );

    //debug() << "Looking for id of artist " << name << ":" << endl;

    if ( result.size() < 1 ) return -1;
    int artistId = result.first().toInt();
    
    //debug() << "    Found: " << QString::number( artistId ) << ":" << endl;
    
    return artistId;

}


JamendoArtistList 
JamendoDatabaseHandler::getArtistsByGenre( const QString &genre )
{

    QString genreSql = "";

    if ( !( genre == "All" ||  genre.isEmpty() ) )
    {
        genreSql = "jamendo_albums.genre='" + genre + "' AND ";
    }

    CollectionDB *db = CollectionDB::instance();

    QString queryString;
    queryString = "SELECT DISTINCT jamendo_artists.id, "
                  "jamendo_artists.name, jamendo_artists.jamendo_page , "
                  "jamendo_artists.artist_page, jamendo_artists.description "
                  "FROM jamendo_albums, jamendo_artists "
                  "WHERE " + genreSql + "jamendo_albums.artist_id "
                  "= jamendo_artists.id;";

    QStringList result = db->query( queryString );

    debug() << "Looking for artist in genre: " <<  genre << endl;

    JamendoArtistList list;

    while ( result.size() > 0 )
    {
        JamendoArtist artist;

        artist.setId( result.front().toInt() );
        result.pop_front();

        artist.setName( result.front() );
        result.pop_front();

        artist.setJamendoURL( result.front() );
        result.pop_front();

        artist.setHomeURL( result.front() );
        result.pop_front();

        artist.setDescription( result.front() );
        result.pop_front();

        list.append( artist );
    }

    return list;


}

JamendoAlbumList 
JamendoDatabaseHandler::getAlbumsByArtistId( int id, const QString &genre )
{

    QString genreSqlString;

    if ( genre.isEmpty() ||  genre == "All" )
    {
        genreSqlString = "";
    }
    else
    {
        genreSqlString = " AND jamendo_albums.genre='" + genre + '\'';
    }


    CollectionDB *db = CollectionDB::instance();

    QString queryString;
    queryString = "SELECT DISTINCT id, "
                  "name, "
                  "artist_id, genre, "
                  "description "
                  "FROM jamendo_albums "
                  "WHERE artist_id = '" + QString::number( id ) + '\'';

    queryString += genreSqlString;
    queryString += ';';


    QStringList result = db->query( queryString );


    JamendoAlbumList list;
    debug() << "Looking for Albums..." << endl;
    debug() << "Query string:" << queryString << endl;


    while ( result.size() > 0 )
    {
        JamendoAlbum album;

        album.setId( result.front().toInt() );
        result.pop_front();

        album.setName( result.front() );
        result.pop_front();

        album.setArtistId( result.front().toInt() );
        result.pop_front();

        album.setGenre( result.front() );
        result.pop_front();

        album.setDescription( result.front() );
        result.pop_front();

        list.append( album );
    }

    return list;


}


JamendoTrackList 
JamendoDatabaseHandler::getTracksByAlbumId( int id )
{

    CollectionDB *db = CollectionDB::instance();

    QString queryString;
    queryString = "SELECT DISTINCT id, "
                  "name, track_number, "
                  "length, album_id, "
                  "preview "
                  "FROM jamendo_tracks "
                  "WHERE album_id = '" + QString::number( id ) + "';";

    QStringList result = db->query( queryString );


    JamendoTrackList list;

    debug() << "Looking for tracks..." << endl;
    debug() << "Query string:" << queryString << endl;


    while ( result.size() > 0 )
    {

        debug() << "track start" << endl;
        JamendoTrack track;

        track.setId( result.front().toInt() );
        result.pop_front();

        track.setName( result.front() );
        result.pop_front();

        track.setTrackNumber( result.front().toInt() );
        result.pop_front();

        track.setDuration( result.front().toInt() );
        result.pop_front();

        track.setAlbumId( result.front().toInt() );
        result.pop_front();

        track.setLofiURL( result.front() );
        result.pop_front();

        list.append( track );
        debug() << "track end" << endl;
    }

    return list;

}

QStringList 
JamendoDatabaseHandler::getAlbumGenres( )
{

    CollectionDB *db = CollectionDB::instance();

    QString queryString;
    queryString = "SELECT DISTINCT genre "
                  "FROM jamendo_albums "
                  "ORDER BY genre;";

    return db->query( queryString );
}

void 
JamendoDatabaseHandler::begin( )
{

    CollectionDB *db = CollectionDB::instance();

    QString queryString = "BEGIN;";

    db->query( queryString );
}

void 
JamendoDatabaseHandler::commit( )
{
    CollectionDB *db = CollectionDB::instance();
    QString queryString = "COMMIT;";

    db->query( queryString );

}

JamendoArtist 
JamendoDatabaseHandler::getArtistById( int id )
{

    CollectionDB *db = CollectionDB::instance();

    QString queryString;
    queryString = "SELECT id, "
                  "name, jamendo_page, "
                  "artist_page, description, "
                  "FROM jamendo_artists "
                  "WHERE id = '" + QString::number( id ) + "';";

    QStringList result = db->query( queryString );

    JamendoArtist artist;

    if ( result.size() == 5 )
    {

        artist.setId( result.front().toInt() );
        result.pop_front();

        artist.setName( result.front() );
        result.pop_front();

        artist.setJamendoURL( result.front() );
        result.pop_front();

        artist.setHomeURL( result.front() );
        result.pop_front();

        artist.setDescription( result.front() );
        result.pop_front();

    }


    return artist;
}

JamendoAlbum 
JamendoDatabaseHandler::getAlbumById( int id )
{


    CollectionDB *db = CollectionDB::instance();

    QString queryString;
    queryString = "SELECT id, "
                  "name, "
                  "artist_id, genre, "
                  "description "
                  "FROM jamendo_albums "
                  "WHERE id = '" + QString::number( id ) + "';";

    QStringList result = db->query( queryString );

    JamendoAlbum album;

    if ( result.size() == 5 )
    {

        album.setId( result.front().toInt() );
        result.pop_front();

        album.setName( result.front() );
        result.pop_front();

        album.setArtistId( result.front().toInt() );
        result.pop_front();

        album.setGenre( result.front() );
        result.pop_front();

        album.setDescription( result.front() );
        result.pop_front();
    }

    return album;

}

JamendoTrack JamendoDatabaseHandler::getTrackById(int id) {


    CollectionDB *db = CollectionDB::instance();

    QString queryString;
    queryString = "SELECT DISTINCT id, "
                  "name, track_number, "
                  "length, album_id, "
                  "preview "
                  "FROM jamendo_tracks "
                  "WHERE id = '" + QString::number( id ) + "';";

    QStringList result = db->query( queryString );

    JamendoTrack track;
    if ( result.size() == 6 )
    {

        track.setId( result.front().toInt() );
        result.pop_front();

        track.setName( result.front() );
        result.pop_front();

        track.setTrackNumber( result.front().toInt() );
        result.pop_front();

        track.setDuration( result.front().toInt() );
        result.pop_front();

        track.setAlbumId( result.front().toInt() );
        result.pop_front();

        track.setLofiURL( result.front() );
        result.pop_front();

    }

    return track;
}











