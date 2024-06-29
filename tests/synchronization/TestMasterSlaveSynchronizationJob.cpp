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

#include "TestMasterSlaveSynchronizationJob.h"

#include "core/support/Debug.h"
#include "core/collections/CollectionLocation.h"
#include "core/collections/CollectionLocationDelegate.h"
#include "core/support/Components.h"
#include "synchronization/MasterSlaveSynchronizationJob.h"

#include "CollectionTestImpl.h"
#include "core/collections/MockCollectionLocationDelegate.h"
#include "mocks/MockTrack.h"
#include "mocks/MockAlbum.h"
#include "mocks/MockArtist.h"

#include <gmock/gmock.h>

#include <QSignalSpy>


QTEST_GUILESS_MAIN( TestMasterSlaveSynchronizationJob )

using ::testing::Return;
using ::testing::AnyNumber;
using ::testing::_;

static int trackCopyCount;
static int trackRemoveCount;

namespace Collections {

class MyCollectionLocation : public CollectionLocation
{
public:
    Collections::CollectionTestImpl *coll;

    QString prettyLocation() const override { return QStringLiteral("foo"); }
    bool isWritable() const override { return true; }

    void removeUrlsFromCollection( const Meta::TrackList &sources ) override
    {
        trackRemoveCount += sources.count();
        coll->mc->acquireWriteLock();
        TrackMap map = coll->mc->trackMap();
        for( auto const &track : sources )
            map.remove( track->uidUrl() );
        coll->mc->setTrackMap( map );
        coll->mc->releaseLock();
        slotRemoveOperationFinished();
    }

    void copyUrlsToCollection(const QMap<Meta::TrackPtr, QUrl> &sources, const Transcoding::Configuration& conf) override
    {
        Q_UNUSED( conf )
        trackCopyCount = sources.count();
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

TestMasterSlaveSynchronizationJob::TestMasterSlaveSynchronizationJob()
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
TestMasterSlaveSynchronizationJob::init()
{
    trackCopyCount = 0;
    trackRemoveCount = 0;
}

void
TestMasterSlaveSynchronizationJob::testAddTracksToEmptySlave()
{
    Collections::CollectionTestImpl *master = new Collections::MyCollectionTestImpl( QStringLiteral("master") );
    Collections::CollectionTestImpl *slave = new Collections::MyCollectionTestImpl( QStringLiteral("slave") );

    //setup master
    addMockTrack( master, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    QCOMPARE( master->mc->trackMap().count(), 1 );
    QCOMPARE( slave->mc->trackMap().count(), 0 );
    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( trackRemoveCount, 0 );

    MasterSlaveSynchronizationJob *job = new MasterSlaveSynchronizationJob();
    QSignalSpy spy( job, &MasterSlaveSynchronizationJob::destroyed );
    job->setMaster( master );
    job->setSlave( slave );
    job->synchronize();
    spy.wait( 1000 );

    QCOMPARE( trackCopyCount, 1 );
    QCOMPARE( trackRemoveCount, 0 );
    QCOMPARE( master->mc->trackMap().count(), 1 );
    QCOMPARE( slave->mc->trackMap().count(), 1 );
    delete master;
    delete slave;
}

void
TestMasterSlaveSynchronizationJob::testAddSingleTrack()
{
    Collections::CollectionTestImpl *master = new Collections::MyCollectionTestImpl( QStringLiteral("master") );
    Collections::CollectionTestImpl *slave = new Collections::MyCollectionTestImpl( QStringLiteral("slave") );

    //setup
    addMockTrack( master, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( slave, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( master, QStringLiteral("track2"), QStringLiteral("artist1"), QStringLiteral("album1") );

    QCOMPARE( master->mc->trackMap().count(), 2 );
    QCOMPARE( slave->mc->trackMap().count(), 1 );
    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( trackRemoveCount, 0 );

    //test
    MasterSlaveSynchronizationJob *job = new MasterSlaveSynchronizationJob();
    QSignalSpy spy( job, &MasterSlaveSynchronizationJob::destroyed );
    job->setMaster( master );
    job->setSlave( slave );
    job->synchronize();
    spy.wait( 1000 );

    //verify
    QCOMPARE( trackCopyCount, 1 );
    QCOMPARE( trackRemoveCount, 0 );
    QCOMPARE( master->mc->trackMap().count(), 2 );
    QCOMPARE( slave->mc->trackMap().count(), 2 );

    delete master;
    delete slave;
}

void
TestMasterSlaveSynchronizationJob::testAddAlbum()
{
    Collections::CollectionTestImpl *master = new Collections::MyCollectionTestImpl( QStringLiteral("master") );
    Collections::CollectionTestImpl *slave = new Collections::MyCollectionTestImpl( QStringLiteral("slave") );

    //setup
    addMockTrack( master, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( slave, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( master, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album2") );

    QCOMPARE( master->mc->trackMap().count(), 2 );
    QCOMPARE( slave->mc->trackMap().count(), 1 );
    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( trackRemoveCount, 0 );

    //test
    MasterSlaveSynchronizationJob *job = new MasterSlaveSynchronizationJob();
    QSignalSpy spy( job, &MasterSlaveSynchronizationJob::destroyed );
    job->setMaster( master );
    job->setSlave( slave );
    job->synchronize();
    spy.wait( 1000 );

    //verify
    QCOMPARE( trackCopyCount, 1 );
    QCOMPARE( trackRemoveCount, 0 );
    QCOMPARE( master->mc->trackMap().count(), 2 );
    QCOMPARE( slave->mc->trackMap().count(), 2 );

    delete master;
    delete slave;
}

void
TestMasterSlaveSynchronizationJob::testAddArtist()
{
    Collections::CollectionTestImpl *master = new Collections::MyCollectionTestImpl( QStringLiteral("master") );
    Collections::CollectionTestImpl *slave = new Collections::MyCollectionTestImpl( QStringLiteral("slave") );

    //setup
    addMockTrack( master, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( slave, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( master, QStringLiteral("track1"), QStringLiteral("artist2"), QStringLiteral("album1") );

    QCOMPARE( master->mc->trackMap().count(), 2 );
    QCOMPARE( slave->mc->trackMap().count(), 1 );
    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( trackRemoveCount, 0 );

    //test
    MasterSlaveSynchronizationJob *job = new MasterSlaveSynchronizationJob();
    QSignalSpy spy( job, &MasterSlaveSynchronizationJob::destroyed );
    job->setMaster( master );
    job->setSlave( slave );
    job->synchronize();
    spy.wait( 1000 );

    //verify
    QCOMPARE( trackCopyCount, 1 );
    QCOMPARE( trackRemoveCount, 0 );
    QCOMPARE( master->mc->trackMap().count(), 2 );
    QCOMPARE( slave->mc->trackMap().count(), 2 );

    delete master;
    delete slave;
}

void
TestMasterSlaveSynchronizationJob::testRemoveSingleTrack()
{
    Collections::CollectionTestImpl *master = new Collections::MyCollectionTestImpl( QStringLiteral("master") );
    Collections::CollectionTestImpl *slave = new Collections::MyCollectionTestImpl( QStringLiteral("slave") );

    Collections::MockCollectionLocationDelegate *delegate = new Collections::MockCollectionLocationDelegate();
    EXPECT_CALL( *delegate, reallyDelete( _, _) ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    Amarok::Components::setCollectionLocationDelegate( delegate );

    //setup
    addMockTrack( master, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( slave, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( slave, QStringLiteral("track2"), QStringLiteral("artist1"), QStringLiteral("album1") );

    QCOMPARE( master->mc->trackMap().count(), 1 );
    QCOMPARE( slave->mc->trackMap().count(), 2 );
    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( trackRemoveCount, 0 );

    //test
    MasterSlaveSynchronizationJob *job = new MasterSlaveSynchronizationJob();
    QSignalSpy spy( job, &MasterSlaveSynchronizationJob::destroyed );
    job->setMaster( master );
    job->setSlave( slave );
    job->synchronize();
    spy.wait( 1000 );

    //verify
    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( trackRemoveCount, 1 );
    QCOMPARE( master->mc->trackMap().count(), 1 );
    QCOMPARE( slave->mc->trackMap().count(), 1 );

    delete master;
    delete slave;
    delete Amarok::Components::setCollectionLocationDelegate( nullptr );
}

void
TestMasterSlaveSynchronizationJob::testRemoveAlbum()
{
    Collections::CollectionTestImpl *master = new Collections::MyCollectionTestImpl( QStringLiteral("master") );
    Collections::CollectionTestImpl *slave = new Collections::MyCollectionTestImpl( QStringLiteral("slave") );

    Collections::MockCollectionLocationDelegate *delegate = new Collections::MockCollectionLocationDelegate();
    EXPECT_CALL( *delegate, reallyDelete( _, _) ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    Amarok::Components::setCollectionLocationDelegate( delegate );

    //setup
    addMockTrack( master, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( slave, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( slave, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album2") );

    QCOMPARE( master->mc->trackMap().count(), 1 );
    QCOMPARE( slave->mc->trackMap().count(), 2 );
    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( trackRemoveCount, 0 );

    //test
    MasterSlaveSynchronizationJob *job = new MasterSlaveSynchronizationJob();
    QSignalSpy spy( job, &MasterSlaveSynchronizationJob::destroyed );
    job->setMaster( master );
    job->setSlave( slave );
    job->synchronize();
    spy.wait( 1000 );

    //verify
    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( trackRemoveCount, 1 );
    QCOMPARE( master->mc->trackMap().count(), 1 );
    QCOMPARE( slave->mc->trackMap().count(), 1 );

    delete master;
    delete slave;
    delete Amarok::Components::setCollectionLocationDelegate( nullptr );
}

void
TestMasterSlaveSynchronizationJob::testRemoveArtist()
{
    Collections::CollectionTestImpl *master = new Collections::MyCollectionTestImpl( QStringLiteral("master") );
    Collections::CollectionTestImpl *slave = new Collections::MyCollectionTestImpl( QStringLiteral("slave") );

    Collections::MockCollectionLocationDelegate *delegate = new Collections::MockCollectionLocationDelegate();
    EXPECT_CALL( *delegate, reallyDelete( _, _) ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    Amarok::Components::setCollectionLocationDelegate( delegate );

    //setup
    addMockTrack( master, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( slave, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( slave, QStringLiteral("track1"), QStringLiteral("artist2"), QStringLiteral("album1") );

    QCOMPARE( master->mc->trackMap().count(), 1 );
    QCOMPARE( slave->mc->trackMap().count(), 2 );
    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( trackRemoveCount, 0 );

    //test
    MasterSlaveSynchronizationJob *job = new MasterSlaveSynchronizationJob();
    QSignalSpy spy( job, &MasterSlaveSynchronizationJob::destroyed );
    job->setMaster( master );
    job->setSlave( slave );
    job->synchronize();
    spy.wait( 1000 );

    //verify
    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( trackRemoveCount, 1 );
    QCOMPARE( master->mc->trackMap().count(), 1 );
    QCOMPARE( slave->mc->trackMap().count(), 1 );

    delete master;
    delete slave;
    delete Amarok::Components::setCollectionLocationDelegate( nullptr );
}

void
TestMasterSlaveSynchronizationJob::testEmptyMaster()
{
    Collections::CollectionTestImpl *master = new Collections::MyCollectionTestImpl( QStringLiteral("master") );
    Collections::CollectionTestImpl *slave = new Collections::MyCollectionTestImpl( QStringLiteral("slave") );

    Collections::MockCollectionLocationDelegate *delegate = new Collections::MockCollectionLocationDelegate();
    EXPECT_CALL( *delegate, reallyDelete( _, _) ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    Amarok::Components::setCollectionLocationDelegate( delegate );

    //setup master
    addMockTrack( slave, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    QCOMPARE( master->mc->trackMap().count(), 0 );
    QCOMPARE( slave->mc->trackMap().count(), 1 );
    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( trackRemoveCount, 0 );

    MasterSlaveSynchronizationJob *job = new MasterSlaveSynchronizationJob();
    QSignalSpy spy( job, &MasterSlaveSynchronizationJob::destroyed );
    job->setMaster( master );
    job->setSlave( slave );
    job->synchronize();
    spy.wait( 1000 );

    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( trackRemoveCount, 1 );
    QCOMPARE( master->mc->trackMap().count(), 0 );
    QCOMPARE( slave->mc->trackMap().count(), 0 );
    delete master;
    delete slave;
    delete Amarok::Components::setCollectionLocationDelegate( nullptr );
}
