/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "TestImportersCommon.h"

#include "CollectionTestImpl.h"
#include "core/meta/Meta.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "databaseimporter/DatabaseImporter.h"
#include "mocks/MetaMock.h"
#include "mocks/MockTrack.h"

#include <qtest_kde.h>

using namespace Collections;

TestImportersCommon::TestImportersCommon()
    : m_localCollection( new CollectionTestImpl( "Metallica" ) )
    , m_fileCollection ( new CollectionTestImpl( "tmp" ) )
{
    qRegisterMetaType<Meta::TrackPtr>();
    qRegisterMetaType<Meta::TrackList>();

    // Registering the fileCollection lets us use CollectionManager::trackForUrl
    // on nonexistant tracks, which is used in importers.
    CollectionManager::instance()->addUnmanagedCollection( m_fileCollection.data(),
                                                CollectionManager::CollectionQueryable );
    CollectionManager::instance()->addUnmanagedCollection( m_localCollection.data(),
                                                  CollectionManager::CollectionEnabled );

    m_trackList << "Enter Sandman" << "Sad but True" << "Holier Than Thou"
                << "The Unforgiven" << "Wherever I May Roam" << "Don't Tread on Me"
                << "Through the Never" << "Nothing Else Matters" << "Of Wolf and Man"
                << "The God That Failed" << "My Friend of Misery"
                << "The Struggle Within";
}

TestImportersCommon::~TestImportersCommon()
{
    CollectionManager::instance()->removeUnmanagedCollection( m_fileCollection.data() );
    CollectionManager::instance()->removeUnmanagedCollection( m_localCollection.data() );
}

void
TestImportersCommon::importFailed()
{
    QFAIL( "Import failed!" );
}

void
TestImportersCommon::blockingImport()
{
    QScopedPointer<DatabaseImporter> importer( newInstance() );
    QEventLoop loop;

    connect( importer.data(), SIGNAL(importSucceeded()), &loop, SLOT(quit()) );
    connect( importer.data(), SIGNAL(importFailed()), &loop, SLOT(quit()) );
    connect( importer.data(), SIGNAL(importError(QString)), &loop, SLOT(quit()) );
    connect( importer.data(), SIGNAL(importFailed()), this, SLOT(importFailed()) );
    connect( importer.data(), SIGNAL(importError(QString)), this, SLOT(importFailed()) );

    importer->startImporting();
    loop.exec();
}

void
TestImportersCommon::setUpTestData()
{
    QTest::addColumn<QString>   ( "title" );
    QTest::addColumn<QDateTime> ( "firstPlayed" );
    QTest::addColumn<QDateTime> ( "lastPlayed" );
    QTest::addColumn<int>       ( "playCount" );
    QTest::addColumn<int>       ( "rating" );
    QTest::addColumn<double>    ( "score" );

    QTest::newRow( "Enter Sandman" )
            << "Enter Sandman" << QDateTime::fromTime_t( 1372416489 )
            << QDateTime::fromTime_t( 1372418184 ) << 4 << 4 << 33.0;
    QTest::newRow( "Sad but True" )
            << "Sad but True" << QDateTime::fromTime_t( 1372417027 )
            << QDateTime::fromTime_t( 1372417063 ) << 2 << 3 << 13.25;
    QTest::newRow( "Holier Than Thou" )
            << "Holier Than Thou" << QDateTime::fromTime_t( 1372417028 )
            << QDateTime::fromTime_t( 1372417055 ) << 2 << 3 << 13.25;
    QTest::newRow( "The Unforgiven" )
            << "The Unforgiven" << QDateTime::fromTime_t( 1372415850 )
            << QDateTime::fromTime_t( 1372419217 ) << 4 << 5 << 43.875;
    QTest::newRow( "Wherever I May Roam" )
            << "Wherever I May Roam" << QDateTime::fromTime_t( 1372417030 )
            << QDateTime::fromTime_t( 1372420415 ) << 3 << 5 << 41.8333;
    QTest::newRow( "Don't Tread on Me" )
            << "Don't Tread on Me" << QDateTime::fromTime_t( 1372417031 )
            << QDateTime::fromTime_t( 1372419457 ) << 3 << 2 << 41.8333;
    QTest::newRow( "Through the Never" )
            << "Through the Never" << QDateTime::fromTime_t( 1372417032 )
            << QDateTime::fromTime_t( 1372419702 ) << 3 << 4 << 41.8333;
    QTest::newRow( "Nothing Else Matters" )
            << "Nothing Else Matters" << QDateTime::fromTime_t( 1372415314 )
            << QDateTime::fromTime_t( 1372418573 ) << 5 << 5 << 45.1;
    QTest::newRow( "Of Wolf and Man" )
            << "Of Wolf and Man" << QDateTime::fromTime_t( 1372416881 )
            << QDateTime::fromTime_t( 1372418830 ) << 4 << 4 << 31.625;
    QTest::newRow( "The God That Failed" )
            << "The God That Failed" << QDateTime::fromTime_t( 1372417035 )
            << QDateTime::fromTime_t( 1372420011 ) << 3 << 2 << 41.8333;
    QTest::newRow( "My Friend of Misery" )
            << "My Friend of Misery" << QDateTime::fromTime_t( 1372417036 )
            << QDateTime::fromTime_t( 1372417060 ) << 3 << 1 << 9.16667;
    QTest::newRow( "The Struggle Within" )
            << "The Struggle Within" << QDateTime::fromTime_t( 1372417042 )
            << QDateTime::fromTime_t( 1372417061 ) << 3 << 3 << 9.66667;
}

void
TestImportersCommon::initTestCase()
{
    QWriteLocker( m_fileCollection->mc->mapLock() );

    Meta::ArtistPtr artist( new MockArtist( "Metallica" ) );
    Meta::AlbumPtr  album ( new MockAlbum( "Metallica", artist ) );
    Meta::YearPtr   year  ( new MockYear( "1991" ) );

    m_fileCollection->mc->addArtist( artist );
    m_fileCollection->mc->addAlbum ( album );
    m_fileCollection->mc->addYear  ( year );

    foreach( const QString &title, m_trackList )
    {
        const QString path( pathForMetadata( artist->name(), album->name(), title ) );

        QVariantMap data;
        data.insert( Meta::Field::TITLE,    title );
        data.insert( Meta::Field::UNIQUEID, path );
        data.insert( Meta::Field::URL,      KUrl( path ) );

        MetaMock *track = new MetaMock( data );
        track->m_album  = album;
        track->m_artist = artist;
        track->m_year   = year;

        Meta::TrackPtr trackPtr( track );
        m_trackForName.insert( title, trackPtr );
        m_fileCollection->mc->addTrack( trackPtr );
    }
}

void
TestImportersCommon::cleanup()
{
    QWriteLocker( m_localCollection->mc->mapLock() );

    m_localCollection->mc->setAlbumMap( AlbumMap() );
    m_localCollection->mc->setArtistMap( ArtistMap() );
    m_localCollection->mc->setComposerMap( ComposerMap() );
    m_localCollection->mc->setGenreMap( GenreMap() );
    m_localCollection->mc->setTrackMap( TrackMap() );
    m_localCollection->mc->setYearMap( YearMap() );
}

void
TestImportersCommon::init()
{
    foreach( Meta::TrackPtr track, m_fileCollection->mc->trackMap() )
    {
        Meta::StatisticsPtr stat = track->statistics();
        stat->beginUpdate();
        stat->setFirstPlayed( QDateTime::fromTime_t( 0 ) );
        stat->setLastPlayed( QDateTime::fromTime_t( 0 ) );
        stat->setPlayCount( 0 );
        stat->setRating( 0 );
        stat->setScore( 0 );
        stat->endUpdate();
    }
}

void
TestImportersCommon::newTracksShouldBeAddedToCollection()
{
    QVERIFY( m_localCollection->mc->trackMap().empty() );

    blockingImport();

    QSet<QString> importedTracks;
    foreach( Meta::TrackPtr track, m_localCollection->mc->trackMap() )
        importedTracks.insert( track->name() );

    QCOMPARE( importedTracks.size(), m_trackList.size() );
    foreach( const QString &track, m_trackList )
        QVERIFY( importedTracks.contains( track ) );

    QFETCH( QString, title );
    Meta::StatisticsPtr stat = m_trackForName[title]->statistics();
    QTEST( stat->lastPlayed(),  "lastPlayed" );
    QTEST( stat->playCount(),   "playCount" );
    QTEST( stat->rating(),      "rating" );
}

void
TestImportersCommon::importedTracksShouldOverwriteEmptyStatistics()
{
    m_localCollection->mc->setTrackMap( m_fileCollection->mc->trackMap() );
    blockingImport();

    QFETCH( QString, title );
    Meta::StatisticsPtr stat = m_trackForName[title]->statistics();
    QTEST( stat->lastPlayed(),  "lastPlayed" );
    QTEST( stat->playCount(),   "playCount" );
    QTEST( stat->rating(),      "rating" );
}

void
TestImportersCommon::importingShouldUseHigherPlaycount()
{
    Meta::TrackPtr track = m_fileCollection->mc->trackMap().values().first();
    m_localCollection->mc->addTrack( track );

    // Every track in the database has higher playcount than 0
    track->statistics()->setPlayCount( 0 );
    blockingImport();
    QVERIFY( track->statistics()->playCount() > 0 );

    track->statistics()->setPlayCount( 1024 );
    blockingImport();
    QEXPECT_FAIL( "", "To be fixed when importers are rewritten on StatSyncing", Continue );
    QCOMPARE( track->statistics()->playCount(), 1024 );
}
