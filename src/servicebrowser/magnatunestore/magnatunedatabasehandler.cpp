/***************************************************************************
 *   Copyright (c) 2006, 2007                                              *
 *        Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/ 

#include "magnatunedatabasehandler.h"

#include "debug.h"


MagnatuneDatabaseHandler::MagnatuneDatabaseHandler()
{}


MagnatuneDatabaseHandler::~MagnatuneDatabaseHandler()
{}

void 
MagnatuneDatabaseHandler::createDatabase( )
{
    //Get database instance
    CollectionDB *db = CollectionDB::instance();

    QString tracksAutoIncrement = "";
    QString albumsAutoIncrement = "";
    QString artistAutoIncrement = "";
    QString moodsAutoIncrement = "";

    if ( db->getDbConnectionType() == DbConnection::postgresql )
    {
        db->query( QString( "CREATE SEQUENCE magnatune_track_seq;" ) );
        db->query( QString( "CREATE SEQUENCE magnatune_album_seq;" ) );
        db->query( QString( "CREATE SEQUENCE magnatune_artist_seq;" ) );
        db->query( QString( "CREATE SEQUENCE magnatune_moods_seq;" ) );

        tracksAutoIncrement = QString( "DEFAULT nextval('magnatune_track_seq')" );
        albumsAutoIncrement = QString( "DEFAULT nextval('magnatune_album_seq')" );
        artistAutoIncrement = QString( "DEFAULT nextval('magnatune_artist_seq')" );
        moodsAutoIncrement  = QString( "DEFAULT nextval('magnatune_moods_seq')" );

    }
    else if ( db->getDbConnectionType() == DbConnection::mysql )
    {
        tracksAutoIncrement = "AUTO_INCREMENT";
        albumsAutoIncrement = "AUTO_INCREMENT";
        artistAutoIncrement = "AUTO_INCREMENT";
        moodsAutoIncrement = "AUTO_INCREMENT";
    }

    // create table containing tracks
    QString queryString = "CREATE TABLE magnatune_tracks ("
                          "id INTEGER PRIMARY KEY " + tracksAutoIncrement + ',' +
                          "name " + db->textColumnType() + ',' +
                          "track_number INTEGER,"
                          "length INTEGER,"
                          "album_id INTEGER,"
                          "artist_id INTEGER,"
                          "preview_lofi " + db->exactTextColumnType() + ',' +
                          "preview_hifi " + db->exactTextColumnType() + ");";

    debug() << "Creating mangnatune_tracks: " << queryString << endl;


    QStringList result = db->query( queryString );

    //Create album table
    queryString = "CREATE TABLE magnatune_albums ("
                  "id INTEGER PRIMARY KEY " + albumsAutoIncrement + ',' +
                  "name " + db->textColumnType() + ',' +
                  "year INTEGER,"
                  "artist_id INTEGER,"
                  "album_code " + db->textColumnType() + ',' +
                  "cover_url " + db->exactTextColumnType() + ',' +
                  "description " + db->exactTextColumnType() + ");";

    debug() << "Creating Mangnatune_albums: " << queryString << endl;

    result = db->query( queryString );

    //Create artist table
    queryString = "CREATE TABLE magnatune_artists ("
                  "id INTEGER PRIMARY KEY " + artistAutoIncrement + ',' +
                  "name " + db->textColumnType() + ',' +
                  "artist_page " + db->exactTextColumnType() + ',' +
                  "description " + db->textColumnType() + ',' +
                  "photo_url " + db->exactTextColumnType() + ");";

    debug() << "Creating mangnatune_artist: " << queryString << endl;

    result = db->query( queryString );

    //create moods table
     queryString = "CREATE TABLE magnatune_moods ("
                  "id INTEGER PRIMARY KEY " + moodsAutoIncrement + ',' +
                  "track_id INTEGER," +
                  "mood " + db->textColumnType() + ");";

    debug() << "Creating mangnatune_moods: " << queryString << endl;

    result = db->query( queryString );



}

void 
MagnatuneDatabaseHandler::destroyDatabase( )
{
    CollectionDB *db = CollectionDB::instance();
    QStringList result = db->query( "DROP TABLE magnatune_tracks;" );
    result = db->query( "DROP TABLE magnatune_albums;" );
    result = db->query( "DROP TABLE magnatune_artists;" );
    result = db->query( "DROP TABLE magnatune_moods;" );

    if ( db->getDbConnectionType() == DbConnection::postgresql )
    {
        db->query( QString( "DROP SEQUENCE magnatune_track_seq;" ) );
        db->query( QString( "DROP SEQUENCE magnatune_album_seq;" ) );
        db->query( QString( "DROP SEQUENCE magnatune_artist_seq;" ) );
        db->query( QString( "DROP SEQUENCE magnatune_moods_seq;" ) );
    }
}

int 
MagnatuneDatabaseHandler::insertTrack( ServiceTrack *track )
{
    MagnatuneTrack * mTrack = dynamic_cast<MagnatuneTrack *> ( track );

    CollectionDB *db = CollectionDB::instance();
    QString queryString = "INSERT INTO magnatune_tracks ( name, track_number, length, "
                          "album_id, artist_id, preview_lofi, preview_hifi ) VALUES ( '"
                          + db->escapeString( mTrack->name()) + "', "
                          + QString::number( mTrack->trackNumber() ) + ", "
                          + QString::number( mTrack->length() ) + ", "
                          + QString::number( mTrack->albumId() ) + ", "
                          + QString::number( mTrack->artistId() ) + ", '"
                          + db->escapeString( mTrack->lofiUrl() ) + "', '"
                          + db->escapeString( mTrack->url() ) + "' );";


    // debug() << "Adding Magnatune track " << queryString << endl;
    int trackId = db->insert( queryString, NULL );

    return trackId; 

    
}

int 
MagnatuneDatabaseHandler::insertAlbum( ServiceAlbum *album )
{

    MagnatuneAlbum * mAlbum = dynamic_cast<MagnatuneAlbum *> ( album );

    QString queryString;
    CollectionDB *db = CollectionDB::instance();
    queryString = "INSERT INTO magnatune_albums ( name, year, artist_id, "
                  "album_code, cover_url, description ) VALUES ( '"
                  + db->escapeString( db->escapeString( mAlbum->name() ) ) + "', "
                  + QString::number( mAlbum->launchYear() ) + ", "
                  + QString::number( mAlbum->artistId() ) + ", '"
                  + db->escapeString( mAlbum->albumCode() ) + "', '"
                  + db->escapeString( mAlbum->coverUrl() ) + "', '"
                  + db->escapeString( mAlbum->description() )+ "' );";

    //debug() << "Adding Magnatune album " << queryString << endl;

    return db->insert( queryString, 0 );
}



int 
MagnatuneDatabaseHandler::insertArtist( ServiceArtist *artist )
{
    MagnatuneArtist * mArtist = dynamic_cast<MagnatuneArtist *> ( artist );

    QString queryString;
    CollectionDB *db = CollectionDB::instance();
    queryString = "INSERT INTO magnatune_artists ( name, artist_page, description, "
                  "photo_url ) VALUES ( '"
                  + db->escapeString( db->escapeString( mArtist->name() ) ) + "', '"
                  + db->escapeString( mArtist->magnatuneUrl()) + "', '"
                  + db->escapeString( mArtist->description() ) + "', '"
                  + db->escapeString( mArtist->photoUrl() ) + "' );";

    //debug() << "Adding Magnatune artist " << queryString << endl;

    return db->insert( queryString, 0 );
}


void 
MagnatuneDatabaseHandler::begin( )
{

    CollectionDB *db = CollectionDB::instance();

    QString queryString = "BEGIN;";

    db->query( queryString );
}

void 
MagnatuneDatabaseHandler::commit( )
{
    CollectionDB *db = CollectionDB::instance();
    QString queryString = "COMMIT;";

    db->query( queryString );

}

void MagnatuneDatabaseHandler::insertMoods(int trackId, QStringList moods)
{
 
    QString queryString;
    CollectionDB *db = CollectionDB::instance();

    foreach( QString mood, moods ) {
        queryString = "INSERT INTO magnatune_moods ( track_id, mood ) VALUES ( "
                      + QString::number( trackId ) + ", '"
                      + db->escapeString( mood ) +  "' );";


        //debug() << "Adding Magnatune mood: " << queryString << endl;
        db->insert( queryString, NULL ); 
    }
}

int MagnatuneDatabaseHandler::getArtistIdByExactName(const QString & name)
{

    CollectionDB *db = CollectionDB::instance();

    QString queryString = "SELECT id from magnatune_artists WHERE name='" + db->escapeString( name ) + "';";
    QStringList result = db->query( queryString );

    //debug() << "Looking for id of artist " << name << ":" << endl;

    if ( result.size() < 1 ) return -1;
    int artistId = result.first().toInt();
    
    //debug() << "    Found: " << QString::number( artistId ) << ":" << endl;
    
    return artistId;

}

int MagnatuneDatabaseHandler::getAlbumIdByAlbumCode(const QString & albumcode)
{
    CollectionDB *db = CollectionDB::instance();

    QString queryString = "SELECT id from magnatune_albums WHERE album_code='" + db->escapeString( albumcode ) + "';";
    QStringList result = db->query( queryString );

    //debug() << "Looking for id of album " << albumcode << ":" << endl;

    if ( result.size() < 1 ) return -1;
    int albumId = result.first().toInt();
    
    //debug() << "  Found: " << QString::number( albumId ) << ":" << endl;
    
    return albumId;
}






