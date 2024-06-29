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

#include "TestUnionJob.h"

#include "core/support/Debug.h"
#include "core/collections/CollectionLocation.h"
#include "synchronization/UnionJob.h"

#include "CollectionTestImpl.h"
#include "mocks/MockTrack.h"
#include "mocks/MockAlbum.h"
#include "mocks/MockArtist.h"

#include <QList>
#include <QSignalSpy>

#include <gmock/gmock.h>

QTEST_GUILESS_MAIN( TestUnionJob )

using ::testing::Return;
using ::testing::AnyNumber;

static QList<int> trackCopyCount;

namespace Collections {

class MyCollectionLocation : public CollectionLocation
{
public:
    Collections::CollectionTestImpl *coll;

    QString prettyLocation() const override { return QStringLiteral("foo"); }
    bool isWritable() const override { return true; }
    bool remove( const Meta::TrackPtr &track )
    {
        coll->mc->acquireWriteLock();
        //theoretically we should clean up the other maps as well...
        TrackMap map = coll->mc->trackMap();
        map.remove( track->uidUrl() );
        coll->mc->setTrackMap( map );
        coll->mc->releaseLock();
        return true;
    }
    void copyUrlsToCollection(const QMap<Meta::TrackPtr, QUrl> &sources, const Transcoding::Configuration& conf) override
    {
        Q_UNUSED( conf )
        trackCopyCount << sources.count();
        for( auto const &track : sources.keys() )
        {
            coll->mc->addTrack( track );
        }
    }
};

class MyCollectionTestImpl : public CollectionTestImpl
{
public:
    MyCollectionTestImpl( const QString &id ) : CollectionTestImpl( id ) {}

    CollectionLocation* location() override
    {
        MyCollectionLocation *r = new MyCollectionLocation();
        r->coll = this;
        return r;
    }
};

} //namespace Collections

void addMockTrack( Collections::CollectionTestImpl *coll, const QString &trackName, const QString &artistName, const QString &albumName )
{
    Meta::MockTrack *track = new Meta::MockTrack();
    ::testing::Mock::AllowLeak( track );
    Meta::TrackPtr trackPtr( track );
    EXPECT_CALL( *track, name() ).Times( AnyNumber() ).WillRepeatedly( Return( trackName ) );
    EXPECT_CALL( *track, uidUrl() ).Times( AnyNumber() ).WillRepeatedly( Return( trackName + QLatin1Char('_') + artistName + QLatin1Char('_') + albumName ) );
    EXPECT_CALL( *track, playableUrl() ).Times( AnyNumber() ).WillRepeatedly( Return( QUrl( QLatin1Char('/') + track->uidUrl() ) ) );
    coll->mc->addTrack( trackPtr );

    Meta::AlbumPtr albumPtr = coll->mc->albumMap().value( albumName, QString() /* no album artist */ );
    Meta::MockAlbum *album;
    Meta::TrackList albumTracks;
    if( albumPtr )
    {
        album = dynamic_cast<Meta::MockAlbum*>( albumPtr.data() );
        if( !album )
        {
            QFAIL( "expected a Meta::MockAlbum" );
            return;
        }
        albumTracks = albumPtr->tracks();
    }
    else
    {
        album = new Meta::MockAlbum();
        ::testing::Mock::AllowLeak( album );
        albumPtr = Meta::AlbumPtr( album );
        EXPECT_CALL( *album, name() ).Times( AnyNumber() ).WillRepeatedly( Return( albumName ) );
        EXPECT_CALL( *album, hasAlbumArtist() ).Times( AnyNumber() ).WillRepeatedly( Return( false ) );
        EXPECT_CALL( *album, albumArtist() ).Times( AnyNumber() ).WillRepeatedly( Return( Meta::ArtistPtr() ) );
        coll->mc->addAlbum( albumPtr );
    }
    albumTracks << trackPtr;
    EXPECT_CALL( *album, tracks() ).Times( AnyNumber() ).WillRepeatedly( Return( albumTracks ) );

    EXPECT_CALL( *track, album() ).Times( AnyNumber() ).WillRepeatedly( Return( albumPtr ) );

    Meta::ArtistPtr artistPtr = coll->mc->artistMap().value( artistName );
    Meta::MockArtist *artist;
    Meta::TrackList artistTracks;
    if( artistPtr )
    {
        artist = dynamic_cast<Meta::MockArtist*>( artistPtr.data() );
        if( !artist )
        {
            QFAIL( "expected a Meta::MockArtist" );
            return;
        }
        artistTracks = artistPtr->tracks();
    }
    else
    {
        artist = new Meta::MockArtist();
        ::testing::Mock::AllowLeak( artist );
        artistPtr = Meta::ArtistPtr( artist );
        EXPECT_CALL( *artist, name() ).Times( AnyNumber() ).WillRepeatedly( Return( artistName ) );
        coll->mc->addArtist( artistPtr );
    }
    artistTracks << trackPtr;
    EXPECT_CALL( *artist, tracks() ).Times( AnyNumber() ).WillRepeatedly( Return( artistTracks ) );
    EXPECT_CALL( *track, artist() ).Times( AnyNumber() ).WillRepeatedly( Return( artistPtr ) );
}

TestUnionJob::TestUnionJob() : QObject()
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
TestUnionJob::init()
{
    trackCopyCount.clear();
}

void
TestUnionJob::testEmptyA()
{
    Collections::CollectionTestImpl *collA = new Collections::MyCollectionTestImpl(QStringLiteral("A"));
    Collections::CollectionTestImpl *collB = new Collections::MyCollectionTestImpl(QStringLiteral("B"));

    addMockTrack( collB, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    QCOMPARE( collA->mc->trackMap().count(), 0 );
    QCOMPARE( collB->mc->trackMap().count(), 1 );
    QVERIFY( trackCopyCount.isEmpty() );

    UnionJob *job = new UnionJob( collA, collB );
    QSignalSpy spy( job, &UnionJob::destroyed );
    job->synchronize();
    spy.wait( 1000 );

    QCOMPARE( trackCopyCount.size(), 1 );
    QVERIFY( trackCopyCount.contains( 1 ) );
    QCOMPARE( collA->mc->trackMap().count(), 1 );
    QCOMPARE( collB->mc->trackMap().count(), 1 );

    delete collA;
    delete collB;
}

void
TestUnionJob::testEmptyB()
{
    Collections::CollectionTestImpl *collA = new Collections::MyCollectionTestImpl(QStringLiteral("A"));
    Collections::CollectionTestImpl *collB = new Collections::MyCollectionTestImpl(QStringLiteral("B"));

    addMockTrack( collA, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    QCOMPARE( collA->mc->trackMap().count(), 1 );
    QCOMPARE( collB->mc->trackMap().count(), 0 );
    QVERIFY( trackCopyCount.isEmpty() );

    UnionJob *job = new UnionJob( collA, collB );
    QSignalSpy spy( job, &UnionJob::destroyed );
    job->synchronize();
    spy.wait( 1000 );

    QCOMPARE( trackCopyCount.size(), 1 );
    QVERIFY( trackCopyCount.contains( 1 ) );
    QCOMPARE( collA->mc->trackMap().count(), 1 );
    QCOMPARE( collB->mc->trackMap().count(), 1 );

    delete collA;
    delete collB;
}

void
TestUnionJob::testAddTrackToBoth()
{
    Collections::CollectionTestImpl *collA = new Collections::MyCollectionTestImpl(QStringLiteral("A"));
    Collections::CollectionTestImpl *collB = new Collections::MyCollectionTestImpl(QStringLiteral("B"));

    addMockTrack( collA, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( collB, QStringLiteral("track2"), QStringLiteral("artist2"), QStringLiteral("album2") );
    QCOMPARE( collA->mc->trackMap().count(), 1 );
    QCOMPARE( collB->mc->trackMap().count(), 1 );
    QVERIFY( trackCopyCount.isEmpty() );

    UnionJob *job = new UnionJob( collA, collB );
    QSignalSpy spy( job, &UnionJob::destroyed );
    job->synchronize();
    spy.wait( 1000 );

    QCOMPARE( trackCopyCount.size(), 2 );
    QCOMPARE( trackCopyCount.at( 0 ), 1 );
    QCOMPARE( trackCopyCount.at( 1 ), 1 );
    QCOMPARE( collA->mc->trackMap().count(), 2 );
    QCOMPARE( collB->mc->trackMap().count(), 2 );

    delete collA;
    delete collB;
}

void
TestUnionJob::testTrackAlreadyInBoth()
{
    Collections::CollectionTestImpl *collA = new Collections::MyCollectionTestImpl(QStringLiteral("A"));
    Collections::CollectionTestImpl *collB = new Collections::MyCollectionTestImpl(QStringLiteral("B"));

    addMockTrack( collA, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( collB, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( collB, QStringLiteral("track2"), QStringLiteral("artist2"), QStringLiteral("album2") );
    QCOMPARE( collA->mc->trackMap().count(), 1 );
    QCOMPARE( collB->mc->trackMap().count(), 2 );
    QVERIFY( trackCopyCount.isEmpty() );

    UnionJob *job = new UnionJob( collA, collB );
    QSignalSpy spy( job, &UnionJob::destroyed );
    job->synchronize();
    spy.wait( 1000 );

    QCOMPARE( trackCopyCount.size(), 1 );
    QVERIFY( trackCopyCount.contains( 1 ) );
    QCOMPARE( collA->mc->trackMap().count(), 2 );
    QCOMPARE( collB->mc->trackMap().count(), 2 );

    delete collA;
    delete collB;
}
