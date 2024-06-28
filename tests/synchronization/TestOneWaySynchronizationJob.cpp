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

#include <gmock/gmock.h>

#include <QSignalSpy>

QTEST_GUILESS_MAIN( TestOneWaySynchronizationJob )

using ::testing::Return;
using ::testing::AnyNumber;


static int trackCopyCount;

namespace Collections {

class MyCollectionLocation : public CollectionLocation
{
public:
    Collections::CollectionTestImpl *coll;

    QString prettyLocation() const override { return "foo"; }
    bool isWritable() const override { return true; }
    void copyUrlsToCollection(const QMap<Meta::TrackPtr, QUrl> &sources, const Transcoding::Configuration& conf) override
    {
        Q_UNUSED( conf )
        // qDebug() << "adding " << sources.count() << " tracks to " << coll->collectionId();
        trackCopyCount = sources.count();
        for( Meta::TrackPtr &track : sources.keys() )
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
    EXPECT_CALL( *track, uidUrl() ).Times( AnyNumber() ).WillRepeatedly( Return( trackName + '_' + artistName + '_' + albumName ) );
    EXPECT_CALL( *track, playableUrl() ).Times( AnyNumber() ).WillRepeatedly( Return( QUrl( '/' + track->uidUrl() ) ) );
    EXPECT_CALL( *track, composer() ).Times( AnyNumber() ).WillRepeatedly( Return( Meta::ComposerPtr() ) );
    EXPECT_CALL( *track, genre() ).Times( AnyNumber() ).WillRepeatedly( Return( Meta::GenrePtr() ) );
    EXPECT_CALL( *track, year() ).Times( AnyNumber() ).WillRepeatedly( Return( Meta::YearPtr() ) );
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
    EXPECT_CALL( *album, albumArtist() ).Times( AnyNumber() ).WillRepeatedly( Return( artistPtr ) );
}

TestOneWaySynchronizationJob::TestOneWaySynchronizationJob() : QObject()
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
    QSignalSpy spy( job, &OneWaySynchronizationJob::destroyed );
    job->setSource( source );
    job->setTarget( target );
    job->synchronize();
    spy.wait( 1000 );

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
    QSignalSpy spy( job, &OneWaySynchronizationJob::destroyed );
    job->setSource( source );
    job->setTarget( target );
    job->synchronize();
    spy.wait( 1000 );

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
    QSignalSpy spy( job, &OneWaySynchronizationJob::destroyed );
    job->setSource( source );
    job->setTarget( target );
    job->synchronize();
    spy.wait( 1000 );

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
    QSignalSpy spy( job, &OneWaySynchronizationJob::destroyed );
    job->setSource( source );
    job->setTarget( target );
    job->synchronize();
    spy.wait( 1000 );

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
    QSignalSpy spy( job, &OneWaySynchronizationJob::destroyed );
    job->setSource( source );
    job->setTarget( target );
    job->synchronize();
    spy.wait( 1000 );

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

    addMockTrack( source, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( source, QStringLiteral("track2"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( target, QStringLiteral("track1"), QStringLiteral("artist1"), QStringLiteral("album1") );
    addMockTrack( target, QStringLiteral("track2"), QStringLiteral("artist1"), QStringLiteral("album1") );

    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( source->mc->trackMap().count(), 2 );
    QCOMPARE( target->mc->trackMap().count(), 2 );

    OneWaySynchronizationJob *job = new OneWaySynchronizationJob();
    QSignalSpy spy( job, &OneWaySynchronizationJob::destroyed );
    job->setSource( source );
    job->setTarget( target );
    job->synchronize();
    spy.wait( 1000 );

    QCOMPARE( trackCopyCount, 0 );
    QCOMPARE( source->mc->trackMap().count(), 2 );
    QCOMPARE( target->mc->trackMap().count(), 2 );

    delete source,
    delete target;
}
