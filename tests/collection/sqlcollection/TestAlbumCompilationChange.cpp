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
#include "Debug.h"
#include "meta/Meta.h"
#include "mysqlecollection/MySqlEmbeddedStorage.h"
#include "SqlCollection.h"
#include "SqlRegistry.h"
#include "SqlMountPointManagerMock.h"

//required for Debug.h
QMutex Debug::mutex;

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
    m_collection = new SqlCollection( "testId", "testcollection" );
    m_collection->setSqlStorage( m_storage );
    m_collection->setMountPointManager( new SqlMountPointManagerMock() );
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

    m_storage->query( "INSERT INTO composers(id, name) VALUES (1, 'composer1');" );
    m_storage->query( "INSERT INTO genres(id, name) VALUES (1, 'genre1');" );
    m_storage->query( "INSERT INTO years(id, name) VALUES (1, '1');" );

    m_storage->query( "INSERT INTO urls(id, deviceid, rpath) VALUES (1, -1, './IDoNotExist.mp3');" );
    m_storage->query( "INSERT INTO urls(id, deviceid, rpath) VALUES (2, -1, './IDoNotExistAsWell.mp3');" );
    m_storage->query( "INSERT INTO urls(id, deviceid, rpath) VALUES (3, -1, './MeNeither.mp3');" );


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

    QStringList trackResult = m_storage->query("SELECT artist FROM tracks WHERE id = 1");
    QCOMPARE( trackResult.count(), 1 );
    QCOMPARE( trackResult.first(), QString( "1" ) ); //track still points at the same row in albums

    QStringList albumResult = m_storage->query( "SELECT name, artist FROM albums WHERE id = 1" );
    QCOMPARE( albumResult.count(), 2 );
    QCOMPARE( albumResult.first(), QString( "album1" ) ); //album name did not change
    QCOMPARE( albumResult.at( 1 ), QString() ); //artist should be <null>, which is an empty string
    QCOMPARE( track->album(), album ); //track still returns the same album object
}

#include "TestAlbumCompilationChange.moc"
