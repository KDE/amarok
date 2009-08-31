/****************************************************************************************
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>               *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MagnatuneDatabaseHandler.h"

#include "collection/CollectionManager.h"
#include "Debug.h"
#include "collection/SqlStorage.h"

using namespace Meta;

MagnatuneDatabaseHandler::MagnatuneDatabaseHandler()
{}


MagnatuneDatabaseHandler::~MagnatuneDatabaseHandler()
{}

void
MagnatuneDatabaseHandler::createDatabase( )
{
    //Get database instance
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();

    QString autoIncrement = "AUTO_INCREMENT";

    // create table containing tracks
    QString queryString = "CREATE TABLE magnatune_tracks ("
                          "id INTEGER PRIMARY KEY " + autoIncrement + ',' +
                          "name " + sqlDb->textColumnType() + ',' +
                          "track_number INTEGER,"
                          "length INTEGER,"
                          "album_id INTEGER,"
                          "artist_id INTEGER,"
                          "preview_lofi " + sqlDb->exactTextColumnType() + ',' +
                          "preview_ogg " + sqlDb->exactTextColumnType() + ',' +
                          "preview_url " + sqlDb->exactTextColumnType() + ") ENGINE = MyISAM;";

    debug() << "Creating mangnatune_tracks: " << queryString;


    QStringList result = sqlDb->query( queryString );

    sqlDb->query( "CREATE INDEX magnatune_tracks_album_id ON magnatune_tracks(album_id);" );
    sqlDb->query( "CREATE INDEX magnatune_tracks_artist_id ON magnatune_tracks(artist_id);" );

    //Create album table
    queryString = "CREATE TABLE magnatune_albums ("
                  "id INTEGER PRIMARY KEY " + autoIncrement + ',' +
                  "name " + sqlDb->textColumnType() + ',' +
                  "year INTEGER,"
                  "artist_id INTEGER,"
                  "album_code " + sqlDb->textColumnType() + ',' +
                  "cover_url " + sqlDb->exactTextColumnType() + ',' +
                  "description " + sqlDb->exactTextColumnType() + ") ENGINE = MyISAM;";

    debug() << "Creating Mangnatune_albums: " << queryString;

    result = sqlDb->query( queryString );

    sqlDb->query( "CREATE INDEX magnatune_albums_name ON magnatune_albums(name);" );
    sqlDb->query( "CREATE INDEX magnatune_albums_artist_id ON magnatune_albums(artist_id);" );


    //Create artist table
    queryString = "CREATE TABLE magnatune_artists ("
                  "id INTEGER PRIMARY KEY " + autoIncrement + ',' +
                  "name " + sqlDb->textColumnType() + ',' +
                  "artist_page " + sqlDb->exactTextColumnType() + ',' +
                  "description " + sqlDb->textColumnType() + ',' +
                  "photo_url " + sqlDb->exactTextColumnType() + ") ENGINE = MyISAM;";

    debug() << "Creating mangnatune_artist: " << queryString;

    result = sqlDb->query( queryString );

    sqlDb->query( "CREATE INDEX magnatune_artists_name ON magnatune_artists(name);" );

    //create genre table
    queryString = "CREATE TABLE magnatune_genre ("
                  "id INTEGER PRIMARY KEY " + autoIncrement + ',' +
                  "name " + sqlDb->textColumnType() + ',' +
                  "album_id INTEGER" + ") ENGINE = MyISAM;";

    result = sqlDb->query( queryString );

    sqlDb->query( "CREATE INDEX magnatune_genre_name ON magnatune_genre(name);" );
    sqlDb->query( "CREATE INDEX magnatune_genre_album_id ON magnatune_genre(album_id);" );


    //create moods table
     queryString = "CREATE TABLE magnatune_moods ("
                  "id INTEGER PRIMARY KEY " + autoIncrement + ',' +
                  "track_id INTEGER," +
                  "mood " + sqlDb->textColumnType() + ") ENGINE = MyISAM;";

    debug() << "Creating mangnatune_moods: " << queryString;

    result = sqlDb->query( queryString );



}

void
MagnatuneDatabaseHandler::destroyDatabase( )
{
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
    QStringList result = sqlDb->query( "DROP TABLE magnatune_tracks;" );
    result = sqlDb->query( "DROP TABLE magnatune_albums;" );
    result = sqlDb->query( "DROP TABLE magnatune_artists;" );
    result = sqlDb->query( "DROP TABLE magnatune_genre;" );
    result = sqlDb->query( "DROP TABLE magnatune_moods;" );


    result = sqlDb->query( "DROP INDEX magnatune_tracks_artist_id;");
    result = sqlDb->query( "DROP INDEX magnatune_tracks_album_id;");
    result = sqlDb->query( "DROP INDEX magnatune_album_name;");
    result = sqlDb->query( "DROP INDEX magnatune_album_artist_id;");
    result = sqlDb->query( "DROP INDEX magnatune_artist_name;");
    result = sqlDb->query( "DROP INDEX magnatune_genre_album_id;");
    result = sqlDb->query( "DROP INDEX magnatune_genre_name;");

   /* if ( sqlDb->type() == DbConnection::postgresql )
    {
        sqlDb->query( QString( "DROP SEQUENCE magnatune_track_seq;" ) );
        sqlDb->query( QString( "DROP SEQUENCE magnatune_album_seq;" ) );
        sqlDb->query( QString( "DROP SEQUENCE magnatune_artist_seq;" ) );
        sqlDb->query( QString( "DROP SEQUENCE magnatune_moods_seq;" ) );
    }*/
}

int
MagnatuneDatabaseHandler::insertTrack( ServiceTrack *track )
{
    MagnatuneTrack * mTrack = static_cast<MagnatuneTrack *> ( track );

    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
    QString queryString = "INSERT INTO magnatune_tracks ( name, track_number, length, "
            "album_id, artist_id, preview_lofi, preview_ogg, preview_url ) VALUES ( '"
                          + sqlDb->escape( mTrack->name()) + "', "
                          + QString::number( mTrack->trackNumber() ) + ", "
                          + QString::number( mTrack->length() ) + ", "
                          + QString::number( mTrack->albumId() ) + ", "
                          + QString::number( mTrack->artistId() ) + ", '"
                          + sqlDb->escape( mTrack->lofiUrl() ) + "', '"
                          + sqlDb->escape( mTrack->oggUrl() ) + "', '"
                          + sqlDb->escape( mTrack->uidUrl() ) + "' );";


    // debug() << "Adding Magnatune track " << queryString;
    int trackId = sqlDb->insert( queryString, NULL );

    return trackId;


}

int
MagnatuneDatabaseHandler::insertAlbum( ServiceAlbum *album )
{

    MagnatuneAlbum * mAlbum = static_cast<MagnatuneAlbum *> ( album );

    QString queryString;
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
    queryString = "INSERT INTO magnatune_albums ( name, year, artist_id, "
                  "album_code, cover_url, description ) VALUES ( '"
                  + sqlDb->escape( sqlDb->escape( mAlbum->name() ) ) + "', "
                  + QString::number( mAlbum->launchYear() ) + ", "
                  + QString::number( mAlbum->artistId() ) + ", '"
                  + sqlDb->escape( mAlbum->albumCode() ) + "', '"
                  + sqlDb->escape( mAlbum->coverUrl() ) + "', '"
                  + sqlDb->escape( mAlbum->description() )+ "' );";

    //debug() << "Adding Magnatune album " << queryString;

    return sqlDb->insert( queryString, 0 );
}



int
MagnatuneDatabaseHandler::insertArtist( ServiceArtist *artist )
{
    MagnatuneArtist * mArtist = static_cast<MagnatuneArtist *> ( artist );

    QString queryString;
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
    queryString = "INSERT INTO magnatune_artists ( name, artist_page, description, "
                  "photo_url ) VALUES ( '"
                  + sqlDb->escape( sqlDb->escape( mArtist->name() ) ) + "', '"
                  + sqlDb->escape( mArtist->magnatuneUrl()) + "', '"
                  + sqlDb->escape( mArtist->description() ) + "', '"
                  + sqlDb->escape( mArtist->photoUrl() ) + "' );";

    //debug() << "Adding Magnatune artist " << queryString;

    return sqlDb->insert( queryString, 0 );
}


void
MagnatuneDatabaseHandler::begin( )
{

    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();

    QString queryString = "BEGIN;";

    sqlDb->query( queryString );
}

void
MagnatuneDatabaseHandler::commit( )
{
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
    QString queryString = "COMMIT;";

    sqlDb->query( queryString );

}

void MagnatuneDatabaseHandler::insertMoods(int trackId, const QStringList &moods)
{

    QString queryString;
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();

    foreach( const QString &mood, moods ) {
        queryString = "INSERT INTO magnatune_moods ( track_id, mood ) VALUES ( "
                      + QString::number( trackId ) + ", '"
                      + sqlDb->escape( mood ) +  "' );";


        //debug() << "Adding Magnatune mood: " << queryString;
        sqlDb->insert( queryString, NULL );
    }
}

int MagnatuneDatabaseHandler::getArtistIdByExactName(const QString & name)
{

    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();

    QString queryString = "SELECT id from magnatune_artists WHERE name='" + sqlDb->escape( name ) + "';";
    QStringList result = sqlDb->query( queryString );

    //debug() << "Looking for id of artist " << name << ":";

    if ( result.size() < 1 ) return -1;
    int artistId = result.first().toInt();

    //debug() << "    Found: " << QString::number( artistId ) << ":";

    return artistId;

}

int MagnatuneDatabaseHandler::getAlbumIdByAlbumCode(const QString & albumcode)
{
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();

    QString queryString = "SELECT id from magnatune_albums WHERE album_code='" + sqlDb->escape( albumcode ) + "';";
    QStringList result = sqlDb->query( queryString );

    //debug() << "Looking for id of album " << albumcode << ":";

    if ( result.size() < 1 ) return -1;
    int albumId = result.first().toInt();

    //debug() << "  Found: " << QString::number( albumId ) << ":";

    return albumId;
}

int MagnatuneDatabaseHandler::insertGenre(ServiceGenre * genre)
{
    QString queryString;
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
    queryString = "INSERT INTO magnatune_genre ( album_id, name "
                  ") VALUES ( "
                  + QString::number ( genre->albumId() ) + ", '"
                  + sqlDb->escape( genre->name() ) + "' );";

    //debug() << "Adding Jamendo genre " << queryString;

    return sqlDb->insert( queryString, 0 );
}






