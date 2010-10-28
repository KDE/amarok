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

#include "TestSqlAlbum.h"

#include <core/meta/Meta.h>
#include <core/capabilities/CustomActionsCapability.h>
#include <core-impl/collections/sqlcollection/SqlBookmarkThisCapability.h>
#include <core-impl/collections/sqlcollection/DatabaseUpdater.h>
#include <core-impl/collections/sqlcollection/DefaultSqlQueryMakerFactory.h>
#include <core-impl/collections/sqlcollection/SqlCollection.h>
#include <core-impl/collections/sqlcollection/SqlRegistry.h>
#include <core-impl/collections/sqlcollection/SqlMountPointManagerMock.h>
#include <core-impl/collections/sqlcollection/mysqlecollection/MySqlEmbeddedStorage.h>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestSqlAlbum )

TestSqlAlbum::TestSqlAlbum()
    : QObject()
    , m_collection( 0 )
    , m_storage( 0 )
    , m_tmpDir( 0 )
    , m_registry( 0 )
{
}

void
TestSqlAlbum::initTestCase()
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
TestSqlAlbum::cleanupTestCase()
{
    delete m_collection;
    //m_storage is deleted by SqlCollection
    //m_registry is deleted by SqlCollection
    delete m_tmpDir;
}

void
TestSqlAlbum::init()
{
    m_registry = new SqlRegistry( m_collection );
    m_registry->setStorage( m_storage );
    m_collection->setRegistry( m_registry );

    //setup base data
    m_storage->query( "TRUNCATE TABLE years;" );
    m_storage->query( "TRUNCATE TABLE genres;" );
    m_storage->query( "TRUNCATE TABLE composers;" );
    m_storage->query( "TRUNCATE TABLE albums;" );
    m_storage->query( "TRUNCATE TABLE artists;" );
    m_storage->query( "TRUNCATE TABLE tracks;" );
    m_storage->query( "TRUNCATE TABLE urls;" );

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
TestSqlAlbum::cleanup()
{
    delete m_registry;
    m_collection->setRegistry( 0 );
}

void
TestSqlAlbum::testCapabilities()
{
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (1, 'album1', 1);" );

    Meta::AlbumPtr album = m_collection->registry()->getAlbum( "album1", 1, 1 );

    QVERIFY( album->hasCapabilityInterface( Capabilities::Capability::CustomActions ) );
    Capabilities::CustomActionsCapability *cac = album->create<Capabilities::CustomActionsCapability>();
    QVERIFY( cac );
    QVERIFY( cac->customActions().count() > 4 );

    QVERIFY( album->hasCapabilityInterface( Capabilities::Capability::BookmarkThis ) );
    Capabilities::BookmarkThisCapability *btc = album->create<Capabilities::BookmarkThisCapability>();
    QVERIFY( btc );
    QVERIFY( btc->bookmarkAction() );

    QVERIFY( !album->hasCapabilityInterface( Capabilities::Capability::Updatable ) );

    // need to delete the actions. They hold pointers that prevent the registry from cleaning
    // it's state.
    foreach( QAction *action, cac->customActions() )
        delete action;
    delete btc->bookmarkAction();
    delete cac;
    delete btc;
}

void
TestSqlAlbum::testSetCompilationWithoutExistingCompilation()
{
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (1, 'album1',1);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );" );

    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::AlbumPtr album = track->album();
    Meta::SqlAlbum *sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    QCOMPARE( sqlAlbum->id(), 1 );
    QCOMPARE( album->tracks().count(), 1 );
    sqlAlbum->setCompilation( true );

    // now the track should be in the compilation
    album = track->album();
    sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QCOMPARE( sqlAlbum->id(), 2 ); // a new album
    QVERIFY( !album->hasAlbumArtist() );
    QVERIFY( album->isCompilation() );
 }

void
TestSqlAlbum::testSetCompilationWithExistingCompilation()
{
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (1, 'albumAndCompilation1',1);" );
    m_storage->query( "INSERT INTO albums(id,name) VALUES (2, 'albumAndCompilation1');" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );" );
    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::AlbumPtr album = track->album();
    Meta::AlbumPtr compilation = m_registry->getAlbum( "albumAndCompilation1", -1, 0 );
    QVERIFY( compilation->isCompilation() );

    Meta::SqlAlbum *sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    QCOMPARE( sqlAlbum->id(), 1 );
    QCOMPARE( album->tracks().count(), 1 );
    sqlAlbum->setCompilation( true );

    // now the track should be in the compilation
    album = track->album();
    sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QCOMPARE( sqlAlbum->id(), 2 ); // the exisitng compilation album
    QVERIFY( !album->hasAlbumArtist() );
    QVERIFY( album->isCompilation() );

    QCOMPARE( album, compilation ); //track returns new album, but the same object that we retrieved above

    QStringList albumResult = m_storage->query( "SELECT name, artist FROM albums WHERE id = 1" );
}

void
TestSqlAlbum::testUnsetCompilationWithoutExistingAlbum()
{
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (1, 'album1',NULL);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );" );

    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::AlbumPtr album = track->album();
    Meta::SqlAlbum *sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QVERIFY( !album->hasAlbumArtist() );
    QVERIFY( album->isCompilation() );
    sqlAlbum->setCompilation( false );

    // now the track should be in a normal album
    album = track->album();
    sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    QCOMPARE( album->name(), QString( "album1" ) ); //album name did not change
    QCOMPARE( album->albumArtist()->name(), track->artist()->name() ); //artist is the same
}

void
TestSqlAlbum::testUnsetCompilationWithExistingAlbum()
{
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (1, 'albumAndCompilation1',NULL);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (2, 'albumAndCompilation1',1);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );" );

    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::AlbumPtr compilation = track->album();
    Meta::SqlAlbum *sqlCompilation = static_cast<Meta::SqlAlbum*>( compilation.data() );
    QVERIFY( compilation->isCompilation() );
    sqlCompilation->setCompilation( false );

    // now the track should be in a normal album
    Meta::AlbumPtr album = track->album();
    Meta::SqlAlbum *sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QCOMPARE( sqlAlbum->id(), 2 ); // the albums should be the already existing one
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    QCOMPARE( album->name(), compilation->name() ); //album name did not change
    QCOMPARE( album->albumArtist()->name(), track->artist()->name() ); //artist is the same
}

void
TestSqlAlbum::testUnsetCompilationWithMultipleExistingAlbums()
{
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (1,'albumAndCompilation1',NULL);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (2,'albumAndCompilation1',1);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (3,'albumAndCompilation1',2);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (2,2,'track2',2,3,1,1,1 );" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (3,3,'track3',1,2,1,1,1 );" );

    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::AlbumPtr compilation = track->album();
    Meta::SqlAlbum *sqlCompilation = static_cast<Meta::SqlAlbum*>( compilation.data() );
    QVERIFY( compilation->isCompilation() );
    sqlCompilation->setCompilation( false );

    Meta::AlbumPtr album;
    Meta::SqlAlbum *sqlAlbum;

    // now the tracks should be in a normal album
    album = track->album();
    sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    QCOMPARE( album->name(), compilation->name() ); //album name did not change
    QCOMPARE( album->albumArtist()->name(), track->artist()->name() ); //artist is the same
    QCOMPARE( sqlAlbum->id(), 2 ); // the albums should be the already existing one

    // tracks 2 and 3 should just stay
}

void
TestSqlAlbum::testUnsetCompilationWithArtistAFeaturingB()
{
    m_storage->query( "INSERT INTO artists(id,name) VALUES (4,'artist1 feat. artist2');" );

    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (1,'albumAndCompilation1',NULL);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (2,'albumAndCompilation1',1);" );

    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',4,1,1,1,1 );" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (2,2,'track2',1,2,1,1,1 );" );

    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::AlbumPtr compilation = track->album();
    Meta::SqlAlbum *sqlCompilation = static_cast<Meta::SqlAlbum*>( compilation.data() );
    QVERIFY( compilation->isCompilation() );
    sqlCompilation->setCompilation( false );

    Meta::AlbumPtr album;
    Meta::SqlAlbum *sqlAlbum;

    // now the tracks should be in a normal album
    album = track->album();
    sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    QCOMPARE( album->name(), compilation->name() ); //album name did not change
    QCOMPARE( track->artist()->name(), QString( "artist1 feat. artist2" ) );
    QCOMPARE( album->albumArtist()->name(), QString("artist1") ); // artist is the same
    QCOMPARE( sqlAlbum->id(), 2 ); // the albums should be the already existing one
}

void
TestSqlAlbum::testUnsetCompilationWithMultipleArtists()
{
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (1,'album1',NULL);" );

    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (2,2,'track2',2,1,1,1,1 );" );

    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::AlbumPtr compilation = track->album();
    Meta::SqlAlbum *sqlCompilation = static_cast<Meta::SqlAlbum*>( compilation.data() );
    QVERIFY( compilation->isCompilation() );
    sqlCompilation->setCompilation( false );

    Meta::AlbumPtr album;
    Meta::SqlAlbum *sqlAlbum;

    // now the tracks should be in a normal album
    album = track->album();
    sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    QCOMPARE( album->name(), compilation->name() ); //album name did not change
    QCOMPARE( album->albumArtist()->name(), track->artist()->name() ); //artist is the same
    QVERIFY( sqlAlbum->id() != 1 ); // the albums should be a new one

    track = m_registry->getTrack( "/IDoNotExistAsWell.mp3" );
    album = track->album();
    sqlAlbum = static_cast<Meta::SqlAlbum*>( album.data() );
    QVERIFY( album->hasAlbumArtist() );
    QVERIFY( !album->isCompilation() );
    QCOMPARE( album->name(), compilation->name() ); //album name did not change
    QCOMPARE( album->albumArtist()->name(), track->artist()->name() ); //artist is the same
    QVERIFY( sqlAlbum->id() != 1 ); // the albums should be a new one
}


#include "TestSqlAlbum.moc"
