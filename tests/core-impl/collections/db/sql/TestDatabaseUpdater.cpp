/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "TestDatabaseUpdater.h"

#include "SqlCollection.h"
#include "DatabaseUpdater.h"
#include "core-impl/storage/sql/mysqlestorage/MySqlEmbeddedStorage.h"

#include <QString>
#include <QStringList>
#include <QTemporaryDir>


QTEST_MAIN( DatabaseUpdaterTest )

QTemporaryDir *DatabaseUpdaterTest::s_tmpDir = nullptr;

DatabaseUpdaterTest::DatabaseUpdaterTest()
    : QObject()
{
    std::atexit([]() { delete DatabaseUpdaterTest::s_tmpDir; } );
}

void
DatabaseUpdaterTest::initTestCase()
{
    if( !s_tmpDir )
        s_tmpDir = new QTemporaryDir();
    QVERIFY( s_tmpDir->isValid() );
    m_storage = QSharedPointer<MySqlEmbeddedStorage>( new MySqlEmbeddedStorage() );
    QVERIFY( m_storage->init( s_tmpDir->path() ) );
    m_collection = new Collections::SqlCollection( m_storage );
}

void
DatabaseUpdaterTest::cleanupTestCase()
{
    delete m_collection;
}

void
DatabaseUpdaterTest::cleanup()
{
    m_storage->query( QStringLiteral("BEGIN") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE tracks;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE albums;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE artists;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE composers;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE genres;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE years;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE urls;") );
    m_storage->query( QStringLiteral("TRUNCATE TABLE directories;") );
    m_storage->query( QStringLiteral("COMMIT") );
}

void
DatabaseUpdaterTest::testNeedsUpdate()
{
    // SqlCollection updates the table by itself
    DatabaseUpdater updater( m_collection );

    m_storage->query( QStringLiteral( "UPDATE admin SET version = %1 WHERE component = 'DB_VERSION';" ).arg( updater.expectedDatabaseVersion() - 1 ) );

    QVERIFY( updater.needsUpdate() );
    QVERIFY( updater.update() );
    QVERIFY( !updater.needsUpdate() );
}

void
DatabaseUpdaterTest::testNeedsNoUpdate()
{
    // SqlCollection updates the table by itself
    DatabaseUpdater updater( m_collection );

    QVERIFY( !updater.needsUpdate() );
    QVERIFY( !updater.update() );
}

void
DatabaseUpdaterTest::testDeleteAllRedundant()
{
    //setup base data
    m_storage->query( QStringLiteral("INSERT INTO artists(id, name) VALUES (1, 'trackArtist');") );
    m_storage->query( QStringLiteral("INSERT INTO artists(id, name) VALUES (2, 'albumArtist');") );
    m_storage->query( QStringLiteral("INSERT INTO artists(id, name) VALUES (3, 'invalidArtist');") );

    m_storage->query( QStringLiteral("INSERT INTO composers(id, name) VALUES (1, 'composer');") );
    m_storage->query( QStringLiteral("INSERT INTO composers(id, name) VALUES (2, 'invalidComposer');") );
    m_storage->query( QStringLiteral("INSERT INTO genres(id, name) VALUES (1, 'genre');") );
    m_storage->query( QStringLiteral("INSERT INTO genres(id, name) VALUES (2, 'invalidGenre');") );
    m_storage->query( QStringLiteral("INSERT INTO years(id, name) VALUES (1, '1');") );
    m_storage->query( QStringLiteral("INSERT INTO years(id, name) VALUES (2, '2');") );

    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (1, 'album1', 2);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES (2, 'albumInvalidAlbum',1);") );

    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, uniqueid ) VALUES (1, -1, './track1.mp3', 'uid://1');") );
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, uniqueid ) VALUES (2, -1, './invalidTrack.mp3', 'uid://2');") );

    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,artist,album,genre,year,composer) "
                      "VALUES (1,1,'track1',1,1,1,1,1 );") );

    DatabaseUpdater updater( m_collection );
    updater.deleteAllRedundant(QStringLiteral("album"));
    updater.deleteAllRedundant(QStringLiteral("artist"));
    updater.deleteAllRedundant(QStringLiteral("genre"));
    updater.deleteAllRedundant(QStringLiteral("composer"));
    updater.deleteAllRedundant(QStringLiteral("url"));
    updater.deleteAllRedundant(QStringLiteral("year"));

    QStringList count;
    count = m_storage->query( QStringLiteral("SELECT COUNT(*) FROM albums;") );
    QCOMPARE( count.first().toInt(), 1 );
    count = m_storage->query( QStringLiteral("SELECT COUNT(*) FROM artists;") );
    QCOMPARE( count.first().toInt(), 2 );
    count = m_storage->query( QStringLiteral("SELECT COUNT(*) FROM genres;") );
    QCOMPARE( count.first().toInt(), 1 );
    count = m_storage->query( QStringLiteral("SELECT COUNT(*) FROM composers;") );
    QCOMPARE( count.first().toInt(), 1 );
    count = m_storage->query( QStringLiteral("SELECT COUNT(*) FROM urls;") );
    QCOMPARE( count.first().toInt(), 1 );
    count = m_storage->query( QStringLiteral("SELECT COUNT(*) FROM years;") );
    QCOMPARE( count.first().toInt(), 1 );
}

void
DatabaseUpdaterTest::testCheckTables()
{
    DatabaseUpdater updater( m_collection );
    updater.checkTables(); // just execute it to get test coverage
}

void
DatabaseUpdaterTest::testCreatePermanentTables()
{
    // actually the collection will call updater.update itself
    // after the update we should have 18 tables

    QStringList tables = m_storage->query( QStringLiteral("select table_name from INFORMATION_SCHEMA.tables WHERE table_schema='amarok'") );
    QCOMPARE( tables.count(), 18 );
}

