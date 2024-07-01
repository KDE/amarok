/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "TestMemoryQueryMaker.h"

#include "mocks/MetaMock.h"
#include "mocks/MockTrack.h"

#include "FileType.h"

#include <QVariantMap>
#include <QSharedPointer>
#include <QSignalSpy>

#include <gmock/gmock.h>

using ::testing::AnyNumber;
using ::testing::Return;

QTEST_GUILESS_MAIN( TestMemoryQueryMaker )

TestMemoryQueryMaker::TestMemoryQueryMaker()
{
    int argc = 1;
    char **argv = (char **) malloc(sizeof(char *));
    argv[0] = strdup( QCoreApplication::applicationName().toLocal8Bit().data() );
    ::testing::InitGoogleMock( &argc, argv );
    qRegisterMetaType<Meta::TrackList>();
    qRegisterMetaType<Meta::AlbumList>();
    qRegisterMetaType<Meta::ArtistList>();
}

void
TestMemoryQueryMaker::initTestCase()
{
    // prepare a memory collection with some test data
    m_mc = QSharedPointer<Collections::MemoryCollection>( new Collections::MemoryCollection() );

    MetaMock *track;

    QVariantMap map;
    map.insert( Meta::Field::UNIQUEID, QStringLiteral("1") );
    map.insert( Meta::Field::TITLE, QStringLiteral("Skater Boy") );
    map.insert( Meta::Field::RATING, 3 );
//    map.insert( Meta::Field::TYPE, int(Amarok::Mp3) );
    map.insert( Meta::Field::TRACKNUMBER, 3 );
    track = new MetaMock( map );
    track->m_artist = new MockArtist(QStringLiteral("Avril Lavigne"));
    track->m_album = new MockAlbum(QStringLiteral("Let Go"));
    m_mc->addTrack( Meta::TrackPtr( track ) );

    map.insert( Meta::Field::UNIQUEID, QStringLiteral("2") );
    map.insert( Meta::Field::TITLE, QStringLiteral("Substitute") );
    map.insert( Meta::Field::RATING, 4 );
 //   map.insert( Meta::Field::TYPE, int(Amarok::Ogg) );
    map.insert( Meta::Field::TRACKNUMBER, 1 );
    track = new MetaMock( map );
    track->m_artist = new MockArtist(QStringLiteral("Clout") );
    track->m_album = new MockAlbum(QStringLiteral("Substitute") );
    m_mc->addTrack( Meta::TrackPtr( track ) );

    map.insert( Meta::Field::UNIQUEID, QStringLiteral("3") );
    map.insert( Meta::Field::TITLE, QStringLiteral("I Say A Little Prayer") );
    map.insert( Meta::Field::RATING, 2 );
  //  map.insert( Meta::Field::TYPE, int(Amarok::Wma) );
    map.insert( Meta::Field::TRACKNUMBER, 1 );
    map.insert( Meta::Field::DISCNUMBER, 2 );
    track = new MetaMock( map );
    track->m_artist = new MockArtist(QStringLiteral("The Bosshoss") );
    track->m_album = new MockAlbum(QStringLiteral("Rodeo Radio") );
    m_mc->addTrack( Meta::TrackPtr( track ) );
}

void TestMemoryQueryMaker::cleanupTestCase()
{
}

void
TestMemoryQueryMaker::testDeleteQueryMakerWhileQueryIsRunning()
{
    QSharedPointer<Collections::MemoryCollection> mc( new Collections::MemoryCollection() );
    mc->addTrack( Meta::TrackPtr( new MetaMock( QVariantMap() )));
    mc->addTrack( Meta::TrackPtr( new MetaMock( QVariantMap() )));
    Meta::MockTrack *mock = new Meta::MockTrack();
    EXPECT_CALL( *mock, uidUrl() ).Times( AnyNumber() ).WillRepeatedly( Return( QStringLiteral("track3") ) );
    Meta::TrackPtr trackPtr( mock );
    mc->addTrack( trackPtr );

    Collections::MemoryQueryMaker *qm = new Collections::MemoryQueryMaker( mc.toWeakRef(), QStringLiteral("test") );
    qm->setQueryType( Collections::QueryMaker::Track );

    qm->run();
    delete qm;
    //we cannot wait for a signal here....
    //QTest::qWait( 500 );
}

void
TestMemoryQueryMaker::testDeleteCollectionWhileQueryIsRunning()
{
    QSharedPointer<Collections::MemoryCollection> mc( new Collections::MemoryCollection() );
    mc->addTrack( Meta::TrackPtr( new MetaMock( QVariantMap() )));
    mc->addTrack( Meta::TrackPtr( new MetaMock( QVariantMap() )));

    Collections::MemoryQueryMaker *qm = new Collections::MemoryQueryMaker( mc, QStringLiteral("test") );
    qm->setQueryType( Collections::QueryMaker::Track );

    QSignalSpy spy( qm, &Collections::QueryMaker::queryDone);

    qm->run();
    mc.clear();
    QTest::qWait( 500 );
    QCOMPARE( spy.count(), 1 );

    delete qm;
}

class TestStringMemoryFilter : public StringMemoryFilter
{
public:
    TestStringMemoryFilter() : StringMemoryFilter() {}

protected:
    QString value( const Meta::TrackPtr &track ) const override { Q_UNUSED(track); return QStringLiteral("abcdef"); }

};

void
TestMemoryQueryMaker::testStringMemoryFilterSpeedFullMatch()
{
    //Test 1: match complete string
    TestStringMemoryFilter filter1;
    filter1.setFilter( QStringLiteral( "abcdef" ), true, true );

    QBENCHMARK {
        filter1.filterMatches( Meta::TrackPtr() );
    }
}

void
TestMemoryQueryMaker::testStringMemoryFilterSpeedMatchBegin()
{
    //Test 2: match beginning of string
    TestStringMemoryFilter filter2;
    filter2.setFilter( QStringLiteral( "abcd" ), true, false );

    QBENCHMARK {
        filter2.filterMatches( Meta::TrackPtr() );
    }
}

void
TestMemoryQueryMaker::testStringMemoryFilterSpeedMatchEnd()
{
    //Test 3: match end of string
    TestStringMemoryFilter filter3;
    filter3.setFilter( QStringLiteral( "cdef" ), false, true );

    QBENCHMARK {
        filter3.filterMatches( Meta::TrackPtr() );
    }
}

void
TestMemoryQueryMaker::testStringMemoryFilterSpeedMatchAnywhere()
{
    //Test 4: match anywhere in string
    TestStringMemoryFilter filter4;
    filter4.setFilter( QStringLiteral( "bcde" ), false, false );

    QBENCHMARK {
        filter4.filterMatches( Meta::TrackPtr() );
    }
}

Meta::TrackList
TestMemoryQueryMaker::executeQueryMaker( Collections::QueryMaker *qm )
{
    QSignalSpy doneSpy1( qm, &Collections::QueryMaker::queryDone );
    QSignalSpy resultSpy1( qm, &Collections::QueryMaker::newTracksReady );

    qm->setQueryType( Collections::QueryMaker::Track );
    qm->run();

    doneSpy1.wait( 1000 );

    if( resultSpy1.count() != 1 ) return Meta::TrackList();
    if( doneSpy1.count() != 1 ) return Meta::TrackList();

    QList<QVariant> args1 = resultSpy1.takeFirst();
    if( !args1.value(0).canConvert<Meta::TrackList>() ) return Meta::TrackList();

    delete qm;

    return args1.value(0).value<Meta::TrackList>();
}


void
TestMemoryQueryMaker::testFilterTitle()
{
    Meta::TrackList tracks;

    // -- just get all the tracks
    Collections::MemoryQueryMaker *qm = new Collections::MemoryQueryMaker( m_mc.toWeakRef(), QStringLiteral("test") );
    tracks = executeQueryMaker( qm );
    QCOMPARE( tracks.count(), 3 );

    // -- filter for title
    qm = new Collections::MemoryQueryMaker( m_mc.toWeakRef(), QStringLiteral("test") );
    qm->addFilter( Meta::valTitle, QStringLiteral("Skater"), true, false );
    tracks = executeQueryMaker( qm );
    QCOMPARE( tracks.count(), 1 );
    QCOMPARE( tracks.first()->name(), QStringLiteral("Skater Boy" ) );

    // -- filter for album
    qm = new Collections::MemoryQueryMaker( m_mc.toWeakRef(), QStringLiteral("test") );
    qm->addFilter( Meta::valAlbum, QStringLiteral("S"), false, false );
    tracks = executeQueryMaker( qm );
    QCOMPARE( tracks.count(), 1 );
    QCOMPARE( tracks.first()->name(), QStringLiteral("Substitute" ) );

    // -- filter for artist
    qm = new Collections::MemoryQueryMaker( m_mc.toWeakRef(), QStringLiteral("test") );
    qm->addFilter( Meta::valArtist, QStringLiteral("Lavigne"), false, true );
    tracks = executeQueryMaker( qm );
    QCOMPARE( tracks.count(), 1 );
    QCOMPARE( tracks.first()->name(), QStringLiteral("Skater Boy" ) );
}

void
TestMemoryQueryMaker::testFilterRating()
{
    Meta::TrackList tracks;
    Collections::MemoryQueryMaker *qm = nullptr;

    // -- filter for Rating
    qm = new Collections::MemoryQueryMaker( m_mc.toWeakRef(), QStringLiteral("test") );
    qm->addNumberFilter( Meta::valRating, 3, Collections::QueryMaker::Equals );
    tracks = executeQueryMaker( qm );
    QCOMPARE( tracks.count(), 1 );
    QCOMPARE( tracks.first()->name(), QStringLiteral("Skater Boy" ) );

    // -- filter for Rating
    qm = new Collections::MemoryQueryMaker( m_mc.toWeakRef(), QStringLiteral("test") );
    qm->addNumberFilter( Meta::valRating, 4, Collections::QueryMaker::LessThan );
    tracks = executeQueryMaker( qm );
    QCOMPARE( tracks.count(), 2 );
}

void
TestMemoryQueryMaker::testFilterAnd()
{
    Meta::TrackList tracks;
    Collections::MemoryQueryMaker *qm = nullptr;

    qm = new Collections::MemoryQueryMaker( m_mc.toWeakRef(), QStringLiteral("test") );
    qm->beginAnd();
    qm->addNumberFilter( Meta::valTrackNr, 1, Collections::QueryMaker::Equals );
    qm->addFilter( Meta::valAlbum, QStringLiteral("o"), false, false );
    qm->endAndOr();
    tracks = executeQueryMaker( qm );
    QCOMPARE( tracks.count(), 1 );
    QCOMPARE( tracks.first()->album()->name(), QStringLiteral("Rodeo Radio" ) );
}

void
TestMemoryQueryMaker::testFilterFormat()
{
    Meta::TrackList tracks;
    Collections::MemoryQueryMaker *qm = nullptr;

    // -- filter for title
    qm = new Collections::MemoryQueryMaker( m_mc.toWeakRef(), QStringLiteral("test") );
    qm->addNumberFilter( Meta::valFormat,
                         int(Amarok::Mp3),
                         Collections::QueryMaker::Equals );
    tracks = executeQueryMaker( qm );
    QCOMPARE( tracks.count(), 0 );
}


