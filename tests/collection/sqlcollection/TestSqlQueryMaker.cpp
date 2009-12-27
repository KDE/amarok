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

#include "TestSqlQueryMaker.h"

#include "Debug.h"
#include "meta/TagLibUtils.h"

#include "DatabaseUpdater.h"
#include "SqlCollection.h"
#include "SqlQueryMaker.h"
#include "SqlRegistry.h"
#include "mysqlecollection/MySqlEmbeddedStorage.h"

#include "SqlMountPointManagerMock.h"

#include "QSignalSpy"

//required for Debug.h
QMutex Debug::mutex;

//defined in TagLibUtils.h

namespace TagLib
{
    struct FileRef
    {
        //dummy
    };
}

void
Meta::Field::writeFields(const QString &filename, const QVariantMap &changes )
{
    return;
}

void
Meta::Field::writeFields(TagLib::FileRef fileref, const QVariantMap &changes)
{
    return;
}

TestSqlQueryMaker::TestSqlQueryMaker()
{
}

void
TestSqlQueryMaker::initTestCase()
{
    m_tmpDir = new KTempDir();
    m_storage = new MySqlEmbeddedStorage( m_tmpDir->name() );
    m_collection = new SqlCollection( "testId", "testcollection" );
    m_collection->setSqlStorage( m_storage );
    SqlMountPointManagerMock *mpm = new SqlMountPointManagerMock();
    m_collection->setMountPointManager( mpm );
    SqlRegistry *registry = new SqlRegistry( m_collection );
    registry->setStorage( m_storage );
    m_collection->setRegistry( registry );
    DatabaseUpdater updater;
    updater.setStorage( m_storage );
    updater.setCollection( m_collection );
    updater.update();

    //setup test data
    m_storage->query( "INSERT INTO artists(id, name) VALUES (1, 'artist1');" );
    m_storage->query( "INSERT INTO artists(id, name) VALUES (2, 'artist2');" );
    m_storage->query( "INSERT INTO artists(id, name) VALUES (3, 'artist3');" );

    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(1,'album1',1);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(2,'album2',1);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(3,'album3',2);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(4,'album4',0);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(5,'album4',3);" );

    m_storage->query( "INSERT INTO composers(id, name) VALUES (1, 'composer1');" );
    m_storage->query( "INSERT INTO composers(id, name) VALUES (2, 'composer2');" );
    m_storage->query( "INSERT INTO composers(id, name) VALUES (3, 'composer3');" );

    m_storage->query( "INSERT INTO genres(id, name) VALUES (1, 'genre1');" );
    m_storage->query( "INSERT INTO genres(id, name) VALUES (2, 'genre2');" );
    m_storage->query( "INSERT INTO genres(id, name) VALUES (3, 'genre3');" );

    m_storage->query( "INSERT INTO years(id, name) VALUES (1, '1');" );
    m_storage->query( "INSERT INTO years(id, name) VALUES (2, '2');" );
    m_storage->query( "INSERT INTO years(id, name) VALUES (3, '3');" );

    m_storage->query( "INSERT INTO urls(id,deviceid,rpath,uniqueid) VALUES (1, -1, './IDoNotExist.mp3','1');" );
    m_storage->query( "INSERT INTO urls(id,deviceid,rpath,uniqueid) VALUES (2, -1, './IDoNotExistAsWell.mp3','2');" );
    m_storage->query( "INSERT INTO urls(id,deviceid,rpath,uniqueid) VALUES (3, -1, './MeNeither.mp3','3');" );
    m_storage->query( "INSERT INTO urls(id,deviceid,rpath,uniqueid) VALUES (4, -1, './NothingHere.mp3','4');" );
    m_storage->query( "INSERT INTO urls(id,deviceid,rpath,uniqueid) VALUES (5, -1, './GuessWhat.mp3','5');" );
    m_storage->query( "INSERT INTO urls(id,deviceid,rpath,uniqueid) VALUES (6, -1, './LookItsA.flac','6');" );

    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(1,1,'track1','comment1',1,1,1,1,1);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(2,2,'track2','comment2',1,2,1,1,1);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(3,3,'track3','comment3',3,4,1,1,1);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(4,4,'track4','comment4',2,3,3,3,3);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(5,5,'track5','',3,5,2,2,2);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(6,6,'track6','',1,4,2,2,2);" );

}

void
TestSqlQueryMaker::cleanupTestCase()
{
    delete m_collection;
    //m_storage is deleted by SqlCollection
    //m_registry is deleted by SqlCollection
    delete m_tmpDir;

}

void
TestSqlQueryMaker::testQueryAlbums()
{
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setAlbumQueryMode( QueryMaker::AllAlbums );
    qm.setQueryType( QueryMaker::Album );
    qm.run();
    QCOMPARE( qm.albums( "testId" ).count(), 5 );
}

void
TestSqlQueryMaker::testQueryArtists()
{
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Artist );
    qm.run();
    QCOMPARE( qm.artists( "testId" ).count(), 3 );
}

void
TestSqlQueryMaker::testQueryComposers()
{
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Composer );
    qm.run();
    QCOMPARE( qm.composers( "testId" ).count(), 3 );
}

void
TestSqlQueryMaker::testQueryGenres()
{
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Genre );
    qm.run();
    QCOMPARE( qm.genres( "testId" ).count(), 3 );
}

void
TestSqlQueryMaker::testQueryYears()
{
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Year );
    qm.run();
    QCOMPARE( qm.years( "testId" ).count(), 3 );
}

void
TestSqlQueryMaker::testQueryTracks()
{
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Track );
    qm.run();
    QCOMPARE( qm.tracks( "testId" ).count(), 6 );
}

void
TestSqlQueryMaker::testAlbumQueryMode()
{
    SqlQueryMaker qm( m_collection );

    qm.setBlocking( true );
    qm.setAlbumQueryMode( QueryMaker::OnlyCompilations );
    qm.setQueryType( QueryMaker::Album );
    qm.run();
    QCOMPARE( qm.albums( "testId" ).count(), 1 );

    qm.reset();
    qm.setBlocking( true );
    qm.setAlbumQueryMode( QueryMaker::OnlyNormalAlbums );
    qm.setQueryType( QueryMaker::Album );
    qm.run();
    QCOMPARE( qm.albums( "testId" ).count(), 4 );

    qm.reset();
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Track );
    qm.setAlbumQueryMode( QueryMaker::OnlyCompilations );
    qm.run();
    QCOMPARE( qm.tracks( "testId" ).count(), 2 );

    qm.reset();
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Track );
    qm.setAlbumQueryMode( QueryMaker::OnlyNormalAlbums );
    qm.run();
    QCOMPARE( qm.tracks( "testId" ).count(), 4 );

    qm.reset();
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Artist );
    qm.setAlbumQueryMode( QueryMaker::OnlyCompilations );
    qm.run();
    QCOMPARE( qm.artists( "testId" ).count() , 2 );

    qm.reset();
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Artist );
    qm.setAlbumQueryMode( QueryMaker::OnlyNormalAlbums );
    qm.run();
    QCOMPARE( qm.artists( "testId" ).count(), 3 );

    qm.reset();
    qm.setBlocking( true );
    qm.setAlbumQueryMode( QueryMaker::OnlyCompilations );
    qm.setQueryType( QueryMaker::Genre );
    qm.run();
    QCOMPARE( qm.genres( "testId" ).count(), 2 );

    qm.reset();
    qm.setBlocking( true );
    qm.setAlbumQueryMode( QueryMaker::OnlyNormalAlbums );
    qm.setQueryType( QueryMaker::Genre );
    qm.run();
    QCOMPARE( qm.genres( "testId" ).count(), 2 );

}

void
TestSqlQueryMaker::testDeleteQueryMakerWithRunningQuery()
{    
    int iteration = 1;
    bool queryNotDoneYet = true;

    //wait one second per query in total, that should be enough for it to complete
    do
    {
        SqlQueryMaker *qm = new SqlQueryMaker( m_collection );
        QSignalSpy spy( qm, SIGNAL(queryDone()) );
        qm->setQueryType( QueryMaker::Track );
        qm->run();
        //wait 20 msec more per iteration, might have to be tweaked
        if( iteration > 1 )
        {
            QTest::qWait( 20 * iteration - 20 );
        }
        delete qm;
        queryNotDoneYet = ( spy.count() == 0 );
        if( iteration > 50 )
        {
            break;
        }
        QTest::qWait( 1000 + 20 - 20 *iteration );
        iteration++;
    } while ( queryNotDoneYet );
}

#include "TestSqlQueryMaker.moc"
