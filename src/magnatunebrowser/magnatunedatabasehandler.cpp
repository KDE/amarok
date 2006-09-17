//
// C++ Implementation: magnatunedatabasehandler
//
// Description: 
//
//
// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "magnatunedatabasehandler.h"

#include "debug.h"


MagnatuneDatabaseHandler * MagnatuneDatabaseHandler::m_pInstance = 0;

MagnatuneDatabaseHandler * MagnatuneDatabaseHandler::instance()
{
	if (m_pInstance == 0) {
		m_pInstance = new MagnatuneDatabaseHandler();
	}
	return m_pInstance;
}


MagnatuneDatabaseHandler::MagnatuneDatabaseHandler()
{
}


MagnatuneDatabaseHandler::~MagnatuneDatabaseHandler()
{
}

void MagnatuneDatabaseHandler::createDatabase( )
{
    //Get database instance
    CollectionDB* db = CollectionDB::instance();

    QString tracksAutoIncrement = "";
    QString albumsAutoIncrement = "";
    QString artistAutoIncrement = "";

    if ( db->getDbConnectionType() == DbConnection::postgresql )
    {
        db->query( QString( "CREATE SEQUENCE magnatune_track_seq;" ) );
        db->query( QString( "CREATE SEQUENCE magnatune_album_seq;" ) );
        db->query( QString( "CREATE SEQUENCE magnatune_artist_seq;" ) );

        tracksAutoIncrement = QString("DEFAULT nextval('magnatune_track_seq')");
        albumsAutoIncrement = QString("DEFAULT nextval('magnatune_album_seq')");
        artistAutoIncrement = QString("DEFAULT nextval('magnatune_artist_seq')");

    }
    else if ( db->getDbConnectionType() == DbConnection::mysql )
    {
        tracksAutoIncrement = "AUTO_INCREMENT";
        albumsAutoIncrement = "AUTO_INCREMENT";
        artistAutoIncrement = "AUTO_INCREMENT";

    }

   // create table containing tracks
   QString queryString = "CREATE TABLE magnatune_tracks ("
                    "id INTEGER PRIMARY KEY " + tracksAutoIncrement + ","
                    "name " + db->textColumnType() + ","
                    "track_number INTEGER,"
                    "length INTEGER,"
                    "album_id INTEGER,"
                    "artist_id INTEGER,"
                    "preview_lofi " + db->exactTextColumnType()+ ","
                    "preview_hifi " + db->exactTextColumnType()+ ");";

   debug() << "Creating mangnatune_tracks: " << queryString << endl;


   QStringList result = db->query(queryString);
 
   //Create album table
   queryString = "CREATE TABLE magnatune_albums ("
                    "id INTEGER PRIMARY KEY " + albumsAutoIncrement + ","
                    "name " + db->textColumnType() + ","
                    "year INTEGER,"
                    "artist_id INTEGER,"
                    "genre " + db->textColumnType() + ","
                    "album_code " + db->textColumnType() + ","
                    "cover_url " + db->exactTextColumnType()+ ");";

   debug() << "Creating mangnatune_albums: " << queryString << endl;

   result = db->query(queryString);

   //Create artist table
   queryString = "CREATE TABLE magnatune_artists ("
                    "id INTEGER PRIMARY KEY " + artistAutoIncrement + ","
                    "name " + db->textColumnType() + ","
                    "artist_page " + db->exactTextColumnType()+ ","
                    "description " + db->textColumnType() + ","
                    "photo_url " + db->exactTextColumnType()+ ");";

   debug() << "Creating mangnatune_artist: " << queryString << endl;

   result = db->query(queryString);

   

}

void MagnatuneDatabaseHandler::destroyDatabase( )
{
    CollectionDB* db = CollectionDB::instance();
    QStringList result = db->query("DROP TABLE IF EXISTS magnatune_tracks;");
    result = db->query("DROP TABLE IF EXISTS magnatune_albums;");
    result = db->query("DROP TABLE IF EXISTS magnatune_artists;");

     if ( db->getDbConnectionType() == DbConnection::postgresql )
     {
           db->query( QString( "DROP SEQUENCE IF EXISTS magnatune_track_seq;" ) );
           db->query( QString( "DROP SEQUENCE IF EXISTS magnatune_album_seq;" ) );
           db->query( QString( "DROP SEQUENCE IF EXISTS magnatune_artist_seq;" ) );
     }   
}

int MagnatuneDatabaseHandler::insertTrack(MagnatuneTrack * track, int albumId, int artistId)
{
     QString numberString;

     CollectionDB* db = CollectionDB::instance();
     QString queryString = "INSERT INTO magnatune_tracks ( name, track_number, length, album_id, artist_id, preview_lofi, preview_hifi ) VALUES ( '" + db->escapeString(track->getName()) + "', " + QString::number(track->getTrackNumber()) + ", " + QString::number(track->getDuration()) +", " + QString::number(albumId) + ", " + QString::number(artistId) + ", '" + db->escapeString(track->getLofiURL()) + "', '" + db->escapeString(track->getHifiURL()) + "' );";


    // debug() << "Adding Magnatune track " << queryString << endl;

    return db->insert(queryString, NULL);
}

int MagnatuneDatabaseHandler::insertAlbum( MagnatuneAlbum * album, int artistId )
{
     QString numberString;

     CollectionDB* db = CollectionDB::instance();
     QString queryString = "INSERT INTO magnatune_albums ( name, year, artist_id, genre, album_code, cover_url ) VALUES ( '" + db->escapeString(db->escapeString(album->getName())) + "', "  + QString::number(album->getLaunchDate().year()) + ", " + QString::number(artistId) + ", '" + db->escapeString(album->getMp3Genre()) + "', '" + album->getAlbumCode() + "', '" + db->escapeString(album->getCoverURL()) + "' );";

    //debug() << "Adding Magnatune album " << queryString << endl;

    return db->insert(queryString, NULL);
}

int MagnatuneDatabaseHandler::insertTracks(MagnatuneTrackList tracks, int albumId, int artistId )
{
    	CollectionDB* db = CollectionDB::instance();

	QString queryString;
        MagnatuneTrackList::iterator it;
        for ( it = tracks.begin(); it != tracks.end(); ++it ) {

		queryString += "INSERT INTO magnatune_tracks ( name, track_number, length, album_id, artist_id, preview_lofi, preview_hifi ) VALUES ( '" + db->escapeString((*it).getName()) + "', " + QString::number((*it).getTrackNumber()) + ", " + QString::number((*it).getDuration()) +", " + QString::number(albumId) + ", " + QString::number(artistId) + ", '" + db->escapeString((*it).getLofiURL()) + "', '" + db->escapeString((*it).getHifiURL()) + "' ); ";

    	}

	
    	return db->insert(queryString, NULL);
	
}

int MagnatuneDatabaseHandler::insertArtist(MagnatuneArtist * artist)
{

     CollectionDB* db = CollectionDB::instance();
     QString queryString = "INSERT INTO magnatune_artists ( name, artist_page, description, photo_url ) VALUES ( '" + db->escapeString(db->escapeString(artist->getName())) +  "', '" + db->escapeString(artist->getHomeURL()) + "', '" + db->escapeString(artist->getDescription()) + "', '" + db->escapeString(artist->getPhotoURL()) + "' );";

    //debug() << "Adding Magnatune artist " << queryString << endl;

    return db->insert(queryString, NULL);
}


int MagnatuneDatabaseHandler::GetArtistIdByExactName( QString name )
{
    CollectionDB* db = CollectionDB::instance();

   QString queryString = "SELECT id from magnatune_artists WHERE name='" + name + "';";

   QStringList result = db->query(queryString);

   debug() << "Looking for id of artist " << name << ":" << endl;
   debug() << "  found: " << result << "!" << endl;

   if (result.size() < 1) return -1;

   return result.first().toInt();

}

MagnatuneArtistList MagnatuneDatabaseHandler::getArtistsByGenre( QString genre )
{

  QString genreSql = "";

  if (genre != "All") {
     genreSql = "magnatune_albums.genre='" + genre + "' AND ";
  }

  CollectionDB* db = CollectionDB::instance();

   QString queryString = "SELECT DISTINCT magnatune_artists.id, " 
			 "magnatune_artists.name, magnatune_artists.artist_page, "
			 "magnatune_artists.description, magnatune_artists.photo_url "
			 "FROM magnatune_albums, magnatune_artists "
			 "WHERE " + genreSql + "magnatune_albums.artist_id = magnatune_artists.id;";

   QStringList result = db->query(queryString);


   debug() << "Looking for artist in genre Rock..." << endl;
   debug() << "  found: " << result << "!" << endl;

   MagnatuneArtistList list;

   while (result.size() > 0) {
       MagnatuneArtist artist;

       artist.setId(result.front().toInt());
       result.pop_front();

       artist.setName(result.front());
       result.pop_front();

       artist.setHomeURL(result.front());
       result.pop_front();

       artist.setDescription(result.front());
       result.pop_front();

       artist.setPhotoURL(result.front());
       result.pop_front();

       list.append(artist);
   }

   return list;

 
}

MagnatuneAlbumList MagnatuneDatabaseHandler::getAlbumsByArtistId( int id, QString genre )
{

    QString genreSqlString;
    
    if ( genre == "" ) {
        genreSqlString = "";
    } else {
        genreSqlString = " AND magnatune_albums.genre='" + genre + "'";
    }
    
    CollectionDB* db = CollectionDB::instance();
    
    QString queryString = "SELECT DISTINCT id, " 
                            "name, year, "
                            "artist_id, genre, "
                            "album_code, cover_url "
                            "FROM magnatune_albums "
                            "WHERE artist_id = '" + QString::number( id ) +"'";
    
    queryString += genreSqlString;
    queryString += ";";
                            
    
    QStringList result = db->query( queryString );
    
    
    MagnatuneAlbumList list;
    debug() << "Looking for Albums..." << endl;
    debug() << "Query string:" << queryString << endl;
    
    debug() << "  found: " << result << "!" << endl;
    
    while (result.size() > 0) {
        MagnatuneAlbum album;
    
        album.setId(result.front().toInt());
        result.pop_front();
    
        album.setName(result.front());
        result.pop_front();
    
        album.setLaunchDate(QDate(result.front().toInt(), 1, 1));
        result.pop_front();
    
        album.setArtistId(result.front().toInt());
        result.pop_front();
    
        album.setMp3Genre(result.front());
        result.pop_front();
    
        album.setAlbumCode(result.front());
        result.pop_front();
    
        album.setCoverURL(result.front());
        result.pop_front();
    
        list.append(album);
    }
    
    return list;


}

MagnatuneTrackList MagnatuneDatabaseHandler::getTracksByAlbumId( int id )
{
    
    CollectionDB* db = CollectionDB::instance();
    
    QString queryString = "SELECT DISTINCT id, " 
                            "name, track_number, "
                            "length, album_id, "
                            "artist_id, preview_lofi, "
                            "preview_hifi "
                            "FROM magnatune_tracks "
                            "WHERE album_id = '" + QString::number(id) +"';";
    
    QStringList result = db->query(queryString);
    
    
    MagnatuneTrackList list;
    
    debug() << "Looking for tracks..." << endl;
    debug() << "Query string:" << queryString << endl;
    
    debug() << "  found: " << result << "!" << endl;
    
    while (result.size() > 0) {
    
        debug() << "track start" << endl;
        MagnatuneTrack track;
    
        track.setId(result.front().toInt());
        result.pop_front();
    
        track.setName(result.front());
        result.pop_front();
    
        track.setTrackNumber(result.front().toInt());
        result.pop_front();
    
        track.setDuration(result.front().toInt());
        result.pop_front();
    
        track.setAlbumId(result.front().toInt());
        result.pop_front();
    
        track.setArtistId(result.front().toInt());
        result.pop_front();
    
        track.setLofiURL(result.front());
        result.pop_front();
    
        track.setHifiURL(result.front());
        result.pop_front();
    
        list.append(track);
        debug() << "track end" << endl;
    }
    
    return list;

}

QStringList MagnatuneDatabaseHandler::getAlbumGenres( )
{
	
    CollectionDB* db = CollectionDB::instance();
    
    QString queryString = "SELECT DISTINCT genre " 
                            "FROM magnatune_albums "
                            "ORDER BY genre;";
    
    return db->query(queryString);
}

void MagnatuneDatabaseHandler::begin( )
{
	
    CollectionDB* db = CollectionDB::instance();
    
    QString queryString = "BEGIN;";
    
    db->query(queryString);
}

void MagnatuneDatabaseHandler::commit( )
{
    CollectionDB* db = CollectionDB::instance();
    QString queryString = "COMMIT;";
    
    db->query(queryString);

}

MagnatuneArtist MagnatuneDatabaseHandler::getArtistById( int id )
{

    CollectionDB* db = CollectionDB::instance();
    
    QString queryString = "SELECT id, " 
                            "name, artist_page, "
                            "description, photo_url "
                            "FROM magnatune_artists "
                            "WHERE id = '" + QString::number(id) + "';";
    
    QStringList result = db->query(queryString);
    
    MagnatuneArtist artist;
    
    if (result.size() == 5) {
    
        artist.setId(result.front().toInt());
        result.pop_front();
    
        artist.setName(result.front());
        result.pop_front();
    
        artist.setHomeURL(result.front());
        result.pop_front();
    
        artist.setDescription( result.front() );
        result.pop_front();
    
        artist.setPhotoURL( result.front() );
        result.pop_front();
    
    }
   

   return artist;
}

MagnatuneAlbum MagnatuneDatabaseHandler::getAlbumById( int id )
{

    
    CollectionDB* db = CollectionDB::instance();

    QString queryString = "SELECT id, " 
            "name, year, "
            "artist_id, genre, "
            "album_code, cover_url "
            "FROM magnatune_albums "
            "WHERE id = '" + QString::number(id) + "';";

    QStringList result = db->query( queryString );

    debug() << "  found: " << result << "!" << endl;
    
    MagnatuneAlbum album;

    if ( result.size() == 7 ) {

        album.setId( result.front().toInt() );
        result.pop_front();

        album.setName( result.front() );
        result.pop_front();

        album.setLaunchDate( QDate( result.front().toInt(), 1, 1 ) );
        result.pop_front();

        album.setArtistId( result.front().toInt() );
        result.pop_front();

        album.setMp3Genre( result.front() );
        result.pop_front();
        
        album.setAlbumCode( result.front() );
        result.pop_front();
        
        album.setCoverURL( result.front() );
        result.pop_front();

    }

    return album;

}


MagnatuneTrackList MagnatuneDatabaseHandler::getTracksByArtistId( int id )
{

    MagnatuneAlbumList albums = getAlbumsByArtistId( id, "" );
    MagnatuneAlbumList::iterator it;
    MagnatuneTrackList tracks;
      
    

    for ( it = albums.begin(); it != albums.end(); ++it ) {

      tracks += getTracksByAlbumId( (*it).getId() );

    }

    return tracks;

}





