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

#include "TestSqlTrackEditing.h"

#include "DatabaseUpdater.h"
#include "core/support/Debug.h"
#include "DefaultSqlQueryMakerFactory.h"
#include "core/meta/Meta.h"
#include "core-impl/meta/file/TagLibUtils.h"
#include "mysqlecollection/MySqlEmbeddedStorage.h"
#include "SqlCollection.h"
#include "SqlMeta.h"
#include "SqlQueryMaker.h"
#include "SqlRegistry.h"
#include "SqlMountPointManagerMock.h"

#include "MetaNotificationSpy.h"

#include <QSignalSpy>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestSqlTrackEditing )

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
    Q_UNUSED( filename )
    Q_UNUSED( changes )
    return;
}

void
Meta::Field::writeFields(TagLib::FileRef fileref, const QVariantMap &changes)
{
    Q_UNUSED( fileref )
    Q_UNUSED( changes )
    return;
}

TestSqlTrackEditing::TestSqlTrackEditing()
    : QObject()
    , m_collection( 0 )
    , m_storage( 0 )
    , m_tmpDir( 0 )
    , m_registry( 0 )
{
}

void
TestSqlTrackEditing::initTestCase()
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
TestSqlTrackEditing::cleanupTestCase()
{
    delete m_collection;
    //m_storage is deleted by SqlCollection
    //m_registry is deleted by SqlCollection
    delete m_tmpDir;

}

void
TestSqlTrackEditing::init()
{
    m_registry = new SqlRegistry( m_collection );
    m_registry->setStorage( m_storage );
    m_collection->setRegistry( m_registry );
    //setup test data
    m_storage->query( "INSERT INTO artists(id, name) VALUES (1, 'artist1');" );
    m_storage->query( "INSERT INTO artists(id, name) VALUES (2, 'artist2');" );
    m_storage->query( "INSERT INTO artists(id, name) VALUES (3, 'artist3');" );

    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(1,'album1',1);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(2,'album2',1);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(3,'album3',2);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(4,'album4',0);" );

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

    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(1,1,'track1','comment1',1,1,1,1,1);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(2,2,'track2','comment2',1,2,1,1,1);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(3,3,'track3','comment3',3,4,1,1,1);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(4,4,'track4','comment4',2,3,3,3,3);" );
}

void
TestSqlTrackEditing::cleanup()
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
TestSqlTrackEditing::testChangeComposerToExisting()
{
    QSignalSpy spy( m_collection, SIGNAL(updated()));
    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::SqlTrack *sqlTrack = static_cast<Meta::SqlTrack*>( track.data() );
    Meta::ComposerPtr originalComposer = track->composer();
    QCOMPARE( track->composer()->name(), QString( "composer1" ) );
    QCOMPARE( track->composer()->tracks().count(), 3 );

    Meta::ComposerPtr targetComposer = m_registry->getComposer( "composer2" );
    QCOMPARE( targetComposer->name(), QString( "composer2" ) );
    QCOMPARE( targetComposer->tracks().count(), 0 );

    MetaNotificationSpy metaSpy;
    metaSpy.subscribeTo( track );

    sqlTrack->setComposer( "composer2" );

    QCOMPARE( metaSpy.notificationsFromTracks().count(), 1 );

    //check data was written to db
    QStringList rs = m_storage->query( "select composer from tracks where id = 1" );
    QCOMPARE( rs.count(), 1);
    QCOMPARE( rs.first(), QString( "2" ) );

    QCOMPARE( track->composer()->name(), QString( "composer2" ) );
    QCOMPARE( originalComposer->tracks().count(), 2 );
    QCOMPARE( track->composer(), targetComposer ); //track returns the new object, not the old one
    QCOMPARE( targetComposer->tracks().count(), 1 );

    QCOMPARE( spy.count(), 1);
}

void
TestSqlTrackEditing::testChangeComposerToNew()
{
    QSignalSpy spy( m_collection, SIGNAL(updated()));
    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::SqlTrack *sqlTrack = static_cast<Meta::SqlTrack*>( track.data() );
    Meta::ComposerPtr originalComposer = track->composer();
    QCOMPARE( track->composer()->name(), QString( "composer1" ) );
    QCOMPARE( track->composer()->tracks().count(), 3 );

    MetaNotificationSpy metaSpy;
    metaSpy.subscribeTo( track );

    sqlTrack->setComposer( "new composer" );

    QCOMPARE( metaSpy.notificationsFromTracks().count(), 1 );

    QCOMPARE( track->composer()->name(), QString( "new composer" ) );
    QCOMPARE( originalComposer->tracks().count(), 2 );

    QVERIFY( track->composer() != originalComposer );

    QCOMPARE( track->composer()->tracks().count(), 1 );

    QStringList count = m_storage->query( "select count(*) from composers where name = 'new composer';" );
    QCOMPARE( count.count(), 1 );
    QCOMPARE( count.first(), QString( "1" ) );

    QStringList composerId = m_storage->query( "select id from composers where name = 'new composer';" );
    QCOMPARE( composerId.count(), 1 );

    QStringList trackComposer = m_storage->query( "select composer from tracks where id = 1;" );
    QCOMPARE( trackComposer.count(), 1 );
    QVERIFY( trackComposer.first() == composerId.first() );

    QCOMPARE( spy.count(), 1);
}

void
TestSqlTrackEditing::testChangeGenreToExisting()
{
    QSignalSpy spy( m_collection, SIGNAL(updated()));
    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::SqlTrack *sqlTrack = static_cast<Meta::SqlTrack*>( track.data() );
    Meta::GenrePtr originalGenre = track->genre();
    QCOMPARE( track->genre()->name(), QString( "genre1" ) );
    QCOMPARE( track->genre()->tracks().count(), 3 );

    Meta::GenrePtr targetGenre = m_registry->getGenre( "genre2" );
    QCOMPARE( targetGenre->name(), QString( "genre2" ) );
    QCOMPARE( targetGenre->tracks().count(), 0 );

    MetaNotificationSpy metaSpy;
    metaSpy.subscribeTo( track );

    sqlTrack->setGenre( "genre2" );

    QCOMPARE( metaSpy.notificationsFromTracks().count(), 1 );

    //check data was written to db
    QStringList rs = m_storage->query( "select genre from tracks where id = 1" );
    QCOMPARE( rs.count(), 1);
    QCOMPARE( rs.first(), QString( "2" ) );

    QCOMPARE( track->genre()->name(), QString( "genre2" ) );
    QCOMPARE( originalGenre->tracks().count(), 2 );
    QCOMPARE( track->genre(), targetGenre ); //track returns the new object, not the old one
    QCOMPARE( targetGenre->tracks().count(), 1 );

    QCOMPARE( spy.count(), 1);
}

void
TestSqlTrackEditing::testChangeGenreToNew()
{
    QSignalSpy spy( m_collection, SIGNAL(updated()));
    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::SqlTrack *sqlTrack = static_cast<Meta::SqlTrack*>( track.data() );
    Meta::GenrePtr originalGenre = track->genre();
    QCOMPARE( track->genre()->name(), QString( "genre1" ) );
    QCOMPARE( track->genre()->tracks().count(), 3 );

    MetaNotificationSpy metaSpy;
    metaSpy.subscribeTo( track );

    sqlTrack->setGenre( "new genre" );

    QCOMPARE( metaSpy.notificationsFromTracks().count(), 1 );

    QCOMPARE( track->genre()->name(), QString( "new genre" ) );
    QCOMPARE( originalGenre->tracks().count(), 2 );

    QVERIFY( track->genre() != originalGenre );

    QCOMPARE( track->genre()->tracks().count(), 1 );

    QStringList count = m_storage->query( "select count(*) from genres where name = 'new genre';" );
    QCOMPARE( count.count(), 1 );
    QCOMPARE( count.first(), QString( "1" ) );

    QStringList genreId = m_storage->query( "select id from genres where name = 'new genre';" );
    QCOMPARE( genreId.count(), 1 );

    QStringList trackGenre = m_storage->query( "select genre from tracks where id = 1;" );
    QCOMPARE( trackGenre.count(), 1 );
    QVERIFY( trackGenre.first() == genreId.first() );

    QCOMPARE( spy.count(), 1);
}

void
TestSqlTrackEditing::testChangeYearToExisting()
{
    QSignalSpy spy( m_collection, SIGNAL(updated()));
    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::SqlTrack *sqlTrack = static_cast<Meta::SqlTrack*>( track.data() );
    Meta::YearPtr originalYear = track->year();
    QCOMPARE( track->year()->name(), QString( "1" ) );
    QCOMPARE( track->year()->tracks().count(), 3 );

    Meta::YearPtr targetYear = m_registry->getYear( "2" );
    QCOMPARE( targetYear->name(), QString( "2" ) );
    QCOMPARE( targetYear->tracks().count(), 0 );

    MetaNotificationSpy metaSpy;
    metaSpy.subscribeTo( track );

    sqlTrack->setYear( "2" );

    QCOMPARE( metaSpy.notificationsFromTracks().count(), 1 );

    //check data was written to db
    QStringList rs = m_storage->query( "select year from tracks where id = 1" );
    QCOMPARE( rs.count(), 1);
    QCOMPARE( rs.first(), QString( "2" ) );

    QCOMPARE( track->year()->name(), QString( "2" ) );
    QCOMPARE( originalYear->tracks().count(), 2 );
    QCOMPARE( track->year(), targetYear ); //track returns the new object, not the old one
    QCOMPARE( targetYear->tracks().count(), 1 );

    QCOMPARE( spy.count(), 1);
}

void
TestSqlTrackEditing::testChangeYearToNew()
{
    QSignalSpy spy( m_collection, SIGNAL(updated()));
    Meta::TrackPtr track = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::SqlTrack *sqlTrack = static_cast<Meta::SqlTrack*>( track.data() );
    Meta::YearPtr originalYear = track->year();
    QCOMPARE( track->year()->name(), QString( "1" ) );
    QCOMPARE( track->year()->tracks().count(), 3 );

    MetaNotificationSpy metaSpy;
    metaSpy.subscribeTo( track );

    sqlTrack->setYear( "2000" );

    QCOMPARE( metaSpy.notificationsFromTracks().count(), 1 );

    QCOMPARE( track->year()->name(), QString( "2000" ) );
    QCOMPARE( originalYear->tracks().count(), 2 );

    QVERIFY( track->year() != originalYear );

    QCOMPARE( track->year()->tracks().count(), 1 );

    QStringList count = m_storage->query( "select count(*) from years where name = '2000';" );
    QCOMPARE( count.count(), 1 );
    QCOMPARE( count.first(), QString( "1" ) );

    QStringList yearId = m_storage->query( "select id from years where name = '2000';" );
    QCOMPARE( yearId.count(), 1 );

    QStringList trackYear = m_storage->query( "select year from tracks where id = 1;" );
    QCOMPARE( trackYear.count(), 1 );
    QVERIFY( trackYear.first() == yearId.first() );

    QCOMPARE( spy.count(), 1);
}

void
TestSqlTrackEditing::testChangeAlbumToExisting()
{
    Meta::TrackPtr track1 = m_registry->getTrack( "/IDoNotExist.mp3" );
    Meta::SqlTrack *sqlTrack = static_cast<Meta::SqlTrack*>( track1.data() );
    Meta::TrackPtr track2 = m_registry->getTrack( "/IDoNotExistAsWell.mp3" );

    Meta::AlbumPtr targetAlbum = track2->album();

    MetaNotificationSpy metaSpy;
    metaSpy.subscribeTo( track1 );

    QCOMPARE( track1->album()->albumArtist(), track2->album()->albumArtist() );
    QCOMPARE( track1->album()->name(), QString( "album1" ) );
    QCOMPARE( track2->album()->name(), QString( "album2" ) );

    sqlTrack->beginMetaDataUpdate();
    sqlTrack->setAlbum( "album2" );
    sqlTrack->endMetaDataUpdate();

    QCOMPARE( track1->album()->name(), QString( "album2" ) );
    QCOMPARE( track1->album(), track2->album() );
    QCOMPARE( track1->album(), targetAlbum );

    QCOMPARE( metaSpy.notificationsFromTracks().count(), 1 );

}

#include "TestSqlTrackEditing.moc"
