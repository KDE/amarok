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

#include "TestOneWaySynchronizationJob.h"

#include "core/support/Debug.h"
#include "core/collections/CollectionLocation.h"
#include "synchronization/OneWaySynchronizationJob.h"

#include "CollectionTestImpl.h"
#include "mocks/MockTrack.h"
#include "mocks/MockAlbum.h"
#include "mocks/MockArtist.h"

#include <KCmdLineArgs>
#include <KGlobal>

#include <qtest_kde.h>

#include <gmock/gmock.h>

QTEST_KDEMAIN_CORE( TestOneWaySynchronizationJob )

using ::testing::Return;
using ::testing::AnyNumber;


static int trackCopyCount;

class MyCollectionLocation : public CollectionLocation
{
public:
    Collections::CollectionTestImpl *coll;

    QString prettyLocation() const { return "foo"; }
    bool isWritable() const { return true; }
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
    void copyUrlsToCollection(const QMap<Meta::TrackPtr, KUrl> &sources)
    {
        qDebug() << "adding " << sources.count() << " tracks to " << coll->collectionId();
        trackCopyCount = sources.count();
        foreach( const Meta::TrackPtr &track, sources.keys() )
        {
            coll->mc->addTrack( track );
        }
    }
};

namespace Collections {

class MyCollectionTestImpl : public CollectionTestImpl
{
public:
    MyCollectionTestImpl( const QString &id ) : CollectionTestImpl( id ) {}

    CollectionLocation* location() const
    {
        MyCollectionLocation *r = new MyCollectionLocation();
        r->coll = const_cast<MyCollectionTestImpl*>( this );
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
    EXPECT_CALL( *track, uidUrl() ).Times( AnyNumber() ).WillRepeatedly( Return( trackName + "_" + artistName + "_" + albumName ) );
    EXPECT_CALL( *track, isPlayable() ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( *track, playableUrl() ).Times( AnyNumber() ).WillRepeatedly( Return( KUrl( '/' + track->uidUrl() ) ) );
    coll->mc->addTrack( trackPtr );

    Meta::AlbumPtr albumPtr = coll->mc->albumMap().value( albumName );
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

TestOneWaySynchronizationJob::TestOneWaySynchronizationJob() : QObject()
{
    KCmdLineArgs::init( KGlobal::activeComponent().aboutData() );
    ::testing::InitGoogleMock( &KCmdLineArgs::qtArgc(), KCmdLineArgs::qtArgv() );
    qRegisterMetaType<Meta::TrackList>();
    qRegisterMetaType<Meta::AlbumList>();
    qRegisterMetaType<Meta::ArtistList>();
}

void
TestOneWaySynchronizationJob::init()
{
    trackCopyCount = 0;
}

void
TestOneWaySynchronizationJob::testAddTrackToTarget()
{
    Collections::CollectionTestImpl *source = new Collections::MyCollectionTestImpl( "source" );
    Collections::CollectionTestImpl *target = new Collections::MyCollectionTestImpl( "target" );

    addMockTrack( source, "track1", "artist1", "album1" );
    addMockTrack( source, "track2", "artist1", "album1" );
    addMockTrack( target, "track1", "artist1", "album1" );

    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( source->mc->trackMap().count(), 2 );
    QCOMPARE( target->mc->trackMap().count(), 1 );

    OneWaySynchronizationJob *job = new OneWaySynchronizationJob();
    job->setSource( source );
    job->setTarget( target );
    job->synchronize();
    QTest::kWaitForSignal( job, SIGNAL(destroyed()), 1000 );

    QCOMPARE( trackCopyCount, 1 );
    QCOMPARE( source->mc->trackMap().count(), 2 );
    QCOMPARE( target->mc->trackMap().count(), 2 );

    delete source,
    delete target;
}

void
TestOneWaySynchronizationJob::testAddAlbumToTarget()
{
    Collections::CollectionTestImpl *source = new Collections::MyCollectionTestImpl( "source" );
    Collections::CollectionTestImpl *target = new Collections::MyCollectionTestImpl( "target" );

    addMockTrack( source, "track1", "artist1", "album1" );
    addMockTrack( source, "track1", "artist1", "album2" );
    addMockTrack( target, "track1", "artist1", "album1" );

    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( source->mc->trackMap().count(), 2 );
    QCOMPARE( target->mc->trackMap().count(), 1 );

    OneWaySynchronizationJob *job = new OneWaySynchronizationJob();
    job->setSource( source );
    job->setTarget( target );
    job->synchronize();
    QTest::kWaitForSignal( job, SIGNAL(destroyed()), 1000 );

    QCOMPARE( trackCopyCount, 1 );
    QCOMPARE( source->mc->trackMap().count(), 2 );
    QCOMPARE( target->mc->trackMap().count(), 2 );

    delete source,
    delete target;
}

void
TestOneWaySynchronizationJob::testAddArtistToTarget()
{
    Collections::CollectionTestImpl *source = new Collections::MyCollectionTestImpl( "source" );
    Collections::CollectionTestImpl *target = new Collections::MyCollectionTestImpl( "target" );

    addMockTrack( source, "track1", "artist1", "album1" );
    addMockTrack( source, "track1", "artist2", "album1" );
    addMockTrack( target, "track1", "artist1", "album1" );

    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( source->mc->trackMap().count(), 2 );
    QCOMPARE( target->mc->trackMap().count(), 1 );

    OneWaySynchronizationJob *job = new OneWaySynchronizationJob();
    job->setSource( source );
    job->setTarget( target );
    job->synchronize();
    QTest::kWaitForSignal( job, SIGNAL(destroyed()), 1000 );

    QCOMPARE( trackCopyCount, 1 );
    QCOMPARE( source->mc->trackMap().count(), 2 );
    QCOMPARE( target->mc->trackMap().count(), 2 );

    delete source,
    delete target;
}

void
TestOneWaySynchronizationJob::testEmptyTarget()
{
    Collections::CollectionTestImpl *source = new Collections::MyCollectionTestImpl( "source" );
    Collections::CollectionTestImpl *target = new Collections::MyCollectionTestImpl( "target" );

    addMockTrack( source, "track1", "artist1", "album1" );
    addMockTrack( source, "track2", "artist1", "album1" );

    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( source->mc->trackMap().count(), 2 );
    QCOMPARE( target->mc->trackMap().count(), 0 );

    OneWaySynchronizationJob *job = new OneWaySynchronizationJob();
    job->setSource( source );
    job->setTarget( target );
    job->synchronize();
    QTest::kWaitForSignal( job, SIGNAL(destroyed()), 1000 );

    QCOMPARE( trackCopyCount, 2 );
    QCOMPARE( source->mc->trackMap().count(), 2 );
    QCOMPARE( target->mc->trackMap().count(), 2 );

    delete source,
    delete target;
}

void
TestOneWaySynchronizationJob::testEmptySourceWithNonEmptyTarget()
{
    Collections::CollectionTestImpl *source = new Collections::MyCollectionTestImpl( "source" );
    Collections::CollectionTestImpl *target = new Collections::MyCollectionTestImpl( "target" );

    addMockTrack( target, "track1", "artist1", "album1" );

    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( source->mc->trackMap().count(), 0 );
    QCOMPARE( target->mc->trackMap().count(), 1 );

    OneWaySynchronizationJob *job = new OneWaySynchronizationJob();
    job->setSource( source );
    job->setTarget( target );
    job->synchronize();
    QTest::kWaitForSignal( job, SIGNAL(destroyed()), 1000 );

    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( source->mc->trackMap().count(), 0 );
    QCOMPARE( target->mc->trackMap().count(), 1 );

    delete source,
    delete target;
}

void
TestOneWaySynchronizationJob::testNoActionNecessary()
{
    Collections::CollectionTestImpl *source = new Collections::MyCollectionTestImpl( "source" );
    Collections::CollectionTestImpl *target = new Collections::MyCollectionTestImpl( "target" );

    addMockTrack( source, "track1", "artist1", "album1" );
    addMockTrack( source, "track2", "artist1", "album1" );
    addMockTrack( target, "track1", "artist1", "album1" );
    addMockTrack( target, "track2", "artist1", "album1" );

    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( source->mc->trackMap().count(), 2 );
    QCOMPARE( target->mc->trackMap().count(), 2 );

    OneWaySynchronizationJob *job = new OneWaySynchronizationJob();
    job->setSource( source );
    job->setTarget( target );
    job->synchronize();
    QTest::kWaitForSignal( job, SIGNAL(destroyed()), 1000 );

    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( source->mc->trackMap().count(), 2 );
    QCOMPARE( target->mc->trackMap().count(), 2 );

    delete source,
    delete target;
}
