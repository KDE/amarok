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
#include "core-impl/meta/file/TagLibUtils.h"

#include "DatabaseUpdater.h"
#include "SqlCollection.h"
#include "SqlQueryMaker.h"
#include "SqlRegistry.h"
#include "mysqlecollection/MySqlEmbeddedStorage.h"

#include "SqlMountPointManagerMock.h"

#include <QSignalSpy>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestSqlQueryMaker )

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

//required for QTest, this is not done in Querymaker.h
Q_DECLARE_METATYPE( Collections::QueryMaker::QueryType )
Q_DECLARE_METATYPE( Collections::QueryMaker::NumberComparison )
Q_DECLARE_METATYPE( Collections::QueryMaker::ReturnFunction )
Q_DECLARE_METATYPE( Collections::QueryMaker::AlbumQueryMode )
Q_DECLARE_METATYPE( Collections::QueryMaker::LabelQueryMode )

TestSqlQueryMaker::TestSqlQueryMaker()
{
    qRegisterMetaType<Meta::DataPtr>();
    qRegisterMetaType<Meta::DataList>();
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
    qRegisterMetaType<Meta::Label>();
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
    m_tmpDir = new KTempDir();
    m_storage = new MySqlEmbeddedStorage( m_tmpDir->name() );
    m_collection = new Collections::SqlCollection( "testId", "testcollection" );
    m_collection->setSqlStorage( m_storage );

    QMap<int,QString> mountPoints;
    mountPoints.insert( 1, "/foo" );
    mountPoints.insert( 2, "/bar" );

    m_mpm = new SqlMountPointManagerMock();
    m_mpm->mountPoints = mountPoints;

    m_collection->setMountPointManager( m_mpm );
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
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(4,'album4',NULL);" );
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
    m_storage->query( "INSERT INTO urls(id,deviceid,rpath,uniqueid) VALUES (4, 2, './NothingHere.mp3','4');" );
    m_storage->query( "INSERT INTO urls(id,deviceid,rpath,uniqueid) VALUES (5, 1, './GuessWhat.mp3','5');" );
    m_storage->query( "INSERT INTO urls(id,deviceid,rpath,uniqueid) VALUES (6, 2, './LookItsA.flac','6');" );

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

    m_storage->query( "INSERT INTO statistics(url,createdate,accessdate,score,rating,playcount) "
                      "VALUES(1,1000,10000, 50.0,5,100);" );
    m_storage->query( "INSERT INTO statistics(url,createdate,accessdate,score,rating,playcount) "
                      "VALUES(2,3000,30000, 70.0,9,50);" );

    m_storage->query( "INSERT INTO labels(id,label) VALUES (1,'labelA'), (2,'labelB'),(3,'test');" );
    m_storage->query( "INSERT INTO urls_labels(url,label) VALUES (1,1),(1,2),(2,2),(3,3),(4,3),(4,2);" );

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
    QCOMPARE( qm.albums( "testId" ).count(), 5 );
}

void
TestSqlQueryMaker::testQueryArtists()
{
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Artist );
    qm.run();
    QCOMPARE( qm.artists( "testId" ).count(), 3 );
}

void
TestSqlQueryMaker::testQueryComposers()
{
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Composer );
    qm.run();
    QCOMPARE( qm.composers( "testId" ).count(), 3 );
}

void
TestSqlQueryMaker::testQueryGenres()
{
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Genre );
    qm.run();
    QCOMPARE( qm.genres( "testId" ).count(), 3 );
}

void
TestSqlQueryMaker::testQueryYears()
{
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Year );
    qm.run();
    QCOMPARE( qm.years( "testId" ).count(), 3 );
}

void
TestSqlQueryMaker::testQueryTracks()
{
    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Track );
    qm.run();
    QCOMPARE( qm.tracks( "testId" ).count(), 6 );
}

void
TestSqlQueryMaker::testAlbumQueryMode()
{
    Collections::SqlQueryMaker qm( m_collection );

    qm.setBlocking( true );
    qm.setAlbumQueryMode( Collections::QueryMaker::OnlyCompilations );
    qm.setQueryType( Collections::QueryMaker::Album );
    qm.run();
    QCOMPARE( qm.albums( "testId" ).count(), 1 );

    qm.reset();
    qm.setBlocking( true );
    qm.setAlbumQueryMode( Collections::QueryMaker::OnlyNormalAlbums );
    qm.setQueryType( Collections::QueryMaker::Album );
    qm.run();
    QCOMPARE( qm.albums( "testId" ).count(), 4 );

    qm.reset();
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Track );
    qm.setAlbumQueryMode( Collections::QueryMaker::OnlyCompilations );
    qm.run();
    QCOMPARE( qm.tracks( "testId" ).count(), 2 );

    qm.reset();
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Track );
    qm.setAlbumQueryMode( Collections::QueryMaker::OnlyNormalAlbums );
    qm.run();
    QCOMPARE( qm.tracks( "testId" ).count(), 4 );

    qm.reset();
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Artist );
    qm.setAlbumQueryMode( Collections::QueryMaker::OnlyCompilations );
    qm.run();
    QCOMPARE( qm.artists( "testId" ).count() , 2 );

    qm.reset();
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Artist );
    qm.setAlbumQueryMode( Collections::QueryMaker::OnlyNormalAlbums );
    qm.run();
    QCOMPARE( qm.artists( "testId" ).count(), 3 );

    qm.reset();
    qm.setBlocking( true );
    qm.setAlbumQueryMode( Collections::QueryMaker::OnlyCompilations );
    qm.setQueryType( Collections::QueryMaker::Genre );
    qm.run();
    QCOMPARE( qm.genres( "testId" ).count(), 2 );

    qm.reset();
    qm.setBlocking( true );
    qm.setAlbumQueryMode( Collections::QueryMaker::OnlyNormalAlbums );
    qm.setQueryType( Collections::QueryMaker::Genre );
    qm.run();
    QCOMPARE( qm.genres( "testId" ).count(), 3 );

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
        QSignalSpy spy( qm, SIGNAL(queryDone()) );
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
        QTest::qWait( 1000 - 10 * iteration );
        iteration++;
    } while ( queryNotDoneYet );
    qDebug() << "Iterations: " << iteration;
}

void
TestSqlQueryMaker::testAsyncAlbumQuery()
{
    Collections::QueryMaker *qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Album );
    QSignalSpy doneSpy1( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy1( qm, SIGNAL(newResultReady(QString,Meta::AlbumList)));

    qm->run();

    QTest::kWaitForSignal( qm, SIGNAL(queryDone()), 1000 );

    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(1).canConvert<Meta::AlbumList>() );
    QCOMPARE( args1.value(1).value<Meta::AlbumList>().count(), 5 );
    QCOMPARE( doneSpy1.count(), 1);
    delete qm;

    qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Album );
    QSignalSpy doneSpy2( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy2( qm, SIGNAL(newResultReady(QString,Meta::AlbumList)));
    qm->addFilter( Meta::valAlbum, "foo" ); //should result in no match

    qm->run();

    QTest::kWaitForSignal( qm, SIGNAL(queryDone()), 1000 );

    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(1).canConvert<Meta::AlbumList>() );
    QCOMPARE( args2.value(1).value<Meta::AlbumList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncArtistQuery()
{
    Collections::QueryMaker *qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Artist );
    QSignalSpy doneSpy1( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy1( qm, SIGNAL(newResultReady(QString,Meta::ArtistList)));

    qm->run();

    QTest::kWaitForSignal( qm, SIGNAL(queryDone()), 1000 );

    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(1).canConvert<Meta::ArtistList>() );
    QCOMPARE( args1.value(1).value<Meta::ArtistList>().count(), 3 );
    QCOMPARE( doneSpy1.count(), 1);
    delete qm;

    qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Artist );
    QSignalSpy doneSpy2( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy2( qm, SIGNAL(newResultReady(QString,Meta::ArtistList)));
    qm->addFilter( Meta::valArtist, "foo" ); //should result in no match

    qm->run();

    QTest::kWaitForSignal( qm, SIGNAL(queryDone()), 1000 );

    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(1).canConvert<Meta::ArtistList>() );
    QCOMPARE( args2.value(1).value<Meta::ArtistList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncComposerQuery()
{
    Collections::QueryMaker *qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Composer );
    QSignalSpy doneSpy1( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy1( qm, SIGNAL(newResultReady(QString,Meta::ComposerList)));

    qm->run();

    QTest::kWaitForSignal( qm, SIGNAL(queryDone()), 1000 );

    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(1).canConvert<Meta::ComposerList>() );
    QCOMPARE( args1.value(1).value<Meta::ComposerList>().count(), 3 );
    QCOMPARE( doneSpy1.count(), 1);

    delete qm;

    qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Composer );
    QSignalSpy doneSpy2( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy2( qm, SIGNAL(newResultReady(QString,Meta::ComposerList)));
    qm->addFilter( Meta::valComposer, "foo" ); //should result in no match

    qm->run();

    QTest::kWaitForSignal( qm, SIGNAL(queryDone()), 1000 );

    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(1).canConvert<Meta::ComposerList>() );
    QCOMPARE( args2.value(1).value<Meta::ComposerList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncTrackQuery()
{
    Collections::QueryMaker *qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Track );
    QSignalSpy doneSpy1( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy1( qm, SIGNAL(newResultReady(QString,Meta::TrackList)));

    qm->run();

    QTest::kWaitForSignal( qm, SIGNAL(queryDone()), 1000 );

    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(1).canConvert<Meta::TrackList>() );
    QCOMPARE( args1.value(1).value<Meta::TrackList>().count(), 6 );
    QCOMPARE( doneSpy1.count(), 1);

    delete qm;

    qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Track );
    QSignalSpy doneSpy2( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy2( qm, SIGNAL(newResultReady(QString,Meta::TrackList)));
    qm->addFilter( Meta::valTitle, "foo" ); //should result in no match

    qm->run();

    QTest::kWaitForSignal( qm, SIGNAL(queryDone()), 1000 );

    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(1).canConvert<Meta::TrackList>() );
    QCOMPARE( args2.value(1).value<Meta::TrackList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncGenreQuery()
{
    Collections::QueryMaker *qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Genre );
    QSignalSpy doneSpy1( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy1( qm, SIGNAL(newResultReady(QString,Meta::GenreList)));

    qm->run();

    QTest::kWaitForSignal( qm, SIGNAL(queryDone()), 1000 );

    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(1).canConvert<Meta::GenreList>() );
    QCOMPARE( args1.value(1).value<Meta::GenreList>().count(), 3 );
    QCOMPARE( doneSpy1.count(), 1);

    delete qm;

    qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Genre );
    QSignalSpy doneSpy2( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy2( qm, SIGNAL(newResultReady(QString,Meta::GenreList)));
    qm->addFilter( Meta::valGenre, "foo" ); //should result in no match

    qm->run();

    QTest::kWaitForSignal( qm, SIGNAL(queryDone()), 1000 );

    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(1).canConvert<Meta::GenreList>() );
    QCOMPARE( args2.value(1).value<Meta::GenreList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncYearQuery()
{
    Collections::QueryMaker *qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Year );
    QSignalSpy doneSpy1( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy1( qm, SIGNAL(newResultReady(QString,Meta::YearList)));

    qm->run();

    QTest::kWaitForSignal( qm, SIGNAL(queryDone()), 1000 );

    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(1).canConvert<Meta::YearList>() );
    QCOMPARE( args1.value(1).value<Meta::YearList>().count(), 3 );
    QCOMPARE( doneSpy1.count(), 1);

    delete qm;

    qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Year );
    QSignalSpy doneSpy2( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy2( qm, SIGNAL(newResultReady(QString,Meta::YearList)));
    qm->addFilter( Meta::valYear, "foo" ); //should result in no match

    qm->run();

    QTest::kWaitForSignal( qm, SIGNAL(queryDone()), 1000 );

    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(1).canConvert<Meta::YearList>() );
    QCOMPARE( args2.value(1).value<Meta::YearList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncTrackDataQuery()
{
    Collections::QueryMaker *qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Track );
    qm->setReturnResultAsDataPtrs( true );
    QSignalSpy doneSpy1( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy1( qm, SIGNAL(newResultReady(QString,Meta::DataList)));

    qm->run();

    QTest::kWaitForSignal( qm, SIGNAL(queryDone()), 1000 );

    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(1).canConvert<Meta::DataList>() );
    QCOMPARE( args1.value(1).value<Meta::DataList>().count(), 6 );
    QCOMPARE( doneSpy1.count(), 1);

    delete qm;

    qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Track );
    qm->setReturnResultAsDataPtrs( true );
    QSignalSpy doneSpy2( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy2( qm, SIGNAL(newResultReady(QString,Meta::DataList)));
    qm->addFilter( Meta::valTitle, "foo" ); //should result in no match

    qm->run();

    QTest::kWaitForSignal( qm, SIGNAL(queryDone()), 1000 );

    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(1).canConvert<Meta::DataList>() );
    QCOMPARE( args2.value(1).value<Meta::DataList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncCustomQuery()
{
    Collections::QueryMaker *qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Custom );
    qm->addReturnFunction( Collections::QueryMaker::Count, Meta::valTitle );
    QSignalSpy doneSpy1( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy1( qm, SIGNAL(newResultReady(QString,QStringList)));

    qm->run();

    QTest::kWaitForSignal( qm, SIGNAL(queryDone()), 1000 );

    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(1).canConvert<QStringList>() );
    QCOMPARE( args1.value(1).value<QStringList>().count(), 1 );
    QCOMPARE( args1.value(1).value<QStringList>().first(), QString( "6" ) );
    QCOMPARE( doneSpy1.count(), 1);

    delete qm;

    qm = new Collections::SqlQueryMaker( m_collection );
    qm->setQueryType( Collections::QueryMaker::Custom );
    qm->addReturnFunction( Collections::QueryMaker::Count, Meta::valTitle );
    QSignalSpy doneSpy2( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy2( qm, SIGNAL(newResultReady(QString,QStringList)));
    qm->addFilter( Meta::valTitle, "foo" ); //should result in no match

    qm->run();

    QTest::kWaitForSignal( qm, SIGNAL(queryDone()), 1000 );

    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(1).canConvert<QStringList>() );
    QCOMPARE( args2.value(1).value<QStringList>().count(), 1 );
    QCOMPARE( args2.value(1).value<QStringList>().first(), QString( "0" ) );
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
    qm.setReturnResultAsDataPtrs( true );

    qm.addFilter( value, filter, matchBeginning, matchEnd );

    qm.run();

    QCOMPARE( qm.data( "testId" ).count(), count );
}

#define PointerToDataList( t ) { \
    Meta::DataList data; \
    data.append( Meta::DataPtr::staticCast( t ) ); \
    return data; \
}

Meta::DataList asList( const Meta::TrackPtr &track )
{
    PointerToDataList( track );
}

Meta::DataList asList( const Meta::ArtistPtr &artist )
{
    PointerToDataList( artist );
}

Meta::DataList asList( const Meta::AlbumPtr &album )
{
    PointerToDataList( album );
}

Meta::DataList asList( const Meta::GenrePtr &genre )
{
    PointerToDataList( genre );
}

Meta::DataList asList( const Meta::ComposerPtr &composer )
{
    PointerToDataList( composer );
}

Meta::DataList asList( const Meta::YearPtr &year )
{
    PointerToDataList( year );
}

#undef PointerToDataList

void
TestSqlQueryMaker::testMatch_data()
{
    QTest::addColumn<Collections::QueryMaker::QueryType>( "type" );
    QTest::addColumn<Meta::DataList>( "dataList" );
    QTest::addColumn<int>( "count" );

    SqlRegistry *r = m_collection->registry();

    QTest::newRow( "track matches artist" ) << Collections::QueryMaker::Track << asList( r->getArtist( "artist1" ) ) << 3;
    QTest::newRow( "track matches album" ) << Collections::QueryMaker::Track << asList( r->getAlbum( "album1",1,1 ) ) << 1;
    QTest::newRow( "track matches genre" ) << Collections::QueryMaker::Track << asList( r->getGenre( "genre1" ) ) << 3;
    QTest::newRow( "track matches composer" ) << Collections::QueryMaker::Track << asList( r->getComposer( "composer1" ) ) << 3;
    QTest::newRow( "track matches year" ) << Collections::QueryMaker::Track << asList( r->getYear("1")) << 3;
    QTest::newRow( "artist matches album" ) << Collections::QueryMaker::Artist << asList(r->getAlbum("album1",1,1)) << 1;
    QTest::newRow( "artist matches genre" ) << Collections::QueryMaker::Artist << asList(r->getGenre("genre1")) << 2;
}

void
TestSqlQueryMaker::testMatch()
{
    QFETCH( Collections::QueryMaker::QueryType, type );
    QFETCH( Meta::DataList, dataList );
    QFETCH( int, count );

    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( type );
    qm.setReturnResultAsDataPtrs( true );

    foreach( const Meta::DataPtr &data, dataList )
    {
        qm.addMatch( data );
    }

    qm.run();

    QCOMPARE( qm.data( "testId" ).count(), count );
}

void
TestSqlQueryMaker::testDynamicCollection()
{
    //this will not crash as we reset the correct mock in cleanup()
    SqlMountPointManagerMock mpm;

    QMap<int, QString> mountPoints;

    mpm.mountPoints = mountPoints;

    m_collection->setMountPointManager( &mpm );

    Collections::SqlQueryMaker trackQm( m_collection );
    trackQm.setQueryType( Collections::QueryMaker::Track );
    trackQm.setBlocking( true );
    trackQm.run();
    QCOMPARE( trackQm.tracks( "testId" ).count(), 3 );

    mpm.mountPoints.insert( 1, "/foo" );

    trackQm.reset();
    trackQm.setQueryType( Collections::QueryMaker::Track );
    trackQm.setBlocking( true );
    trackQm.run();
    QCOMPARE( trackQm.tracks( "testId" ).count(), 4 );

    Collections::SqlQueryMaker artistQm( m_collection );
    artistQm.setQueryType( Collections::QueryMaker::Artist );
    artistQm.setBlocking( true );
    artistQm.run();
    QCOMPARE( artistQm.artists( "testId" ).count(), 2 );

    Collections::SqlQueryMaker albumQm( m_collection );
    albumQm.setQueryType( Collections::QueryMaker::Album );
    albumQm.setBlocking( true );
    albumQm.run();
    QCOMPARE( albumQm.albums( "testId" ).count(), 4 );

    Collections::SqlQueryMaker genreQm( m_collection );
    genreQm.setQueryType( Collections::QueryMaker::Genre );
    genreQm.setBlocking( true );
    genreQm.run();
    QCOMPARE( genreQm.genres( "testId" ).count(), 2 );

    Collections::SqlQueryMaker composerQm( m_collection );
    composerQm.setQueryType( Collections::QueryMaker::Composer );
    composerQm.setBlocking( true );
    composerQm.run();
    QCOMPARE( composerQm.composers( "testId" ).count(), 2 );

    Collections::SqlQueryMaker yearQm( m_collection );
    yearQm.setQueryType( Collections::QueryMaker::Year );
    yearQm.setBlocking( true );
    yearQm.run();
    QCOMPARE( yearQm.years( "testId" ).count(), 2 );

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

    QString insertTrack = QString( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                              "VALUES(999,999,'%1','',1,1,1,1,1);").arg( m_storage->escape( filter ) );

    //there is a unique index on TRACKS.URL
    m_storage ->query( "INSERT INTO urls(id,deviceid,rpath,uniqueid) VALUES(999,-1, './foobar.mp3','999');");
    m_storage->query( insertTrack );

    QCOMPARE( m_storage->query( "select count(*) from urls where id = 999" ).first(), QString("1") );
    QCOMPARE( m_storage->query( "select count(*) from tracks where id = 999" ).first(), QString("1") );

    Collections::SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( Collections::QueryMaker::Track );
    qm.setReturnResultAsDataPtrs( true );
    qm.addFilter( Meta::valTitle, filter, !like, !like );

    qm.run();

    m_storage->query( "DELETE FROM urls WHERE id = 999;" );
    m_storage->query( "DELETE FROM tracks WHERE id = 999;" );

    QCOMPARE( qm.data( "testId" ).count(), 1 );
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

    QTest::newRow( "include rating greater 4" ) << Collections::QueryMaker::Track << Meta::valRating << 4 << Collections::QueryMaker::GreaterThan << false << 2;
    QTest::newRow( "exclude rating smaller 4" ) << Collections::QueryMaker::Album << Meta::valRating << 4 << Collections::QueryMaker::LessThan << true << 2;
    QTest::newRow( "exclude tracks first played later than 2000" ) << Collections::QueryMaker::Track << Meta::valFirstPlayed << 2000 << Collections::QueryMaker::GreaterThan << true << 1;
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
    qm.setReturnResultAsDataPtrs( true );

    if( exclude )
        qm.excludeNumberFilter( value, filter, comparison );
    else
        qm.addNumberFilter( value, filter, comparison );

    qm.run();

    QCOMPARE( qm.data( "testId" ).count(), count );
}

void
TestSqlQueryMaker::testReturnFunctions_data()
{
    QTest::addColumn<Collections::QueryMaker::ReturnFunction>( "function" );
    QTest::addColumn<qint64>( "value" );
    QTest::addColumn<QString>( "result" );

    QTest::newRow( "count tracks" ) << Collections::QueryMaker::Count << Meta::valTitle << QString( "6" );
    QTest::newRow( "sum of playcount" ) << Collections::QueryMaker::Sum << Meta::valPlaycount << QString( "150" );
    QTest::newRow( "min score" ) << Collections::QueryMaker::Min << Meta::valScore << QString( "50" );
    QTest::newRow( "max rating" ) << Collections::QueryMaker::Max << Meta::valRating << QString( "9" );
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

    QCOMPARE( qm.customData( "testId" ).first(), result );

}

void
TestSqlQueryMaker::testLabelMatch()
{
    Meta::LabelPtr label = m_collection->registry()->getLabel( "labelB", -1 );
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Track );
    qm.addMatch( label );
    qm.run();

    QCOMPARE( qm.tracks().count(), 3 );
}

void
TestSqlQueryMaker::testMultipleLabelMatches()
{
    Meta::LabelPtr labelB = m_collection->registry()->getLabel( "labelB", -1 );
    Meta::LabelPtr labelA = m_collection->registry()->getLabel( "labelA", -1 );
    SqlQueryMaker qm( m_collection );
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
    QTest::addColumn<QueryMaker::QueryType>( "type" );
    QTest::addColumn<int>( "result" );

    QTest::newRow( "query tracks" ) << QueryMaker::Track << 1;
    QTest::newRow( "query albums" ) << QueryMaker::Album << 1;
    QTest::newRow( "query artists" ) << QueryMaker::Artist << 1;
    QTest::newRow( "query genre" ) << QueryMaker::Genre << 1;
    QTest::newRow( "query composers" ) << QueryMaker::Composer << 1;
    QTest::newRow( "query years" ) << QueryMaker::Year << 1;
    QTest::newRow( "query labels" ) << QueryMaker::Label << 2;
}

void
TestSqlQueryMaker::testQueryTypesWithLabelMatching()
{

    QFETCH( QueryMaker::QueryType, type );
    QFETCH( int, result );

    Meta::LabelPtr labelB = m_collection->registry()->getLabel( "labelB", -1 );
    Meta::LabelPtr labelA = m_collection->registry()->getLabel( "labelA", -1 );
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setReturnResultAsDataPtrs( true );
    qm.setQueryType( type );
    qm.addMatch( labelB );
    qm.addMatch( labelA );
    qm.run();

    QCOMPARE( qm.data().count(), result );
}

void
TestSqlQueryMaker::testFilterOnLabelsAndCombination()
{
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Track );
    qm.beginAnd();
    qm.addFilter( Meta::valLabel, "labelB", true, true );
    qm.addFilter( Meta::valLabel, "labelA", false, false );
    qm.endAndOr();
    qm.run();

    QCOMPARE( qm.tracks().count(), 1 );
}

void
TestSqlQueryMaker::testFilterOnLabelsOrCombination()
{
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Track );
    qm.beginOr();
    qm.addFilter( Meta::valLabel, "labelB", true, true );
    qm.addFilter( Meta::valLabel, "labelA", false, false );
    qm.endAndOr();
    qm.run();

    QCOMPARE( qm.tracks().count(), 3 );
}

void
TestSqlQueryMaker::testFilterOnLabelsNegationAndCombination()
{
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Track );
    qm.beginAnd();
    qm.excludeFilter( Meta::valLabel, "labelB", true, true );
    qm.excludeFilter( Meta::valLabel, "labelA", false, false );
    qm.endAndOr();
    qm.run();

    QCOMPARE( qm.tracks().count(), 3 );
}

void
TestSqlQueryMaker::testFilterOnLabelsNegationOrCombination()
{
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Track );
    qm.beginOr();
    qm.excludeFilter( Meta::valLabel, "labelB", true, true );
    qm.excludeFilter( Meta::valLabel, "labelA", false, false );
    qm.endAndOr();
    qm.run();

    QCOMPARE( qm.tracks().count(), 5 );
}

void
TestSqlQueryMaker::testComplexLabelsFilter()
{
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Track );
    qm.beginOr();
    qm.addFilter( Meta::valLabel, "test", true, true );
    qm.beginAnd();
    qm.addFilter( Meta::valLabel, "labelB", false, false );
    qm.excludeFilter( Meta::valLabel, "labelA", false, true );
    qm.endAndOr();
    qm.endAndOr();
    qm.run();

    QCOMPARE( qm.tracks().count(), 3 );
}

void
TestSqlQueryMaker::testLabelQueryMode_data()
{
    QTest::addColumn<QueryMaker::QueryType>( "type" );
    QTest::addColumn<QueryMaker::LabelQueryMode>( "labelMode" );
    QTest::addColumn<QueryMaker::AlbumQueryMode>( "albumMode" );
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
    qm.setReturnResultAsDataPtrs( true );
    qm.setQueryType( type );
    qm.setAlbumQueryMode( albumMode );
    qm.setLabelQueryMode( labelMode );
    qm.run();

    QCOMPARE( qm.data().count(), result );

}

#include "TestSqlQueryMaker.moc"
