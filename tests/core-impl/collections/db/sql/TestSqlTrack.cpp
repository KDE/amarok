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

#include "amarokconfig.h"
#include "DefaultSqlQueryMakerFactory.h"
#include "core/meta/Meta.h"
#include "core-impl/storage/sql/mysqlestorage/MySqlEmbeddedStorage.h"

#include "SqlCollection.h"
#include "SqlMeta.h"
#include "SqlRegistry.h"
#include "SqlMountPointManagerMock.h"

#include "MetaNotificationSpy.h"

#include <QDateTime>
#include <QSignalSpy>


QTEST_GUILESS_MAIN( TestSqlTrack )

QTemporaryDir *TestSqlTrack::s_tmpDir = nullptr;

TestSqlTrack::TestSqlTrack()
    : QObject()
    , m_collection( nullptr )
    , m_storage( nullptr )
{
}

void
TestSqlTrack::initTestCase()
{
    AmarokConfig::instance(QStringLiteral("amarokrc"));

    s_tmpDir = new QTemporaryDir();
    m_storage = QSharedPointer<MySqlEmbeddedStorage>( new MySqlEmbeddedStorage() );
    QVERIFY( m_storage->init( s_tmpDir->path() ) );
    m_collection = new Collections::SqlCollection( m_storage );
    m_collection->setMountPointManager( new SqlMountPointManagerMock( this, m_storage ) );

    // I just need the table and not the whole playlist manager
    m_storage->query( QStringLiteral("CREATE TABLE playlist_tracks ("
            " id ") + m_storage->idType() +
            QStringLiteral(", playlist_id INTEGER "
            ", track_num INTEGER "
            ", url ") + m_storage->exactTextColumnType() +
            QStringLiteral(", title ") + m_storage->textColumnType() +
            QStringLiteral(", album ") + m_storage->textColumnType() +
            QStringLiteral(", artist ") + m_storage->textColumnType() +
            QStringLiteral(", length INTEGER "
            ", uniqueid ") + m_storage->textColumnType(128) + QStringLiteral(") ENGINE = MyISAM;" ) );

}

void
TestSqlTrack::cleanupTestCase()
{
    delete m_collection;
    //m_storage is deleted by SqlCollection
}

void
TestSqlTrack::init()
{
    //setup base data
    m_storage->query( QStringLiteral("INSERT INTO artists(id, name) VALUES (1, 'artist1');") );
    m_storage->query( QStringLiteral("INSERT INTO artists(id, name) VALUES (2, 'artist2');") );
    m_storage->query( QStringLiteral("INSERT INTO artists(id, name) VALUES (3, 'artist3');") );

    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES(1,'album1',1);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES(2,'album2',1);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES(3,'album3',2);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES(4,'album-compilation',0);") );

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
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES (4, -1, './NothingHere.mp3', 1, '4');") );

    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(1,1,'track1','comment1',1,1,1,1,1);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(2,2,'track2','comment2',1,2,1,1,1);") );

    m_collection->registry()->emptyCache();
}

void
TestSqlTrack::cleanup()
{
    m_storage->query( QStringLiteral("TRUNCATE TABLE years;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE genres;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE composers;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE albums;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE artists;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE tracks;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE urls;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE directories;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE statistics;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE labels;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE urls_labels;") );
}


void
TestSqlTrack::setAllValues( Meta::SqlTrack *track )
{
    track->setTitle( "New Title" );
    track->setAlbum( "New Album" );
    track->setArtist( "New Artist" );
    track->setComposer( "New Composer" );
    track->setYear( 1999 );
    track->setGenre( "New Genre" );

    track->setUrl( -1, "./new_url", 2 );

    track->setBpm( 32.0 );
    track->setComment( "New Comment" );

    track->setScore( 64.0 );
    track->setRating( 5 );

    track->setLength( 5000 );
    track->setSampleRate( 4400 );
    track->setBitrate( 128 );

    track->setTrackNumber( 4 );
    track->setDiscNumber( 1 );

    track->setFirstPlayed( QDateTime::fromSecsSinceEpoch(100) );
    track->setLastPlayed( QDateTime::fromSecsSinceEpoch(200) );
    track->setPlayCount( 20 );

    Meta::ReplayGainTag modes[] = { Meta::ReplayGain_Track_Gain,
        Meta::ReplayGain_Track_Peak,
        Meta::ReplayGain_Album_Gain,
        Meta::ReplayGain_Album_Peak };

    for( int i=0; i<4; i++ )
        track->setReplayGain( modes[i], qreal(i) );

    track->addLabel( "New Label" );
}

void
TestSqlTrack::getAllValues( Meta::SqlTrack *track )
{
    QCOMPARE( track->name(), QStringLiteral( "New Title" ) );
    QCOMPARE( track->album()->name(), QStringLiteral( "New Album" ) );
    QCOMPARE( track->artist()->name(), QStringLiteral( "New Artist" ) );
    QCOMPARE( track->composer()->name(), QStringLiteral( "New Composer" ) );
    QCOMPARE( track->year()->name(), QStringLiteral( "1999" ) );
    QCOMPARE( track->genre()->name(), QStringLiteral( "New Genre" ) );

    QCOMPARE( track->playableUrl().path(), QStringLiteral( "/new_url" ) );
    QCOMPARE( track->bpm(), 32.0 );
    QCOMPARE( track->comment(), QStringLiteral( "New Comment" ) );

    QCOMPARE( track->score(), 64.0 );
    QCOMPARE( track->rating(), 5 );

    QCOMPARE( track->length(), qint64(5000) );
    QCOMPARE( track->sampleRate(), 4400 );
    QCOMPARE( track->bitrate(), 128 );

    QCOMPARE( track->trackNumber(), 4 );
    QCOMPARE( track->discNumber(), 1 );

    QCOMPARE( track->firstPlayed(), QDateTime::fromSecsSinceEpoch(100) );
    QCOMPARE( track->lastPlayed(), QDateTime::fromSecsSinceEpoch(200) );
    QCOMPARE( track->playCount(), 20 );

    Meta::ReplayGainTag modes[] = { Meta::ReplayGain_Track_Gain,
        Meta::ReplayGain_Track_Peak,
        Meta::ReplayGain_Album_Gain,
        Meta::ReplayGain_Album_Peak };

    for( int i=0; i<4; i++ )
            QCOMPARE( track->replayGain( modes[i] ), qreal(i) );

    QVERIFY( track->labels().count() > 0 );
    QVERIFY( track->labels().contains( m_collection->registry()->getLabel("New Label") ) );
}

/** Check that the registry always returns the same track pointer */
void
TestSqlTrack::testGetTrack()
{
    {
        Meta::TrackPtr track1 = m_collection->registry()->getTrack( 1 );
        Meta::TrackPtr track2 = m_collection->registry()->getTrack( "/IDoNotExist.mp3" );
        Meta::TrackPtr track3 = m_collection->registry()->getTrackFromUid( "1" );

        QVERIFY( track1 );
        QVERIFY( track1 == track2 );
        QVERIFY( track1 == track3 );
    }

    // and also after empty cache
    m_collection->registry()->emptyCache();

    // changed order...
    {
        Meta::TrackPtr track2 = m_collection->registry()->getTrack( "/IDoNotExist.mp3" );
        Meta::TrackPtr track3 = m_collection->registry()->getTrackFromUid( "1" );
        Meta::TrackPtr track1 = m_collection->registry()->getTrack( 1 );

        QVERIFY( track1 );
        QVERIFY( track1 == track2 );
        QVERIFY( track1 == track3 );
    }

    // do again creating a new track
    cleanup();
    m_collection->registry()->emptyCache();

    // changed order...
    {
        Meta::TrackPtr track1 = m_collection->registry()->getTrack( -1, "./newTrack.mp3", 2, "amarok-sqltrackuid://newuid" );
        Meta::SqlTrack *sqlTrack1 = static_cast<Meta::SqlTrack*>( track1.data() );
        sqlTrack1->setBpm( 100 ); // have to commit the new track

        QVERIFY( track1 );
        QCOMPARE( track1->playableUrl().path(), QStringLiteral("/newTrack.mp3" ));
        QCOMPARE( track1->uidUrl(), QStringLiteral("amarok-sqltrackuid://newuid" ));
    }

    m_collection->registry()->emptyCache();

    // changed order...
    {
        Meta::TrackPtr track1 = m_collection->registry()->getTrackFromUid("amarok-sqltrackuid://newuid");

        QVERIFY( track1 );
        QCOMPARE( track1->playableUrl().path(), QStringLiteral("/newTrack.mp3" ));
        QCOMPARE( track1->uidUrl(), QStringLiteral("amarok-sqltrackuid://newuid" ));
        QCOMPARE( track1->bpm(), 100.0 );
    }
}

void
TestSqlTrack::testSetAllValuesSingleNotExisting()
{
    {
        // get a new track
        Meta::TrackPtr track1 = m_collection->registry()->getTrack( -1, QStringLiteral("./IamANewTrack.mp3"), 1, "1e34fb213489" );

        QSignalSpy spy( m_collection, &Collections::SqlCollection::updated);
        MetaNotificationSpy metaSpy;
        metaSpy.subscribeTo( track1 );

        Meta::SqlTrack *sqlTrack1 = static_cast<Meta::SqlTrack*>( track1.data() );
        setAllValues( sqlTrack1 );
        getAllValues( sqlTrack1 );

        // new track should have an up-to-date create time (not more than 3 seconds old)
        QVERIFY( track1->createDate().secsTo(QDateTime::currentDateTime()) < 3 );

        QVERIFY( metaSpy.notificationsFromTracks().count() > 1 ); // we should be notified about the changes
    }

    // and also after empty cache
    m_collection->registry()->emptyCache();

    {
        Meta::TrackPtr track1 = m_collection->registry()->getTrack( QStringLiteral("/new_url") );
        Meta::SqlTrack *sqlTrack1 = static_cast<Meta::SqlTrack*>( track1.data() );
        QVERIFY( track1 );
        getAllValues( sqlTrack1 );
    }
}

/** Set all track values but before that create them in the registry. */
void
TestSqlTrack::testSetAllValuesSingleExisting()
{
    {
        Meta::GenrePtr    genre    = m_collection->registry()->getGenre( "New Genre" );
        Meta::ComposerPtr composer = m_collection->registry()->getComposer( "New Composer" );
        Meta::YearPtr     year     = m_collection->registry()->getYear( 1999 );
        Meta::AlbumPtr    album    = m_collection->registry()->getAlbum( "New Album", "New Artist" );
        m_collection->registry()->getLabel( "New Label" );

        Meta::TrackPtr track1 = m_collection->registry()->getTrack( QStringLiteral("/IDoNotExist.mp3") );

        Meta::SqlTrack *sqlTrack1 = static_cast<Meta::SqlTrack*>( track1.data() );
        setAllValues( sqlTrack1 );
        getAllValues( sqlTrack1 );

        // check that the existing object are really updated with the new tracklist
        QCOMPARE( genre->tracks().count(), 1 );
        QCOMPARE( genre->tracks().first().data(), track1.data() );

        QCOMPARE( composer->tracks().count(), 1 );
        QCOMPARE( composer->tracks().first().data(), track1.data() );

        QCOMPARE( year->tracks().count(), 1 );
        QCOMPARE( year->tracks().first().data(), track1.data() );

        // the logic, how renaming the track artist influences its album is still
        // unfinished. For sure the track must be in an album with the defined
        // name
        QCOMPARE( sqlTrack1->album()->name(), QStringLiteral("New Album") );
        QCOMPARE( sqlTrack1->album()->tracks().count(), 1 );
        QCOMPARE( sqlTrack1->album()->tracks().first().data(), track1.data() );
    }

    // and also after empty cache
    m_collection->registry()->emptyCache();

    {
        Meta::TrackPtr track1 = m_collection->registry()->getTrack( QStringLiteral("/new_url") );
        Meta::SqlTrack *sqlTrack1 = static_cast<Meta::SqlTrack*>( track1.data() );
        QVERIFY( track1 );
        getAllValues( sqlTrack1 );

        Meta::GenrePtr    genre    = m_collection->registry()->getGenre( "New Genre" );
        Meta::ComposerPtr composer = m_collection->registry()->getComposer( "New Composer" );
        Meta::YearPtr     year     = m_collection->registry()->getYear( 1999 );
        Meta::AlbumPtr    album    = m_collection->registry()->getAlbum( "New Album", "New Artist" );

        // check that the existing object are really updated with the new tracklist
        QCOMPARE( genre->tracks().count(), 1 );
        QCOMPARE( genre->tracks().first().data(), track1.data() );

        QCOMPARE( composer->tracks().count(), 1 );
        QCOMPARE( composer->tracks().first().data(), track1.data() );

        QCOMPARE( year->tracks().count(), 1 );
        QCOMPARE( year->tracks().first().data(), track1.data() );

        // the logic, how renaming the track artist influences its album is still
        // unfinished. For sure the track must be in an album with the defined
        // name
        QCOMPARE( sqlTrack1->album()->name(), QStringLiteral("New Album") );
        QCOMPARE( sqlTrack1->album()->tracks().count(), 1 );
        QCOMPARE( sqlTrack1->album()->tracks().first().data(), track1.data() );
    }
}

void
TestSqlTrack::testSetAllValuesBatch()
{
    {
        Meta::TrackPtr track1 = m_collection->registry()->getTrack( QStringLiteral("/IDoNotExist.mp3") );
        Meta::SqlTrack *sqlTrack1 = static_cast<Meta::SqlTrack*>( track1.data() );

        QSignalSpy spy( m_collection, &Collections::SqlCollection::updated);
        MetaNotificationSpy metaSpy;
        metaSpy.subscribeTo( track1 );

        sqlTrack1->beginUpdate();

        setAllValues( sqlTrack1 );
        QCOMPARE( metaSpy.notificationsFromTracks().count(), 1 ); // add label does one notify

        sqlTrack1->endUpdate();
        QCOMPARE( metaSpy.notificationsFromTracks().count(), 2 ); // only one notificate for all the changes

        getAllValues( sqlTrack1 );
    }

    // and also after empty cache
    m_collection->registry()->emptyCache();

    {
        Meta::TrackPtr track1 = m_collection->registry()->getTrack( QStringLiteral("/new_url") );
        Meta::SqlTrack *sqlTrack1 = static_cast<Meta::SqlTrack*>( track1.data() );
        QVERIFY( track1 );
        getAllValues( sqlTrack1 );
    }
}

void
TestSqlTrack::testUnsetValues()
{
    {
        Meta::TrackPtr track1 = m_collection->registry()->getTrack( QStringLiteral("/IDoNotExist.mp3") );
        Meta::SqlTrack *sqlTrack1 = static_cast<Meta::SqlTrack*>( track1.data() );

        setAllValues( sqlTrack1 );

        // now unset the values again
        sqlTrack1->setAlbum( "" );
        sqlTrack1->setArtist( "" );
        sqlTrack1->setComposer( "" );
        sqlTrack1->setYear( 0 ); // it is not clear what an empty year exactly is
        sqlTrack1->setGenre( "" );

        // note: Amarok is still not clear if an empty artist means track->artist() == 0
        QVERIFY( !track1->album() || track1->album()->name().isEmpty() );
        QVERIFY( !track1->artist() || track1->artist()->name().isEmpty() );
        QVERIFY( !track1->composer() || track1->composer()->name().isEmpty() );
        QVERIFY( !track1->year() || track1->year()->year() == 0 );
        QVERIFY( !track1->genre() || track1->genre()->name().isEmpty() );
    }

    // and also after empty cache
    m_collection->registry()->emptyCache();

    {
        Meta::TrackPtr track1 = m_collection->registry()->getTrack( QStringLiteral("/new_url") );
        QVERIFY( track1 );
        QVERIFY( !track1->album() || track1->album()->name().isEmpty() );
        QVERIFY( !track1->artist() || track1->artist()->name().isEmpty() );
        QVERIFY( !track1->composer() || track1->composer()->name().isEmpty() );
        QVERIFY( !track1->year() || track1->year()->year() == 0 );
        QVERIFY( !track1->genre() || track1->genre()->name().isEmpty() );
    }
}

void
TestSqlTrack::testFinishedPlaying()
{
    Meta::TrackPtr track1 = m_collection->registry()->getTrack( QStringLiteral("/IDoNotExist.mp3") );
    Meta::SqlTrack *sqlTrack1 = static_cast<Meta::SqlTrack*>( track1.data() );

    sqlTrack1->setLength( 5000 );

    QCOMPARE( sqlTrack1->score(),       0.0 );
    QCOMPARE( sqlTrack1->playCount(),   0 );
    QVERIFY( !sqlTrack1->firstPlayed().isValid() );
    QVERIFY( !sqlTrack1->lastPlayed().isValid() );

    // now play the track not really
    sqlTrack1->finishedPlaying( 0.1 );

    // can't do a statement about the score here
    QCOMPARE( sqlTrack1->playCount(),   0 );
    QVERIFY( !sqlTrack1->firstPlayed().isValid() );
    QVERIFY( !sqlTrack1->lastPlayed().isValid() );

    // and now really play it
    sqlTrack1->finishedPlaying( 1.0 );

    QVERIFY(  sqlTrack1->score() > 0.0 );
    QCOMPARE( sqlTrack1->playCount(),   1 );
    QVERIFY(  sqlTrack1->firstPlayed().secsTo( QDateTime::currentDateTime() ) < 2 );
    QVERIFY(  sqlTrack1->lastPlayed().secsTo( QDateTime::currentDateTime() ) < 2 );
}


void
TestSqlTrack::testAlbumRemaingsNonCompilationAfterChangingAlbumName()
{
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (3,3,'track1',1,1,1,1,1 );") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (4,4,'track2',1,1,1,1,1 );") );

    Meta::TrackPtr track1 = m_collection->registry()->getTrack( 3 );
    Meta::TrackPtr track2 = m_collection->registry()->getTrack( 4 );

    QCOMPARE( track1->album()->name(), QStringLiteral( "album1" ) );
    QVERIFY( track1->album()->hasAlbumArtist() );
    QCOMPARE( track1->album().data(), track2->album().data() );

    Meta::SqlTrack *sqlTrack1 = static_cast<Meta::SqlTrack*>( track1.data() );
    sqlTrack1->setAlbum( QStringLiteral("album2") );
    Meta::SqlTrack *sqlTrack2 = static_cast<Meta::SqlTrack*>( track2.data() );
    sqlTrack2->beginUpdate();
    sqlTrack2->setAlbum( QStringLiteral("album2") );
    sqlTrack2->endUpdate();

    QCOMPARE( track1->album()->name(), QStringLiteral( "album2" ) );
    QVERIFY( track1->album()->hasAlbumArtist() );
    QVERIFY( track1->album() == track2->album() );
}

void
TestSqlTrack::testAlbumRemainsCompilationAfterChangingAlbumName()
{
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (3,3,'track1',1,4,1,1,1 );") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (4,4,'track2',1,4,1,1,1 );") );

    Meta::TrackPtr track1 = m_collection->registry()->getTrack( 3 );
    Meta::TrackPtr track2 = m_collection->registry()->getTrack( 4 );

    QVERIFY( track1 );
    QVERIFY( track1->album() );
    QVERIFY( track2 );
    QVERIFY( track2->album() );
    QCOMPARE( track1->album()->name(), QStringLiteral( "album-compilation" ) );
    QVERIFY( track1->album()->isCompilation() );
    QVERIFY( track1->album().data() == track2->album().data() );

    Meta::SqlTrack *sqlTrack1 = static_cast<Meta::SqlTrack*>( track1.data() );
    Meta::SqlTrack *sqlTrack2 = static_cast<Meta::SqlTrack*>( track2.data() );
    sqlTrack1->setAlbum( QStringLiteral("album2") );
    sqlTrack2->beginUpdate();
    sqlTrack2->setAlbum( QStringLiteral("album2") );
    sqlTrack2->endUpdate();

    QCOMPARE( track1->album()->name(), QStringLiteral( "album2" ) );
    QVERIFY( track1->album()->isCompilation() );
    QVERIFY( track1->album() == track2->album() );
}

void
TestSqlTrack::testRemoveLabelFromTrack()
{
    Meta::TrackPtr track = m_collection->registry()->getTrack( QStringLiteral("/IDoNotExist.mp3") );
    Meta::LabelPtr label = m_collection->registry()->getLabel( QStringLiteral("A") );
    track->addLabel( label );
    QCOMPARE( track->labels().count(), 1 );

    track->removeLabel( label );
    QCOMPARE( track->labels().count(), 0 );

    QStringList urlsLabelsCount = m_storage->query( QStringLiteral("SELECT COUNT(*) FROM urls_labels;") );
    QCOMPARE( urlsLabelsCount.first().toInt(), 0 );
}

void
TestSqlTrack::testRemoveLabelFromTrackWhenNotInCache()
{
    m_storage->query( QStringLiteral("INSERT INTO labels(id,label) VALUES (1,'A');") );
    m_storage->query( QStringLiteral("INSERT INTO urls_labels(url,label) VALUES (1,1);") );

    Meta::TrackPtr track = m_collection->registry()->getTrack( QStringLiteral("/IDoNotExist.mp3") );
    Meta::LabelPtr label = m_collection->registry()->getLabel( QStringLiteral("A") );

    track->removeLabel( label );
    QCOMPARE( track->labels().count(), 0 );

    QStringList urlsLabelsCount = m_storage->query( QStringLiteral("SELECT COUNT(*) FROM urls_labels;") );
    QCOMPARE( urlsLabelsCount.first().toInt(), 0 );
}

