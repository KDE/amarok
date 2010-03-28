/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#include "TestAlbumCompilationChange.h"

#include "DatabaseUpdater.h"
#include "core/support/Debug.h"
#include "DefaultSqlQueryMakerFactory.h"
#include "core/meta/Meta.h"
#include "mysqlecollection/MySqlEmbeddedStorage.h"
#include "SqlCollection.h"
#include "SqlRegistry.h"
#include "SqlMountPointManagerMock.h"

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestAlbumCompilationChange )

TestAlbumCompilationChange::TestAlbumCompilationChange()
    : QObject()
    , m_collection( 0 )
    , m_storage( 0 )
    , m_tmpDir( 0 )
    , m_registry( 0 )
{
}

void
TestAlbumCompilationChange::initTestCase()
{
    m_tmpDir = new KTempDir();
    m_storage = new MySqlEmbeddedStorage( m_tmpDir->name() );
    m_collection = new Collections::SqlCollection( "testId", "testcollection" );
    m_collection->setSqlStorage( m_storage );
    m_collection->setMountPointManager( new SqlMountPointManagerMock() );
    m_collection->setQueryMakerFactory( new Collections::DefaultSqlQueryMakerFactory( m_collection ) );
    DatabaseUpdater updater;
    updater.setStorage( m_storage );
    updater.setCollection( m_collection );
    updater.update();
}

void
TestAlbumCompilationChange::cleanupTestCase()
{
    delete m_collection;
    //m_storage is deleted by SqlCollection
    //m_registry is deleted by SqlCollection
    delete m_tmpDir;
}

void
TestAlbumCompilationChange::init()
{
    m_registry = new SqlRegistry( m_collection );
    m_registry->setStorage( m_storage );
    m_collection->setRegistry( m_registry );
    //setup base data
    m_storage->query( "INSERT INTO artists(id, name) VALUES (1, 'artist1');" );
    m_storage->query( "INSERT INTO artists(id, name) VALUES (2, 'artist2');" );
    m_storage->query( "INSERT INTO artists(id, name) VALUES (3, 'artist3');" );

    m_storage->query( "INSERT INTO composers(id, name) VALUES (1, 'composer1');" );
    m_storage->query( "INSERT INTO genres(id, name) VALUES (1, 'genre1');" );
    m_storage->query( "INSERT INTO years(id, name) VALUES (1, '1');" );

    m_storage->query( "INSERT INTO urls(id, deviceid, rpath, uniqueid ) VALUES (1, -1, './IDoNotExist.mp3', 'uid://1');" );
    m_storage->query( "INSERT INTO urls(id, deviceid, rpath, uniqueid ) VALUES (2, -1, './IDoNotExistAsWell.mp3', 'uid://2');" );
    m_storage->query( "INSERT INTO urls(id, deviceid, rpath, uniqueid ) VALUES (3, -1, './MeNeither.mp3', 'uid:/3');" );


}

void
TestAlbumCompilationChange::cleanup()
{
    delete m_registry;
    m_collection->setRegistry( 0 );
    m_storage->query( "TRUNCATE TABLE years;" );
    m_storage->query( "TRUNCATE TABLE genres;" );
    m_storage->query( "TRUNCATE TABLE composers;" );
    m_storage->query( "TRUNCATE TABLE albums;" );
    m_storage->query( "TRUNCATE TABLE artists;" );
    m_storage->query( "TRUNCATE TABLE tracks;" );
    m_storage->query( "TRUNCATE TABLE urls;" );
}

void
TestAlbumCompilationChange::testSetCompilationWithoutExistingCompilation()
{
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (1, 'album1',1);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );" );
    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::AlbumPtr album;
    album = track->album();
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    Meta::SqlAlbum *sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    sqlAlbum->setCompilation( true );
    QVERIFY( !album->hasAlbumArtist() );
    QVERIFY( album->isCompilation() );

    QStringList trackResult = m_storage->query("SELECT album FROM tracks WHERE id = 1");
    QCOMPARE( trackResult.count(), 1 );
    QCOMPARE( trackResult.first(), QString( "1" ) ); //track still points at the same row in albums

    QStringList albumResult = m_storage->query( "SELECT name, artist FROM albums WHERE id = 1" );
    QCOMPARE( albumResult.count(), 2 );
    QCOMPARE( albumResult.first(), QString( "album1" ) ); //album name did not change
    QCOMPARE( albumResult.at( 1 ), QString() ); //artist should be <null>, which is an empty string
    QVERIFY( track->album()->isCompilation() ); //track is now in compilation
    QCOMPARE( track->album(), album ); //track still returns the same album object
}

void
TestAlbumCompilationChange::testSetCompilationWithExistingCompilation()
{
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (1, 'albumAndCompilation1',1);" );
    m_storage->query( "INSERT INTO albums(id,name) VALUES (2, 'albumAndCompilation1');" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );" );
    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::AlbumPtr album = track->album();
    Meta::AlbumPtr compilation = m_registry->getAlbum( "albumAndCompilation1", -1, 0 );
    QVERIFY( compilation->isCompilation() );

    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    Meta::SqlAlbum *sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    sqlAlbum->setCompilation( true );
    QVERIFY( !album->hasAlbumArtist() );
    QVERIFY( album->isCompilation() );

    QStringList trackResult = m_storage->query("SELECT album FROM tracks WHERE id = 1");
    QCOMPARE( trackResult.count(), 1 );
    QCOMPARE( trackResult.first(), QString( "2" ) ); //track points at the compilation row

    QStringList albumResult = m_storage->query( "SELECT name, artist FROM albums WHERE id = 1" );
    QCOMPARE( albumResult.count(), 0 ); //album1 should not exist anymore as there is no track associated to it
    QVERIFY( track->album()->isCompilation() ); //track is now in compilation
    QCOMPARE( track->album(), compilation ); //track returns new album, but the same object that we retrieved above
}

void
TestAlbumCompilationChange::testUnsetCompilationWithoutExistingAlbum()
{
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (1, 'album1',0);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );" );
    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::AlbumPtr album;
    album = track->album();
    QVERIFY( !album->hasAlbumArtist() );
    QVERIFY( album->isCompilation() );
    Meta::SqlAlbum *sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    sqlAlbum->setCompilation( false );
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );

    QStringList trackResult = m_storage->query("SELECT album FROM tracks WHERE id = 1");
    QCOMPARE( trackResult.count(), 1 );
    QCOMPARE( trackResult.first(), QString( "1" ) ); //track still points at the same row in albums

    QStringList albumResult = m_storage->query( "SELECT name, artist FROM albums WHERE id = 1" );
    QCOMPARE( albumResult.count(), 2 );
    QCOMPARE( albumResult.first(), QString( "album1" ) ); //album name did not change
    QCOMPARE( albumResult.at( 1 ), QString( "1" ) ); //artist should be 1
    QVERIFY( track->album()->hasAlbumArtist() ); //track is not in compilation anymore
    QCOMPARE( track->album(), album ); //track still returns the same album object
}

void
TestAlbumCompilationChange::testUnsetCompilationWithExistingAlbum()
{
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (1, 'albumAndCompilation1',0);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (2, 'albumAndCompilation1',1);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );" );
    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::AlbumPtr compilation = track->album();
    Meta::AlbumPtr album = m_registry->getAlbum( "albumAndCompilation1", -1, 1 );
    QVERIFY( compilation->isCompilation() );
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );

    Meta::SqlAlbum *sqlCompilation = static_cast<Meta::SqlAlbum*>( compilation.data() );
    sqlCompilation->setCompilation( false );

    QStringList trackResult = m_storage->query("SELECT album FROM tracks WHERE id = 1");
    QCOMPARE( trackResult.count(), 1 );
    QCOMPARE( trackResult.first(), QString( "2" ) ); //track points at album row

    QStringList albumResult = m_storage->query( "SELECT name, artist FROM albums WHERE id = 1" );
    QCOMPARE( albumResult.count(), 0 ); //album1 should not exist anymore as there is no track associated to it
    QVERIFY( track->album()->hasAlbumArtist() ); //track is not in compilation anymore
    QCOMPARE( track->album(), album ); //track returns new album, but the same object that we retrieved above
}

void
TestAlbumCompilationChange::testUnsetCompilationWithMultipleExistingAlbums()
{
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (1,'albumAndCompilation1',0);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (2,'albumAndCompilation1',1);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (3,'albumAndCompilation1',2);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (2,2,'track2',2,3,1,1,1 );" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (3,2,'track3',1,2,1,1,1 );" );

    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::AlbumPtr compilation = track->album();
    QVERIFY( compilation->isCompilation() );

    Meta::AlbumPtr targetAlbum = m_registry->getAlbum( "albumAndCompilation1", -1, 1 );
    QVERIFY( targetAlbum->hasAlbumArtist() );

    Meta::SqlAlbum *sqlCompilation = static_cast<Meta::SqlAlbum*>( compilation.data() );
    sqlCompilation->setCompilation( false );

    QVERIFY( track->album()->hasAlbumArtist() ); //album is not a compilation anymore
    QCOMPARE( track->album()->albumArtist()->name(), QString( "artist1" ) ); //and it has the track's artist as albumartist

    QStringList trackResult = m_storage->query("SELECT album FROM tracks WHERE id = 1");
    QCOMPARE( trackResult.count(), 1 );
    QCOMPARE( trackResult.first(), QString( "2" ) ); //track points at correct album row

    QStringList albumResult = m_storage->query( "SELECT name, artist FROM albums WHERE id = 1" );
    QCOMPARE( albumResult.count(), 0 ); //album1 should not exist anymore as there is no track associated to it

    QCOMPARE( track->album(), targetAlbum ); //track returns existing object for the new album

}

void
TestAlbumCompilationChange::testUnsetCompilationWithArtistAFeaturingB()
{
    m_storage->query( "INSERT INTO artists(id,name) VALUES (4,'artist1 feat. artist2');" );

    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (1,'albumAndCompilation1',0);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (2,'albumAndCompilation1',1);" );

    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',4,1,1,1,1 );" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (2,2,'track2',1,2,1,1,1 );" );

    Meta::AlbumPtr targetAlbum = m_registry->getAlbum( "albumAndCompilation1", -1, 1 );
    QVERIFY( targetAlbum->hasAlbumArtist() );
    QCOMPARE( targetAlbum->albumArtist()->name(), QString( "artist1" ) );

    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    QCOMPARE( track->artist()->name(), QString( "artist1 feat. artist2" ) );

    Meta::AlbumPtr compilation = track->album();
    QVERIFY( compilation->isCompilation() );

    Meta::SqlAlbum *sqlCompilation = static_cast<Meta::SqlAlbum*>( compilation.data() );
    sqlCompilation->setCompilation( false );

    QVERIFY( track->album()->hasAlbumArtist() );
    //similar to the logic used in ScanResultProcessor, we should identify "artist1" as the albumartist
    QCOMPARE( track->album()->albumArtist()->name(), QString( "artist1" ) );

    QCOMPARE( track->album(), targetAlbum );
}

void
TestAlbumCompilationChange::testUnsetCompilationWithMultipleArtists()
{
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (1,'album1',0);" );

    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (2,2,'track2',2,1,1,1,1 );" );

    Meta::TrackPtr track1 = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::TrackPtr track2 = m_registry->getTrack( "/IDoNotExistAsWell.mp3" );

    Meta::AlbumPtr album = track1->album();
    QCOMPARE( album, track2->album() );
    QVERIFY( album->isCompilation() );

    Meta::SqlAlbum *sqlCompilation = static_cast<Meta::SqlAlbum*>( album.data() );
    sqlCompilation->setCompilation( false );

    QStringList albumsCount = m_storage->query( "SELECT count(*) FROM albums;" );
    QCOMPARE( albumsCount.first(), QString::number( 2 ) );

    QVERIFY( !track1->album()->isCompilation() );
    QVERIFY( !track2->album()->isCompilation() );


}

#include "TestAlbumCompilationChange.moc"
