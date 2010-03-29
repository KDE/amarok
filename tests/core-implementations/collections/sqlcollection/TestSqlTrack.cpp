/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#include "TestSqlTrack.h"


#include "DatabaseUpdater.h"
#include "core/support/Debug.h"
#include "DefaultSqlQueryMakerFactory.h"
#include "core/meta/Meta.h"
#include "mysqlecollection/MySqlEmbeddedStorage.h"
#include "SqlCollection.h"
#include "SqlRegistry.h"
#include "SqlMountPointManagerMock.h"

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestSqlTrack )

TestSqlTrack::TestSqlTrack()
    : QObject()
    , m_collection( 0 )
    , m_storage( 0 )
    , m_tmpDir( 0 )
    , m_registry( 0 )
{
}

void
TestSqlTrack::initTestCase()
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
TestSqlTrack::cleanupTestCase()
{
    delete m_collection;
    //m_storage is deleted by SqlCollection
    //m_registry is deleted by SqlCollection
    delete m_tmpDir;
}

void
TestSqlTrack::init()
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
TestSqlTrack::cleanup()
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
TestSqlTrack::testAlbumRemaingsNonCompilationAfterChangingAlbumName()
{
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (1, 'album1', 1);" );

    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (2,2,'track2',1,1,1,1,1 );" );

    Meta::TrackPtr track1 = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::TrackPtr track2 = m_registry->getTrack( "/IDoNotExistAsWell.mp3" );

    QCOMPARE( track1->album()->name(), QString( "album1" ) );
    QVERIFY( track1->album()->hasAlbumArtist() );
    QVERIFY( track1->album() == track2->album() );

    Meta::SqlTrack *sqlTrack1 = static_cast<Meta::SqlTrack*>( track1.data() );
    Meta::SqlTrack *sqlTrack2 = static_cast<Meta::SqlTrack*>( track2.data() );
    sqlTrack1->setAlbum( "album2" );
    sqlTrack2->beginMetaDataUpdate();
    sqlTrack2->setAlbum( "album2" );
    sqlTrack2->endMetaDataUpdate();

    QCOMPARE( track1->album()->name(), QString( "album2" ) );
    QVERIFY( track1->album()->hasAlbumArtist() );
    QVERIFY( track1->album() == track2->album() );
}

void
TestSqlTrack::testAlbumRemainsCompilationAfterChangingAlbumName()
{
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES (1, 'album1', 0);" );

    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );" );
    m_storage->query( "INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (2,2,'track2',1,1,1,1,1 );" );

    Meta::TrackPtr track1 = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::TrackPtr track2 = m_registry->getTrack( "/IDoNotExistAsWell.mp3" );

    QCOMPARE( track1->album()->name(), QString( "album1" ) );
    QVERIFY( track1->album()->isCompilation() );
    QVERIFY( track1->album() == track2->album() );

    Meta::SqlTrack *sqlTrack1 = static_cast<Meta::SqlTrack*>( track1.data() );
    Meta::SqlTrack *sqlTrack2 = static_cast<Meta::SqlTrack*>( track2.data() );
    sqlTrack1->setAlbum( "album2" );
    sqlTrack2->beginMetaDataUpdate();
    sqlTrack2->setAlbum( "album2" );
    sqlTrack2->endMetaDataUpdate();

    QCOMPARE( track1->album()->name(), QString( "album2" ) );
    QVERIFY( track1->album()->isCompilation() );
    QVERIFY( track1->album() == track2->album() );
}

#include "TestSqlTrack.moc"
