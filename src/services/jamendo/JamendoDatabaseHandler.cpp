/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "JamendoDatabaseHandler.h"

#include "collection/CollectionManager.h"
#include "core/support/Debug.h"
#include "collection/SqlStorage.h"

using namespace Meta;

JamendoDatabaseHandler::JamendoDatabaseHandler()
{
}

JamendoDatabaseHandler::~JamendoDatabaseHandler()
{
}

void
JamendoDatabaseHandler::createDatabase( )
{
    //Get database instance
    SqlStorage *db = CollectionManager::instance()->sqlStorage();


    QString autoIncrement = "AUTO_INCREMENT";

    // create table containing tracks
    QString queryString = "CREATE TABLE jamendo_tracks ("
            "id INTEGER PRIMARY KEY " + autoIncrement + ',' +
                          "name " + db->textColumnType() + ',' +
                          "track_number INTEGER,"
                          "length INTEGER,"
                          "preview_url " + db->exactTextColumnType() + ',' +
                          "album_id INTEGER,"
                          "artist_id INTEGER ) ENGINE = MyISAM;";

    debug() << "Creating jamendo_tracks: " << queryString;

    QStringList result = db->query( queryString );
    db->query( "CREATE INDEX jamendo_tracks_id ON jamendo_tracks(id);" );
    db->query( "CREATE INDEX jamendo_tracks_album_id ON jamendo_tracks(album_id);" );
    db->query( "CREATE INDEX jamendo_tracks_artist_id ON jamendo_tracks(artist_id);" );

    //Create album table
    queryString = "CREATE TABLE jamendo_albums ("
                  "id INTEGER PRIMARY KEY " + autoIncrement + ',' +
                  "name " + db->textColumnType() + ',' +
                  "description " + db->exactTextColumnType() + ',' +
                  "popularity FLOAT, " +
                  "cover_url " + db->exactTextColumnType() + ',' +
                  "launch_year Integer, "
                  "genre " + db->exactTextColumnType() + ',' +
                  "artist_id INTEGER, "
                  "mp3_torrent_url " + db->exactTextColumnType() + ',' +
                  "ogg_torrent_url " + db->exactTextColumnType() + " ) ENGINE = MyISAM;";

    debug() << "Creating jamendo_albums: " << queryString;

    result = db->query( queryString );

    db->query( "CREATE INDEX jamendo_albums_id ON jamendo_albums(id);" );
    db->query( "CREATE INDEX jamendo_albums_name ON jamendo_albums(name);" );
    db->query( "CREATE INDEX jamendo_albums_artist_id ON jamendo_albums(artist_id);" );

    //Create artist table
    queryString = "CREATE TABLE jamendo_artists ("
                  "id INTEGER PRIMARY KEY " + autoIncrement + ',' +
                  "name " + db->textColumnType() + ',' +
                  "description " + db->textColumnType() + ',' +
                  "country " + db->textColumnType() + ',' +
                  "photo_url " + db->textColumnType() + ',' +
                  "jamendo_url " + db->textColumnType() + ',' +
                  "home_url " + db->textColumnType() + ") ENGINE = MyISAM;";

    debug() << "Creating jamendo_artists: " << queryString;

    result = db->query( queryString );

    db->query( "CREATE INDEX jamendo_artists_id ON jamendo_artists(id);" );
    db->query( "CREATE INDEX jamendo_artists_name ON jamendo_artists(name);" );

    //create genre table
    queryString = "CREATE TABLE jamendo_genre ("
                  "id INTEGER PRIMARY KEY " + autoIncrement + ',' +
                  "name " + db->textColumnType() + ',' +
                  "album_id INTEGER" + ") ENGINE = MyISAM;";

    debug() << "Creating jamendo_genres: " << queryString;

    result = db->query( queryString );

    db->query( "CREATE INDEX jamendo_genre_id ON jamendo_genre(id);" );
    db->query( "CREATE INDEX jamendo_genre_name ON jamendo_genre(name);" );
    db->query( "CREATE INDEX jamendo_genre_album_id ON jamendo_genre(album_id);" );
}

void
JamendoDatabaseHandler::destroyDatabase( )
{
    debug() << "Destroy Jamendo database ";

    SqlStorage *db = CollectionManager::instance()->sqlStorage();

    QStringList  result = db->query( "DROP INDEX jamendo_tracks_id ON jamendo_tracks;");
    result = db->query( "DROP INDEX jamendo_tracks_artist_id ON jamendo_tracks;");
    result = db->query( "DROP INDEX jamendo_tracks_album_id ON jamendo_tracks;");
    result = db->query( "DROP INDEX jamendo_albums_id ON jamendo_albums;");
    result = db->query( "DROP INDEX jamendo_albums_name ON jamendo_albums;");
    result = db->query( "DROP INDEX jamendo_albums_artist_id ON jamendo_albums;");
    result = db->query( "DROP INDEX jamendo_artists_id ON jamendo_artists;");
    result = db->query( "DROP INDEX jamendo_artists_name ON jamendo_artists;");
    result = db->query( "DROP INDEX jamendo_genre_id ON jamendo_genre;");
    result = db->query( "DROP INDEX jamendo_genre_album_id ON jamendo_genre;");
    result = db->query( "DROP INDEX jamendo_genre_name ON jamendo_genre;");


    result = db->query( "DROP TABLE jamendo_tracks;" );
    result = db->query( "DROP TABLE jamendo_albums;" );
    result = db->query( "DROP TABLE jamendo_artists;" );
    result = db->query( "DROP TABLE jamendo_genre;" );

    //FIXME: We only support sqlite currently.  DbConnection no longer exists.
}

int
JamendoDatabaseHandler::insertTrack( ServiceTrack *track )
{
    JamendoTrack * jTrack = static_cast<JamendoTrack *> ( track );
    QString numberString;

    SqlStorage *db = CollectionManager::instance()->sqlStorage();
    QString queryString = "INSERT INTO jamendo_tracks ( id, name, track_number, length, "
                          "album_id, artist_id, preview_url ) VALUES ( "
                          + QString::number( jTrack->id() ) + ", '"
                          + db->escape( jTrack->name() ) + "', "
                          + QString::number( jTrack->trackNumber() ) + ", "
                          + QString::number( jTrack->length() ) + ", "
                          + QString::number( jTrack->albumId() ) + ", "
                          + QString::number( jTrack->artistId() ) + ", '"
                          + db->escape( jTrack->uidUrl() ) + "' );";

    // debug() << "Adding Jamendo track " << queryString;
    int trackId = db->insert( queryString, NULL );

    // Process moods:

   /* QStringList moods = track->getMoods();

    foreach( QString mood, moods ) {
        queryString = "INSERT INTO jamendo_moods ( track_id, mood ) VALUES ( "
                      + QString::number( trackId ) + ", '"
                      + db->escape( mood ) +  "' );";


        //debug() << "Adding Jamendo mood: " << queryString;
        db->insert( queryString, NULL );
    }
*/
    return trackId;
}

int
JamendoDatabaseHandler::insertAlbum( ServiceAlbum *album )
{
    JamendoAlbum * jAlbum = static_cast<JamendoAlbum *> ( album );

    QString queryString;
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
    queryString = "INSERT INTO jamendo_albums ( id, name, description, "
                  "popularity, cover_url, launch_year, genre, "
                  "artist_id, mp3_torrent_url, ogg_torrent_url ) VALUES ( "
                  + QString::number( jAlbum->id() ) + ", '"
                  + sqlDb->escape(  jAlbum->name() ) + "', '"
                  + sqlDb->escape( jAlbum->description() )+ "', "
                  + QString::number( jAlbum->popularity() ) + ", '"
                  + sqlDb->escape( jAlbum->coverUrl() )+ "', "
                  + QString::number( jAlbum->launchYear() ) + ", '"
                  + sqlDb->escape( jAlbum->genre() )+ "', "
                  + QString::number( jAlbum->artistId() ) + ", '"
                  + sqlDb->escape( jAlbum->mp3TorrentUrl() ) + "', '"
                  + sqlDb->escape( jAlbum->oggTorrentUrl() ) + "' );";

    //debug() << "Adding Jamendo album " << queryString;

    return sqlDb->insert( queryString, QString() );
}


int
JamendoDatabaseHandler::insertArtist( ServiceArtist *artist )
{
    JamendoArtist * jArtist = static_cast<JamendoArtist *> ( artist );
    QString queryString;
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
    queryString = "INSERT INTO jamendo_artists ( id, name, description, "
                  "country, photo_url, jamendo_url, home_url "
                  ") VALUES ( "
                  + QString::number( jArtist->id() ) + ", '"
                  + sqlDb->escape( jArtist->name() ) + "', '"
                  + sqlDb->escape( jArtist->description() ) + "', '"
                  + sqlDb->escape( jArtist->country() ) + "', '"
                  + sqlDb->escape( jArtist->photoURL() ) + "', '"
                  + sqlDb->escape( jArtist->jamendoURL() ) + "', '"
                  + sqlDb->escape( jArtist->homeURL() ) + "' );";

    //debug() << "Adding Jamendo artist " << queryString;

    return sqlDb->insert( queryString, QString() );
/*
    QString m_country;
    QString m_photoURL;
    QString m_jamendoURL;
    QString m_homeURL;*/
}

int JamendoDatabaseHandler::insertGenre(ServiceGenre * genre)
{
    QString queryString;
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
    queryString = "INSERT INTO jamendo_genre ( album_id, name "
                  ") VALUES ( "
                  + QString::number ( genre->albumId() ) + ", '"
                  + sqlDb->escape( genre->name() ) + "' );";

    //debug() << "Adding Jamendo genre " << queryString;

    return sqlDb->insert( queryString, 0 );
}

void
JamendoDatabaseHandler::begin( )
{
    CollectionManager *mgr = CollectionManager::instance();
    QString queryString = "BEGIN;";
    mgr->sqlStorage()->query( queryString );
}

void
JamendoDatabaseHandler::commit( )
{
    CollectionManager *mgr = CollectionManager::instance();
    QString queryString = "COMMIT;";
    mgr->sqlStorage()->query( queryString );
}

void
JamendoDatabaseHandler::trimGenres( int minCount )
{
    QString query = QString("delete from jamendo_genre where name IN ( SELECT name from jamendo_genre GROUP BY jamendo_genre.name HAVING COUNT ( jamendo_genre.name ) < %1 );").arg( minCount );

    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
    sqlDb->query( query );

    //also trim genre names that have only 1 or 2 chars
    query = QString ("delete from jamendo_genre where name REGEXP '^.{1,2}$';" );
    sqlDb->query( query );
    
}

