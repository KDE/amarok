/****************************************************************************************
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhn@kde.org>                           *
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

#include "MagnatuneDatabaseHandler.h"

#include <core-impl/storage/StorageManager.h>
#include <core/support/Debug.h>
#include <core/storage/SqlStorage.h>

using namespace Meta;

MagnatuneDatabaseHandler::MagnatuneDatabaseHandler()
{}


MagnatuneDatabaseHandler::~MagnatuneDatabaseHandler()
{}

void
MagnatuneDatabaseHandler::createDatabase( )
{
    //Get database instance
    auto sqlDb = StorageManager::instance()->sqlStorage();

    QString autoIncrement = QStringLiteral("AUTO_INCREMENT");

    // create table containing tracks
    QString queryString = QStringLiteral("CREATE TABLE magnatune_tracks ("
                          "id INTEGER PRIMARY KEY ") + autoIncrement + QLatin1Char(',') +
                          QStringLiteral("name ") + sqlDb->textColumnType() + QLatin1Char(',') +
                          QStringLiteral("track_number INTEGER,"
                          "length INTEGER,"
                          "album_id INTEGER,"
                          "artist_id INTEGER,"
                          "preview_lofi ") + sqlDb->exactTextColumnType() + QLatin1Char(',') +
                          QStringLiteral("preview_ogg ") + sqlDb->exactTextColumnType() + QLatin1Char(',') +
                          QStringLiteral("preview_url ") + sqlDb->exactTextColumnType() + QStringLiteral(") ENGINE = MyISAM;");

    debug() << "Creating magnatune_tracks: " << queryString;


    QStringList result = sqlDb->query( queryString );

    sqlDb->query( QStringLiteral("CREATE INDEX magnatune_tracks_album_id ON magnatune_tracks(album_id);") );
    sqlDb->query( QStringLiteral("CREATE INDEX magnatune_tracks_artist_id ON magnatune_tracks(artist_id);") );

    //Create album table
    queryString = QStringLiteral("CREATE TABLE magnatune_albums ("
                  "id INTEGER PRIMARY KEY ") + autoIncrement + QLatin1Char(',') +
                  QStringLiteral("name ") + sqlDb->textColumnType() + QLatin1Char(',') +
                  QStringLiteral("year INTEGER,"
                  "artist_id INTEGER,"
                  "album_code ") + sqlDb->textColumnType() + QLatin1Char(',') +
                  QStringLiteral("cover_url ") + sqlDb->exactTextColumnType() + QLatin1Char(',') +
                  QStringLiteral("description ") + sqlDb->exactTextColumnType() + QStringLiteral(") ENGINE = MyISAM;");

    debug() << "Creating Magnatune_albums: " << queryString;

    result = sqlDb->query( queryString );

    sqlDb->query( QStringLiteral("CREATE INDEX magnatune_albums_name ON magnatune_albums(name);") );
    sqlDb->query( QStringLiteral("CREATE INDEX magnatune_albums_artist_id ON magnatune_albums(artist_id);") );


    //Create artist table
    queryString = QStringLiteral("CREATE TABLE magnatune_artists ("
                  "id INTEGER PRIMARY KEY ") + autoIncrement + QLatin1Char(',') +
                  QStringLiteral("name ") + sqlDb->textColumnType() + QLatin1Char(',') +
                  QStringLiteral("artist_page ") + sqlDb->exactTextColumnType() + QLatin1Char(',') +
                  QStringLiteral("description ") + sqlDb->textColumnType() + QLatin1Char(',') +
                  QStringLiteral("photo_url ") + sqlDb->exactTextColumnType() + QStringLiteral(") ENGINE = MyISAM;");

    debug() << "Creating magnatune_artist: " << queryString;

    result = sqlDb->query( queryString );

    sqlDb->query( QStringLiteral("CREATE INDEX magnatune_artists_name ON magnatune_artists(name);") );

    //create genre table
    queryString = QStringLiteral("CREATE TABLE magnatune_genre ("
                  "id INTEGER PRIMARY KEY ") + autoIncrement + QLatin1Char(',') +
                  QStringLiteral("name ") + sqlDb->textColumnType() + QLatin1Char(',') +
                  QStringLiteral("album_id INTEGER") + QStringLiteral(") ENGINE = MyISAM;");

    result = sqlDb->query( queryString );

    sqlDb->query( QStringLiteral("CREATE INDEX magnatune_genre_name ON magnatune_genre(name);") );
    sqlDb->query( QStringLiteral("CREATE INDEX magnatune_genre_album_id ON magnatune_genre(album_id);") );


    //create moods table
     queryString = QStringLiteral("CREATE TABLE magnatune_moods ("
                  "id INTEGER PRIMARY KEY ") + autoIncrement + QLatin1Char(',') +
                  QStringLiteral("track_id INTEGER,") +
                  QStringLiteral("mood ") + sqlDb->textColumnType() + QStringLiteral(") ENGINE = MyISAM;");

    debug() << "Creating magnatune_moods: " << queryString;

    result = sqlDb->query( queryString );



}

void
MagnatuneDatabaseHandler::destroyDatabase( )
{
    auto sqlDb = StorageManager::instance()->sqlStorage();
    QStringList result = sqlDb->query( QStringLiteral("DROP TABLE IF EXISTS magnatune_tracks;") );
    result = sqlDb->query( QStringLiteral("DROP TABLE IF EXISTS magnatune_albums;") );
    result = sqlDb->query( QStringLiteral("DROP TABLE IF EXISTS magnatune_artists;") );
    result = sqlDb->query( QStringLiteral("DROP TABLE IF EXISTS magnatune_genre;") );
    result = sqlDb->query( QStringLiteral("DROP TABLE IF EXISTS magnatune_moods;") );


    /* that would only work for db2/oracle. Other databases connect the index to the table (which we just dropped)
    result = sqlDb->query( "DROP INDEX magnatune_tracks_artist_id;");
    result = sqlDb->query( "DROP INDEX magnatune_tracks_album_id;");
    result = sqlDb->query( "DROP INDEX magnatune_album_name;");
    result = sqlDb->query( "DROP INDEX magnatune_album_artist_id;");
    result = sqlDb->query( "DROP INDEX magnatune_artist_name;");
    result = sqlDb->query( "DROP INDEX magnatune_genre_album_id;");
    result = sqlDb->query( "DROP INDEX magnatune_genre_name;");
    */

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

    auto sqlDb = StorageManager::instance()->sqlStorage();
    QString queryString = QStringLiteral("INSERT INTO magnatune_tracks ( name, track_number, length, "
            "album_id, artist_id, preview_lofi, preview_ogg, preview_url ) VALUES ( '")
                          + sqlDb->escape( mTrack->name()) + QStringLiteral("', ")
                          + QString::number( mTrack->trackNumber() ) + QStringLiteral(", ")
                          + QString::number( mTrack->length() * 1000 ) + QStringLiteral(", ")
                          + QString::number( mTrack->albumId() ) + QStringLiteral(", ")
                          + QString::number( mTrack->artistId() ) + QStringLiteral(", '")
                          + sqlDb->escape( mTrack->lofiUrl() ) + QStringLiteral("', '")
                          + sqlDb->escape( mTrack->oggUrl() ) + QStringLiteral("', '")
                          + sqlDb->escape( mTrack->uidUrl() ) + QStringLiteral("' );");


    // debug() << "Adding Magnatune track " << queryString;
    int trackId = sqlDb->insert( queryString, QString() );

    return trackId;


}

int
MagnatuneDatabaseHandler::insertAlbum( ServiceAlbum *album )
{

    MagnatuneAlbum * mAlbum = static_cast<MagnatuneAlbum *> ( album );

    QString queryString;
    auto sqlDb = StorageManager::instance()->sqlStorage();
    queryString = QStringLiteral("INSERT INTO magnatune_albums ( name, year, artist_id, "
                  "album_code, cover_url, description ) VALUES ( '")
                  + sqlDb->escape( sqlDb->escape( mAlbum->name() ) ) + QStringLiteral("', ")
                  + QString::number( mAlbum->launchYear() ) + QStringLiteral(", ")
                  + QString::number( mAlbum->artistId() ) + QStringLiteral(", '")
                  + sqlDb->escape( mAlbum->albumCode() ) + QStringLiteral("', '")
                  + sqlDb->escape( mAlbum->coverUrl() ) + QStringLiteral("', '")
                  + sqlDb->escape( mAlbum->description() )+ QStringLiteral("' );");

    //debug() << "Adding Magnatune album " << queryString;

    return sqlDb->insert( queryString, QString() );
}



int
MagnatuneDatabaseHandler::insertArtist( ServiceArtist *artist )
{
    MagnatuneArtist * mArtist = static_cast<MagnatuneArtist *> ( artist );

    QString queryString;
    auto sqlDb = StorageManager::instance()->sqlStorage();
    queryString = QStringLiteral("INSERT INTO magnatune_artists ( name, artist_page, description, "
                  "photo_url ) VALUES ( '")
                  + sqlDb->escape( mArtist->name() ) + QStringLiteral("', '")
                  + sqlDb->escape( mArtist->magnatuneUrl().url() ) + QStringLiteral("', '")
                  + sqlDb->escape( mArtist->description() ) + QStringLiteral("', '")
                  + sqlDb->escape( mArtist->photoUrl().url() ) + QStringLiteral("' );");

    //debug() << "Adding Magnatune artist " << queryString;

    return sqlDb->insert( queryString, QString() );
}


void
MagnatuneDatabaseHandler::begin( )
{

    auto sqlDb = StorageManager::instance()->sqlStorage();

    QString queryString = QStringLiteral("BEGIN;");

    sqlDb->query( queryString );
}

void
MagnatuneDatabaseHandler::commit( )
{
    auto sqlDb = StorageManager::instance()->sqlStorage();
    QString queryString = QStringLiteral("COMMIT;");

    sqlDb->query( queryString );
    sqlDb->query( QStringLiteral("FLUSH TABLES;") );
}

void MagnatuneDatabaseHandler::insertMoods(int trackId, const QStringList &moods)
{

    QString queryString;
    auto sqlDb = StorageManager::instance()->sqlStorage();

    for( const QString &mood : moods ) {
        queryString = QStringLiteral("INSERT INTO magnatune_moods ( track_id, mood ) VALUES ( ")
                      + QString::number( trackId ) + QStringLiteral(", '")
                      + sqlDb->escape( mood ) +  QStringLiteral("' );");


        //debug() << "Adding Magnatune mood: " << queryString;
        sqlDb->insert( queryString, QString() );
    }
}

int MagnatuneDatabaseHandler::getArtistIdByExactName(const QString & name)
{

    auto sqlDb = StorageManager::instance()->sqlStorage();

    QString queryString = QStringLiteral("SELECT id from magnatune_artists WHERE name='") + sqlDb->escape( name ) + QStringLiteral("';");
    QStringList result = sqlDb->query( queryString );

    //debug() << "Looking for id of artist " << name << ":";

    if ( result.size() < 1 ) return -1;
    int artistId = result.first().toInt();

    //debug() << "    Found: " << QString::number( artistId ) << ":";

    return artistId;

}

int MagnatuneDatabaseHandler::getAlbumIdByAlbumCode(const QString & albumcode)
{
    auto sqlDb = StorageManager::instance()->sqlStorage();

    QString queryString = QStringLiteral("SELECT id from magnatune_albums WHERE album_code='") + sqlDb->escape( albumcode ) + QStringLiteral("';");
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
    auto sqlDb = StorageManager::instance()->sqlStorage();
    queryString = QStringLiteral("INSERT INTO magnatune_genre ( album_id, name "
                  ") VALUES ( ")
                  + QString::number ( genre->albumId() ) + QStringLiteral(", '")
                  + sqlDb->escape( genre->name() ) + QStringLiteral("' );");

    //debug() << "Adding Jamendo genre " << queryString;

    return sqlDb->insert( queryString, QString() );
}






