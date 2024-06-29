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

#include "core/support/Debug.h"

#include "DatabaseUpdater.h"
#include "SqlCollection.h"
#include "SqlQueryMaker.h"
#include "SqlRegistry.h"
#include "core-impl/storage/sql/mysqlestorage/MySqlEmbeddedStorage.h"

#include "SqlMountPointManagerMock.h"

#include <QSignalSpy>


using namespace Collections;

QTEST_GUILESS_MAIN( TestSqlQueryMaker )

QTemporaryDir *TestSqlQueryMaker::s_tmpDir = nullptr;

//required for QTest, this is not done in Querymaker.h
Q_DECLARE_METATYPE( Collections::QueryMaker::QueryType )
Q_DECLARE_METATYPE( Collections::QueryMaker::NumberComparison )
Q_DECLARE_METATYPE( Collections::QueryMaker::ReturnFunction )
Q_DECLARE_METATYPE( Collections::QueryMaker::AlbumQueryMode )
Q_DECLARE_METATYPE( Collections::QueryMaker::LabelQueryMode )

TestSqlQueryMaker::TestSqlQueryMaker()
{
    std::atexit([]() { delete TestSqlQueryMaker::s_tmpDir; } );
    qRegisterMetaType<Meta::TrackPtr>();
    qRegisterMetaType<Meta::TrackList>();
    qRegisterMetaType<Meta::AlbumPtr>();
    qRegisterMetaType<Meta::AlbumList>();
    qRegisterMetaType<Meta::ArtistPtr>();
    qRegisterMetaType<Meta::ArtistList>();
    qRegisterMetaType<Meta::GenrePtr>();
    qRegisterMetaType<Meta::GenreList>();
    qRegisterMetaType<Meta::ComposerPtr>();
    qRegisterMetaType<Meta::ComposerList>();
    qRegisterMetaType<Meta::YearPtr>();
    qRegisterMetaType<Meta::YearList>();
    qRegisterMetaType<Meta::LabelPtr>();
    qRegisterMetaType<Meta::LabelList>();
    qRegisterMetaType<Collections::QueryMaker::QueryType>();
    qRegisterMetaType<Collections::QueryMaker::NumberComparison>();
    qRegisterMetaType<Collections::QueryMaker::ReturnFunction>();
    qRegisterMetaType<Collections::QueryMaker::AlbumQueryMode>();
    qRegisterMetaType<Collections::QueryMaker::LabelQueryMode>();
}

void
TestSqlQueryMaker::initTestCase()
{
    if( !s_tmpDir )
        s_tmpDir = new QTemporaryDir();
    m_storage = QSharedPointer<MySqlEmbeddedStorage>( new MySqlEmbeddedStorage() );
    QVERIFY( m_storage->init( s_tmpDir->path() ) );
    m_collection = new Collections::SqlCollection( m_storage );

    QMap<int,QString> mountPoints;
    mountPoints.insert( 1, QStringLiteral("/foo") );
    mountPoints.insert( 2, QStringLiteral("/bar") );

    m_mpm = new SqlMountPointManagerMock( this, m_storage );
    m_mpm->m_mountPoints = mountPoints;

    m_collection->setMountPointManager( m_mpm );

    //setup test data
    m_storage->query( QStringLiteral("INSERT INTO artists(id, name) VALUES (1, 'artist1');") );
    m_storage->query( QStringLiteral("INSERT INTO artists(id, name) VALUES (2, 'artist2');") );
    m_storage->query( QStringLiteral("INSERT INTO artists(id, name) VALUES (3, 'artist3');") );

    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES(1,'album1',1);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES(2,'album2',1);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES(3,'album3',2);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES(4,'album4',NULL);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES(5,'album4',3);") );

    m_storage->query( QStringLiteral("INSERT INTO composers(id, name) VALUES (1, 'composer1');") );
    m_storage->query( QStringLiteral("INSERT INTO composers(id, name) VALUES (2, 'composer2');") );
    m_storage->query( QStringLiteral("INSERT INTO composers(id, name) VALUES (3, 'composer3');") );

    m_storage->query( QStringLiteral("INSERT INTO genres(id, name) VALUES (1, 'genre1');") );
    m_storage->query( QStringLiteral("INSERT INTO genres(id, name) VALUES (2, 'genre2');") );
    m_storage->query( QStringLiteral("INSERT INTO genres(id, name) VALUES (3, 'genre3');") );

    m_storage->query( QStringLiteral("INSERT INTO years(id, name) VALUES (1, '1');") );
    m_storage->query( QStringLiteral("INSERT INTO years(id, name) VALUES (2, '2');") );
    m_storage->query( QStringLiteral("INSERT INTO years(id, name) VALUES (3, '3');") );

    m_storage->query( QStringLiteral("INSERT INTO directories(id, deviceid, dir) VALUES (1, -1, './');") );

    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES (1, -1, './IDoNotExist.mp3', 1, '1');") );
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES (2, -1, './IDoNotExistAsWell.mp3', 1, '2');") );
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES (3, -1, './MeNeither.mp3', 1, '3');") );
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES (4, 2, './NothingHere.mp3', 1, '4');") );
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES (5, 1, './GuessWhat.mp3', 1, '5');") );
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES (6, 2, './LookItsA.flac', 1, '6');") );

    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(1,1,'track1','comment1',1,1,1,1,1);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(2,2,'track2','comment2',1,2,1,1,1);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(3,3,'track3','comment3',3,4,1,1,1);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(4,4,'track4','comment4',2,3,3,3,3);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(5,5,'track5','',3,5,2,2,2);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(6,6,'track6','',1,4,2,2,2);") );

    m_storage->query( QStringLiteral("INSERT INTO statistics(url,createdate,accessdate,score,rating,playcount) "
                      "VALUES(1,1000,10000, 50.0,2,100);") );
    m_storage->query( QStringLiteral("INSERT INTO statistics(url,createdate,accessdate,score,rating,playcount) "
                      "VALUES(2,2000,30000, 70.0,9,50);") );
    m_storage->query( QStringLiteral("INSERT INTO statistics(url,createdate,accessdate,score,rating,playcount) "
                      "VALUES(3,4000,20000, 60.0,4,10);") );

    m_storage->query( QStringLiteral("INSERT INTO labels(id,label) VALUES (1,'labelA'), (2,'labelB'),(3,'test');") );
    m_storage->query( QStringLiteral("INSERT INTO urls_labels(url,label) VALUES (1,1),(1,2),(2,2),(3,3),(4,3),(4,2);") );

}

void
TestSqlQueryMaker::cleanupTestCase()
{
    delete m_collection;
    //m_storage is deleted by SqlCollection

}

void
TestSqlQueryMaker::cleanup()
{
    m_collection->setMountPointManager( m_mpm );
}

void
TestSqlQueryMaker::testQueryAlbums()
{
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setAlbumQueryMode( Collections::QueryMaker::AllAlbums );
    qm.setQueryType( Collections::QueryMaker::Album );
    qm.run();
    QCOMPARE( qm.albums().count(), 5 );
}

void
TestSqlQueryMaker::testQueryArtists()
{
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Artist );
    qm.run();
    QCOMPARE( qm.artists().count(), 3 );
}

void
TestSqlQueryMaker::testQueryComposers()
{
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Composer );
    qm.run();
    QCOMPARE( qm.composers().count(), 3 );
}

void
TestSqlQueryMaker::testQueryGenres()
{
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Genre );
    qm.run();
    QCOMPARE( qm.genres().count(), 3 );
}

void
TestSqlQueryMaker::testQueryYears()
{
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Year );
    qm.run();
    QCOMPARE( qm.years().count(), 3 );
}

void
TestSqlQueryMaker::testQueryTracks()
{
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Track );
    qm.run();
    QCOMPARE( qm.tracks().count(), 6 );
}

void
TestSqlQueryMaker::testAlbumQueryMode()
{
    {
        Collections::SqlQueryMaker qm( m_collection );
        qm.setBlocking( true );
        qm.setAlbumQueryMode( Collections::QueryMaker::OnlyCompilations );
        qm.setQueryType( Collections::QueryMaker::Album );
        qm.run();
        QCOMPARE( qm.albums().count(), 1 );
    }

    {
        Collections::SqlQueryMaker qm( m_collection );
        qm.setBlocking( true );
        qm.setAlbumQueryMode( Collections::QueryMaker::OnlyNormalAlbums );
        qm.setQueryType( Collections::QueryMaker::Album );
        qm.run();
        QCOMPARE( qm.albums().count(), 4 );
    }

    {
        Collections::SqlQueryMaker qm( m_collection );
        qm.setBlocking( true );
        qm.setQueryType( Collections::QueryMaker::Track );
        qm.setAlbumQueryMode( Collections::QueryMaker::OnlyCompilations );
        qm.run();
        QCOMPARE( qm.tracks().count(), 2 );
    }

    {
        Collections::SqlQueryMaker qm( m_collection );
        qm.setBlocking( true );
        qm.setQueryType( Collections::QueryMaker::Track );
        qm.setAlbumQueryMode( Collections::QueryMaker::OnlyNormalAlbums );
        qm.run();
        QCOMPARE( qm.tracks().count(), 4 );
    }

    {
        Collections::SqlQueryMaker qm( m_collection );
        qm.setBlocking( true );
        qm.setQueryType( Collections::QueryMaker::Artist );
        qm.setAlbumQueryMode( Collections::QueryMaker::OnlyCompilations );
        qm.run();
        QCOMPARE( qm.artists().count() , 2 );
    }

    {
        Collections::SqlQueryMaker qm( m_collection );
        qm.setBlocking( true );
        qm.setQueryType( Collections::QueryMaker::Artist );
        qm.setAlbumQueryMode( Collections::QueryMaker::OnlyNormalAlbums );
        qm.run();
        QCOMPARE( qm.artists().count(), 3 );
    }

    {
        Collections::SqlQueryMaker qm( m_collection );
        qm.setBlocking( true );
        qm.setAlbumQueryMode( Collections::QueryMaker::OnlyCompilations );
        qm.setQueryType( Collections::QueryMaker::Genre );
        qm.run();
        QCOMPARE( qm.genres().count(), 2 );
    }

    {
        Collections::SqlQueryMaker qm( m_collection );
        qm.setBlocking( true );
        qm.setAlbumQueryMode( Collections::QueryMaker::OnlyNormalAlbums );
        qm.setQueryType( Collections::QueryMaker::Genre );
        qm.run();
        QCOMPARE( qm.genres().count(), 3 );
    }

}

void
TestSqlQueryMaker::testDeleteQueryMakerWithRunningQuery()
{
    int iteration = 0;
    bool queryNotDoneYet = true;

    //wait one second per query in total, that should be enough for it to complete
    do
    {
        Collections::SqlQueryMaker *qm = new Collections::SqlQueryMaker( m_collection );
        QSignalSpy spy( qm, &Collections::QueryMaker::queryDone );
        qm->setQueryType( Collections::QueryMaker::Track );
        qm->addFilter( Meta::valTitle, QString::number( iteration), false, false );
        qm->run();
        //wait 10 msec more per iteration, might have to be tweaked
        if( iteration > 0 )
        {
            QTest::qWait( 10 * iteration );
        }
        delete qm;
        queryNotDoneYet = ( spy.count() == 0 );
        if( iteration > 50 )
        {
            break;
        }
        iteration++;
    } while ( queryNotDoneYet );
    qDebug() << "Iterations: " << iteration;
}

void
TestSqlQueryMaker::testAsyncAlbumQuery()
{
    Collections::QueryMaker *qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Album );
    QSignalSpy doneSpy1( qm, &Collections::QueryMaker::queryDone );
    QSignalSpy resultSpy1( qm, &Collections::QueryMaker::newAlbumsReady );

    qm->run();

    doneSpy1.wait( 1000 );

    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(0).canConvert<Meta::AlbumList>() );
    QCOMPARE( args1.value(0).value<Meta::AlbumList>().count(), 5 );
    QCOMPARE( doneSpy1.count(), 1);
    delete qm;

    qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Album );
    QSignalSpy doneSpy2( qm, &Collections::QueryMaker::queryDone );
    QSignalSpy resultSpy2( qm, &Collections::QueryMaker::newAlbumsReady );
    qm->addFilter( Meta::valAlbum, QStringLiteral("foo") ); //should result in no match

    qm->run();

    doneSpy2.wait( 1000 );

    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(0).canConvert<Meta::AlbumList>() );
    QCOMPARE( args2.value(0).value<Meta::AlbumList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncArtistQuery()
{
    Collections::QueryMaker *qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Artist );
    QSignalSpy doneSpy1( qm, &Collections::QueryMaker::queryDone );
    QSignalSpy resultSpy1( qm, &Collections::QueryMaker::newArtistsReady );

    qm->run();

    doneSpy1.wait( 1000 );

    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(0).canConvert<Meta::ArtistList>() );
    QCOMPARE( args1.value(0).value<Meta::ArtistList>().count(), 3 );
    QCOMPARE( doneSpy1.count(), 1);
    delete qm;

    qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Artist );
    QSignalSpy doneSpy2( qm, &Collections::QueryMaker::queryDone );
    QSignalSpy resultSpy2( qm, &Collections::QueryMaker::newArtistsReady );
    qm->addFilter( Meta::valArtist, QStringLiteral("foo") ); //should result in no match

    qm->run();

    doneSpy2.wait( 1000 );

    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(0).canConvert<Meta::ArtistList>() );
    QCOMPARE( args2.value(0).value<Meta::ArtistList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncComposerQuery()
{
    Collections::QueryMaker *qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Composer );
    QSignalSpy doneSpy1( qm, &Collections::QueryMaker::queryDone );
    QSignalSpy resultSpy1( qm, &Collections::QueryMaker::newComposersReady );

    qm->run();

    doneSpy1.wait( 1000 );

    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(0).canConvert<Meta::ComposerList>() );
    QCOMPARE( args1.value(0).value<Meta::ComposerList>().count(), 3 );
    QCOMPARE( doneSpy1.count(), 1);

    delete qm;

    qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Composer );
    QSignalSpy doneSpy2( qm, &Collections::QueryMaker::queryDone );
    QSignalSpy resultSpy2( qm, &Collections::QueryMaker::newComposersReady );
    qm->addFilter( Meta::valComposer, QStringLiteral("foo") ); //should result in no match

    qm->run();

    doneSpy2.wait( 1000 );

    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(0).canConvert<Meta::ComposerList>() );
    QCOMPARE( args2.value(0).value<Meta::ComposerList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncTrackQuery()
{
    Collections::QueryMaker *qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Track );
    QSignalSpy doneSpy1( qm, &Collections::QueryMaker::queryDone );
    QSignalSpy resultSpy1( qm, &Collections::QueryMaker::newTracksReady );

    qm->run();

    doneSpy1.wait( 1000 );

    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(0).canConvert<Meta::TrackList>() );
    QCOMPARE( args1.value(0).value<Meta::TrackList>().count(), 6 );
    QCOMPARE( doneSpy1.count(), 1);

    delete qm;

    qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Track );
    QSignalSpy doneSpy2( qm, &Collections::QueryMaker::queryDone );
    QSignalSpy resultSpy2( qm, &Collections::QueryMaker::newTracksReady );
    qm->addFilter( Meta::valTitle, QStringLiteral("foo") ); //should result in no match

    qm->run();

    doneSpy2.wait( 1000 );

    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(0).canConvert<Meta::TrackList>() );
    QCOMPARE( args2.value(0).value<Meta::TrackList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncGenreQuery()
{
    Collections::QueryMaker *qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Genre );
    QSignalSpy doneSpy1( qm, &Collections::QueryMaker::queryDone );
    QSignalSpy resultSpy1( qm, &Collections::QueryMaker::newGenresReady );

    qm->run();

    doneSpy1.wait( 1000 );

    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(0).canConvert<Meta::GenreList>() );
    QCOMPARE( args1.value(0).value<Meta::GenreList>().count(), 3 );
    QCOMPARE( doneSpy1.count(), 1);

    delete qm;

    qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Genre );
    QSignalSpy doneSpy2( qm, &Collections::QueryMaker::queryDone );
    QSignalSpy resultSpy2( qm, &Collections::QueryMaker::newGenresReady );
    qm->addFilter( Meta::valGenre, QStringLiteral("foo") ); //should result in no match

    qm->run();

    doneSpy2.wait( 1000 );

    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(0).canConvert<Meta::GenreList>() );
    QCOMPARE( args2.value(0).value<Meta::GenreList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncYearQuery()
{
    Collections::QueryMaker *qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Year );
    QSignalSpy doneSpy1( qm, &Collections::QueryMaker::queryDone );
    QSignalSpy resultSpy1( qm, &Collections::QueryMaker::newYearsReady );

    qm->run();

    doneSpy1.wait( 1000 );

    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(0).canConvert<Meta::YearList>() );
    QCOMPARE( args1.value(0).value<Meta::YearList>().count(), 3 );
    QCOMPARE( doneSpy1.count(), 1);

    delete qm;

    qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Year );
    QSignalSpy doneSpy2( qm, &Collections::QueryMaker::queryDone );
    QSignalSpy resultSpy2( qm, &Collections::QueryMaker::newYearsReady );
    qm->addFilter( Meta::valYear, QStringLiteral("foo") ); //should result in no match

    qm->run();

    doneSpy2.wait( 1000 );

    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(0).canConvert<Meta::YearList>() );
    QCOMPARE( args2.value(0).value<Meta::YearList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncCustomQuery()
{
    Collections::QueryMaker *qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Custom );
    qm->addReturnFunction( Collections::QueryMaker::Count, Meta::valTitle );
    QSignalSpy doneSpy1( qm, &Collections::QueryMaker::queryDone );
    QSignalSpy resultSpy1( qm, &Collections::QueryMaker::newResultReady );

    qm->run();

    doneSpy1.wait( 1000 );

    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(0).canConvert<QStringList>() );
    QCOMPARE( args1.value(0).value<QStringList>().count(), 1 );
    QCOMPARE( args1.value(0).value<QStringList>().first(), QStringLiteral( "6" ) );
    QCOMPARE( doneSpy1.count(), 1);

    delete qm;

    qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Custom );
    qm->addReturnFunction( Collections::QueryMaker::Count, Meta::valTitle );
    QSignalSpy doneSpy2( qm, &Collections::QueryMaker::queryDone );
    QSignalSpy resultSpy2( qm, &Collections::QueryMaker::newResultReady );
    qm->addFilter( Meta::valTitle, QStringLiteral("foo") ); //should result in no match

    qm->run();

    doneSpy2.wait( 1000 );

    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(0).canConvert<QStringList>() );
    QCOMPARE( args2.value(0).value<QStringList>().count(), 1 );
    QCOMPARE( args2.value(0).value<QStringList>().first(), QStringLiteral( "0" ) );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testFilter_data()
{
    QTest::addColumn<Collections::QueryMaker::QueryType>( "type" );
    QTest::addColumn<qint64>( "value" );
    QTest::addColumn<QString>( "filter" );
    QTest::addColumn<bool>( "matchBeginning" );
    QTest::addColumn<bool>( "matchEnd" );
    QTest::addColumn<int>( "count" );

    QTest::newRow( "track match all titles" ) << Collections::QueryMaker::Track << Meta::valTitle << "track" << false << false << 6;
    QTest::newRow( "track match all title beginnings" ) << Collections::QueryMaker::Track << Meta::valTitle << "track" << true << false << 6;
    QTest::newRow( "track match one title beginning" ) << Collections::QueryMaker::Track << Meta::valTitle << "track1" << true << false << 1;
    QTest::newRow( "track match one title end" ) << Collections::QueryMaker::Track << Meta::valTitle << "rack2" << false << true << 1;
    QTest::newRow( "track match title on both ends" ) << Collections::QueryMaker::Track << Meta::valTitle << "track3" << true << true << 1;
    QTest::newRow( "track match artist" ) << Collections::QueryMaker::Track << Meta::valArtist << "artist1" << false << false << 3;
    QTest::newRow( "artist match artist" ) << Collections::QueryMaker::Artist << Meta::valArtist << "artist1" << true << true << 1;
    QTest::newRow( "album match artist" ) << Collections::QueryMaker::Album << Meta::valArtist << "artist3" << false << false << 2;
    QTest::newRow( "track match genre" ) << Collections::QueryMaker::Track << Meta::valGenre << "genre1" << false << false << 3;
    QTest::newRow( "genre match genre" ) << Collections::QueryMaker::Genre << Meta::valGenre << "genre1" << false << false << 1;
    QTest::newRow( "track match composer" ) << Collections::QueryMaker::Track << Meta::valComposer << "composer2" << false << false << 2;
    QTest::newRow( "composer match composer" ) << Collections::QueryMaker::Composer << Meta::valComposer << "composer2" << false << false << 1;
    QTest::newRow( "track match year" ) << Collections::QueryMaker::Track << Meta::valYear << "2" << true << true << 2;
    QTest::newRow( "year match year" ) << Collections::QueryMaker::Year << Meta::valYear << "1" << false << false << 1;
    QTest::newRow( "album match album" ) << Collections::QueryMaker::Album << Meta::valAlbum << "album1" << false << false << 1;
    QTest::newRow( "track match album" ) << Collections::QueryMaker::Track << Meta::valAlbum << "album1" << false << false << 1;
    QTest::newRow( "track match albumartit" ) << Collections::QueryMaker::Track << Meta::valAlbumArtist << "artist1" << false << false << 2;
    QTest::newRow( "album match albumartist" ) << Collections::QueryMaker::Album << Meta::valAlbumArtist << "artist2" << false << false << 1;
    QTest::newRow( "album match all albumartists" ) << Collections::QueryMaker::Album << Meta::valAlbumArtist << "artist" << true << false << 4;
    QTest::newRow( "genre match albumartist" ) << Collections::QueryMaker::Genre << Meta::valAlbumArtist << "artist1" << false << false << 1;
    QTest::newRow( "year match albumartist" ) << Collections::QueryMaker::Year << Meta::valAlbumArtist << "artist1" << false << false << 1;
    QTest::newRow( "composer match albumartist" ) << Collections::QueryMaker::Composer << Meta::valAlbumArtist << "artist1" << false << false << 1;
    QTest::newRow( "genre match title" ) << Collections::QueryMaker::Genre << Meta::valTitle << "track1" << false << false << 1;
    QTest::newRow( "composer match title" ) << Collections::QueryMaker::Composer << Meta::valTitle << "track1" << false << false << 1;
    QTest::newRow( "year match title" ) << Collections::QueryMaker::Year << Meta::valTitle << "track1" << false << false << 1;
    QTest::newRow( "album match title" ) << Collections::QueryMaker::Album << Meta::valTitle << "track1" << false << false << 1;
    QTest::newRow( "artist match title" ) << Collections::QueryMaker::Artist << Meta::valTitle << "track1" << false << false << 1;
    QTest::newRow( "track match comment" ) << Collections::QueryMaker::Track << Meta::valComment << "comment" << true << false << 4;
    QTest::newRow( "track match url" ) << Collections::QueryMaker::Track << Meta::valUrl << "Exist" << false << false << 2;
    QTest::newRow( "album match comment" ) << Collections::QueryMaker::Track << Meta::valComment << "comment1" << true << true << 1;
}

void
TestSqlQueryMaker::checkResultCount( Collections::SqlQueryMaker* qm,
                                     Collections::QueryMaker::QueryType type, int count ) {
    switch( type ) {
    case QueryMaker::Track: QCOMPARE( qm->tracks().count(), count ); break;
    case QueryMaker::Artist: QCOMPARE( qm->artists().count(), count ); break;
    case QueryMaker::Album: QCOMPARE( qm->albums().count(), count ); break;
    case QueryMaker::Genre: QCOMPARE( qm->genres().count(), count ); break;
    case QueryMaker::Composer: QCOMPARE( qm->composers().count(), count ); break;
    case QueryMaker::Year: QCOMPARE( qm->years().count(), count ); break;
    case QueryMaker::Label: QCOMPARE( qm->labels().count(), count ); break;
    default:
        ; // do nothing
    }
}

void
TestSqlQueryMaker::testFilter()
{
    QFETCH( Collections::QueryMaker::QueryType, type );
    QFETCH( qint64, value );
    QFETCH( QString, filter );
    QFETCH( bool, matchBeginning );
    QFETCH( bool, matchEnd );
    QFETCH( int, count );

    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( type );

    qm.addFilter( value, filter, matchBeginning, matchEnd );

    qm.run();

    checkResultCount( &qm, type, count );
}

void
TestSqlQueryMaker::testDynamicCollection()
{
    //this will not crash as we reset the correct mock in cleanup()
    SqlMountPointManagerMock mpm( this, m_storage );

    QMap<int, QString> mountPoints;

    mpm.m_mountPoints = mountPoints;

    m_collection->setMountPointManager( &mpm );

    Collections::SqlQueryMaker trackQm( m_collection );
    trackQm.setQueryType( Collections::QueryMaker::Track );
    trackQm.setBlocking( true );
    trackQm.run();
    QCOMPARE( trackQm.tracks().count(), 3 );

    mpm.m_mountPoints.insert( 1, QStringLiteral("/foo") );

    Collections::SqlQueryMaker trackQm2( m_collection );
    trackQm2.setQueryType( Collections::QueryMaker::Track );
    trackQm2.setBlocking( true );
    trackQm2.run();
    QCOMPARE( trackQm2.tracks().count(), 4 );

    Collections::SqlQueryMaker artistQm( m_collection );
    artistQm.setQueryType( Collections::QueryMaker::Artist );
    artistQm.setBlocking( true );
    artistQm.run();
    QCOMPARE( artistQm.artists().count(), 2 );

    Collections::SqlQueryMaker albumQm( m_collection );
    albumQm.setQueryType( Collections::QueryMaker::Album );
    albumQm.setBlocking( true );
    albumQm.run();
    QCOMPARE( albumQm.albums().count(), 4 );

    Collections::SqlQueryMaker genreQm( m_collection );
    genreQm.setQueryType( Collections::QueryMaker::Genre );
    genreQm.setBlocking( true );
    genreQm.run();
    QCOMPARE( genreQm.genres().count(), 2 );

    Collections::SqlQueryMaker composerQm( m_collection );
    composerQm.setQueryType( Collections::QueryMaker::Composer );
    composerQm.setBlocking( true );
    composerQm.run();
    QCOMPARE( composerQm.composers().count(), 2 );

    Collections::SqlQueryMaker yearQm( m_collection );
    yearQm.setQueryType( Collections::QueryMaker::Year );
    yearQm.setBlocking( true );
    yearQm.run();
    QCOMPARE( yearQm.years().count(), 2 );

}

void
TestSqlQueryMaker::testSpecialCharacters_data()
{
    QTest::addColumn<QString>( "filter" );
    QTest::addColumn<bool>( "like" );

    QTest::newRow( "slash in filter w/o like" ) << "AC/DC" << false;
    QTest::newRow( "slash in filter w/ like" ) << "AC/DC" << true;
    QTest::newRow( "backslash in filter w/o like" ) << "AC\\DC" << false;
    QTest::newRow( "backslash in filter w like" ) << "AC\\DC" << true;
    QTest::newRow( "quote in filter w/o like" ) << "Foo'Bar" << false;
    QTest::newRow( "quote in filter w like" ) << "Foo'Bar" << true;
    QTest::newRow( "% in filter w/o like" ) << "Foo%Bar" << false;
    QTest::newRow( "% in filter w/ like" ) << "Foo%Bar"  << true;
    QTest::newRow( "filter ending with % w/o like" ) << "Foo%" << false;
    QTest::newRow( "filter ending with % w like" ) << "Foo%" << true;
    QTest::newRow( "filter beginning with % w/o like" ) << "%Foo" << false;
    QTest::newRow( "filter beginning with % w/o like" ) << "%Foo" << true;
    QTest::newRow( "\" in filter w/o like" ) << "Foo\"Bar" << false;
    QTest::newRow( "\" in filter w like" ) << "Foo\"Bar" << true;
    QTest::newRow( "_ in filter w/o like" ) << "track_" << false;
    QTest::newRow( "_ in filter w/ like" ) << "track_" << true;
    QTest::newRow( "filter with two consecutive backslashes w/o like" ) << "Foo\\\\Bar" << false;
    QTest::newRow( "filter with two consecutive backslashes w like" ) << "Foo\\\\Bar" << true;
    QTest::newRow( "filter with backslash% w/o like" ) << "FooBar\\%" << false;
    QTest::newRow( "filter with backslash% w like" ) << "FooBar\\%" << true;
}

void
TestSqlQueryMaker::testSpecialCharacters()
{
    QFETCH( QString, filter );
    QFETCH( bool, like );

    QString insertTrack = QStringLiteral( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                              "VALUES(999,999,'%1','',1,1,1,1,1);").arg( m_storage->escape( filter ) );

    //there is a unique index on TRACKS.URL
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES(999, -1, './foobar.mp3', 1, '999');") );
    m_storage->query( insertTrack );

    QCOMPARE( m_storage->query( QStringLiteral("select count(*) from urls where id = 999") ).first(), QStringLiteral("1") );
    QCOMPARE( m_storage->query( QStringLiteral("select count(*) from tracks where id = 999") ).first(), QStringLiteral("1") );

    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Track );
    qm.addFilter( Meta::valTitle, filter, !like, !like );

    qm.run();

    m_storage->query( QStringLiteral("DELETE FROM urls WHERE id = 999;") );
    m_storage->query( QStringLiteral("DELETE FROM tracks WHERE id = 999;") );

    QCOMPARE( qm.tracks().count(), 1 );
}

void
TestSqlQueryMaker::testNumberFilter_data()
{
    QTest::addColumn<Collections::QueryMaker::QueryType>( "type" );
    QTest::addColumn<qint64>( "value" );
    QTest::addColumn<int>( "filter" );
    QTest::addColumn<Collections::QueryMaker::NumberComparison>( "comparison" );
    QTest::addColumn<bool>( "exclude" );
    QTest::addColumn<int>( "count" );

    QTest::newRow( "include rating greater 4" ) << Collections::QueryMaker::Track << Meta::valRating << 4 << Collections::QueryMaker::GreaterThan << false << 1;
    QTest::newRow( "exclude rating smaller 4" ) << Collections::QueryMaker::Album << Meta::valRating << 4 << Collections::QueryMaker::LessThan << true << 4;
    QTest::newRow( "exclude tracks first played later than 2000" ) << Collections::QueryMaker::Track << Meta::valFirstPlayed << 2000 << Collections::QueryMaker::GreaterThan << true << 5;
    //having never been played does not mean played before 20000
    QTest::newRow( "include last played before 20000" ) << Collections::QueryMaker::Track << Meta::valLastPlayed << 20000 << Collections::QueryMaker::LessThan << false << 1;
    QTest::newRow( "playcount equals 100" ) << Collections::QueryMaker::Album << Meta::valPlaycount << 100 << Collections::QueryMaker::Equals << false << 1;
    //should include unplayed songs
    QTest::newRow( "playcount != 50" ) << Collections::QueryMaker::Track << Meta::valPlaycount << 50 << Collections::QueryMaker::Equals << true << 5;
    QTest::newRow( "score greater 60" ) << Collections::QueryMaker::Genre << Meta::valScore << 60 << Collections::QueryMaker::GreaterThan << false << 1;
}

void
TestSqlQueryMaker::testNumberFilter()
{

    QFETCH( Collections::QueryMaker::QueryType, type );
    QFETCH( qint64, value );
    QFETCH( int, filter );
    QFETCH( bool, exclude );
    QFETCH( Collections::QueryMaker::NumberComparison, comparison );
    QFETCH( int, count );

    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( type );

    if( exclude )
        qm.excludeNumberFilter( value, filter, comparison );
    else
        qm.addNumberFilter( value, filter, comparison );

    qm.run();

    checkResultCount( &qm, type, count );
}

void
TestSqlQueryMaker::testReturnFunctions_data()
{
    QTest::addColumn<Collections::QueryMaker::ReturnFunction>( "function" );
    QTest::addColumn<qint64>( "value" );
    QTest::addColumn<QString>( "result" );

    QTest::newRow( "count tracks" ) << Collections::QueryMaker::Count << Meta::valTitle << QStringLiteral( "6" );
    QTest::newRow( "sum of playcount" ) << Collections::QueryMaker::Sum << Meta::valPlaycount << QStringLiteral( "160" );
    QTest::newRow( "min score" ) << Collections::QueryMaker::Min << Meta::valScore << QStringLiteral( "50" );
    QTest::newRow( "max rating" ) << Collections::QueryMaker::Max << Meta::valRating << QStringLiteral( "9" );
}

void
TestSqlQueryMaker::testReturnFunctions()
{
    QFETCH( Collections::QueryMaker::ReturnFunction, function );
    QFETCH( qint64, value );
    QFETCH( QString, result );

    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Custom );
    qm.addReturnFunction( function, value );

    qm.run();

    QCOMPARE( qm.customData().first(), result );
}

void
TestSqlQueryMaker::testLabelMatch()
{
    Meta::LabelPtr label = m_collection->registry()->getLabel( QStringLiteral("labelB") );
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Track );
    qm.addMatch( label );
    qm.run();

    QCOMPARE( qm.tracks().count(), 3 );
}

void
TestSqlQueryMaker::testMultipleLabelMatches()
{
    Meta::LabelPtr labelB = m_collection->registry()->getLabel( QStringLiteral("labelB") );
    Meta::LabelPtr labelA = m_collection->registry()->getLabel( QStringLiteral("labelA") );
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Track );
    qm.addMatch( labelB );
    qm.addMatch( labelA );
    qm.run();

    QCOMPARE( qm.tracks().count(), 1 );
}

void
TestSqlQueryMaker::testQueryTypesWithLabelMatching_data()
{
    QTest::addColumn<Collections::QueryMaker::QueryType>( "type" );
    QTest::addColumn<int>( "result" );

    QTest::newRow( "query tracks" ) << Collections::QueryMaker::Track << 1;
    QTest::newRow( "query albums" ) << Collections::QueryMaker::Album << 1;
    QTest::newRow( "query artists" ) << Collections::QueryMaker::Artist << 1;
    QTest::newRow( "query genre" ) << Collections::QueryMaker::Genre << 1;
    QTest::newRow( "query composers" ) << Collections::QueryMaker::Composer << 1;
    QTest::newRow( "query years" ) << Collections::QueryMaker::Year << 1;
    QTest::newRow( "query labels" ) << Collections::QueryMaker::Label << 2;
}

void
TestSqlQueryMaker::testQueryTypesWithLabelMatching()
{

    QFETCH( Collections::QueryMaker::QueryType, type );
    QFETCH( int, result );

    Meta::LabelPtr labelB = m_collection->registry()->getLabel( QStringLiteral("labelB") );
    Meta::LabelPtr labelA = m_collection->registry()->getLabel( QStringLiteral("labelA") );
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( type );
    qm.addMatch( labelB );
    qm.addMatch( labelA );
    qm.run();

    checkResultCount( &qm, type, result );
}

void
TestSqlQueryMaker::testFilterOnLabelsAndCombination()
{
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Track );
    qm.beginAnd();
    qm.addFilter( Meta::valLabel, QStringLiteral("labelB"), true, true );
    qm.addFilter( Meta::valLabel, QStringLiteral("labelA"), false, false );
    qm.endAndOr();
    qm.run();

    QCOMPARE( qm.tracks().count(), 1 );
}

void
TestSqlQueryMaker::testFilterOnLabelsOrCombination()
{
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Track );
    qm.beginOr();
    qm.addFilter( Meta::valLabel, QStringLiteral("labelB"), true, true );
    qm.addFilter( Meta::valLabel, QStringLiteral("labelA"), false, false );
    qm.endAndOr();
    qm.run();

    QCOMPARE( qm.tracks().count(), 3 );
}

void
TestSqlQueryMaker::testFilterOnLabelsNegationAndCombination()
{
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Track );
    qm.beginAnd();
    qm.excludeFilter( Meta::valLabel, QStringLiteral("labelB"), true, true );
    qm.excludeFilter( Meta::valLabel, QStringLiteral("labelA"), false, false );
    qm.endAndOr();
    qm.run();

    QCOMPARE( qm.tracks().count(), 3 );
}

void
TestSqlQueryMaker::testFilterOnLabelsNegationOrCombination()
{
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Track );
    qm.beginOr();
    qm.excludeFilter( Meta::valLabel, QStringLiteral("labelB"), true, true );
    qm.excludeFilter( Meta::valLabel, QStringLiteral("labelA"), false, false );
    qm.endAndOr();
    qm.run();

    QCOMPARE( qm.tracks().count(), 5 );
}

void
TestSqlQueryMaker::testComplexLabelsFilter()
{
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Track );
    qm.beginOr();
    qm.addFilter( Meta::valLabel, QStringLiteral("test"), true, true );
    qm.beginAnd();
    qm.addFilter( Meta::valLabel, QStringLiteral("labelB"), false, false );
    qm.excludeFilter( Meta::valLabel, QStringLiteral("labelA"), false, true );
    qm.endAndOr();
    qm.endAndOr();
    qm.run();

    QCOMPARE( qm.tracks().count(), 3 );
}

void
TestSqlQueryMaker::testLabelQueryMode_data()
{
    QTest::addColumn<Collections::QueryMaker::QueryType>( "type" );
    QTest::addColumn<Collections::QueryMaker::LabelQueryMode>( "labelMode" );
    QTest::addColumn<Collections::QueryMaker::AlbumQueryMode>( "albumMode" );
    QTest::addColumn<int>( "result" );

    QTest::newRow( "labels with querymode WithoutLabels" ) << QueryMaker::Label << QueryMaker::OnlyWithoutLabels << QueryMaker::AllAlbums << 0;
    QTest::newRow( "tracks with labels" ) << QueryMaker::Track << QueryMaker::OnlyWithLabels << QueryMaker::AllAlbums << 4;
    QTest::newRow( "Compilations with labels" ) << QueryMaker::Album << QueryMaker::OnlyWithLabels << QueryMaker::OnlyCompilations << 1;
    QTest::newRow( "Compilations without labels" ) << QueryMaker::Album << QueryMaker::OnlyWithoutLabels << QueryMaker::OnlyCompilations << 1;
}

void
TestSqlQueryMaker::testLabelQueryMode()
{
    QFETCH( QueryMaker::QueryType, type );
    QFETCH( QueryMaker::LabelQueryMode, labelMode );
    QFETCH( QueryMaker::AlbumQueryMode, albumMode );
    QFETCH( int, result );

    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( type );
    qm.setAlbumQueryMode( albumMode );
    qm.setLabelQueryMode( labelMode );
    qm.run();

    checkResultCount( &qm, type, result );
}

