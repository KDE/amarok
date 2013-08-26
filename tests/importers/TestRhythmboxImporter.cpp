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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "TestRhythmboxImporter.h"

#include "importers/rhythmbox/RhythmboxConfigWidget.h"
#include "importers/rhythmbox/RhythmboxProvider.h"

#include <QApplication>

#include <qtest_kde.h>

QTEST_KDEMAIN( TestRhythmboxImporter, GUI )

using namespace StatSyncing;

void
TestRhythmboxImporter::init()
{
    // Set a default config
    m_cfg = RhythmboxConfigWidget( QVariantMap() ).config();
}

void
TestRhythmboxImporter::providerShouldHandleInexistantDbFile()
{
    m_cfg.insert( "dbPath", "/wdawd\\wdadwgd/das4hutyf" );

    RhythmboxProvider provider( m_cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestRhythmboxImporter::providerShouldHandleInvalidDbFile()
{
    m_cfg.insert( "dbPath", QApplication::applicationFilePath() );

    RhythmboxProvider provider( m_cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestRhythmboxImporter::providerShouldHandleIllFormedDbFile()
{
    m_cfg.insert( "dbPath", QCoreApplication::applicationDirPath() +
                  "/importers_files/illFormedLibrary.xml" );

    RhythmboxProvider provider( m_cfg, 0 );
    QVERIFY( provider.artistTracks( "NonSuch" ).empty() );
}

void
TestRhythmboxImporter::providerShouldHandleErroneousConfigValues()
{
    m_cfg.insert( "dbPath", "\\wd%aw@d/sdsd2'vodk0-=$$" );
    m_cfg.insert( "name", QColor( Qt::white ) );

    RhythmboxProvider provider( m_cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestRhythmboxImporter::providerShouldHandleInexistantArtist()
{
    m_cfg.insert( "dbPath", QCoreApplication::applicationDirPath() +
                  "/importers_files/rhythmdb.xml" );

    RhythmboxProvider provider( m_cfg, 0 );
    QVERIFY( provider.artistTracks( "NonSuch" ).empty() );
}

void
TestRhythmboxImporter::artistsShouldReturnExistingArtists()
{
    m_cfg.insert( "dbPath", QCoreApplication::applicationDirPath() +
                  "/importers_files/rhythmdb.xml" );

    RhythmboxProvider provider( m_cfg, 0 );
    QVERIFY( provider.artists().contains( "Metallica" ) );
}

void
TestRhythmboxImporter::artistTracksShouldReturnPopulatedTracks_data()
{
    QTest::addColumn<QString>   ( "album" );
    QTest::addColumn<QString>   ( "artist" );
    QTest::addColumn<QDateTime> ( "lastPlayed" );
    QTest::addColumn<int>       ( "playCount" );
    QTest::addColumn<int>       ( "rating" );

    QTest::newRow( "Enter Sandman" )
         << "Metallica" << "Metallica" << QDateTime::fromTime_t( 1372418184 ) << 14 << 8;
    QTest::newRow( "Sad but True" )
         << "Metallica" << "Metallica" << QDateTime::fromTime_t( 1372417063 ) << 7  << 4;
    QTest::newRow( "Holier Than Thou" )
         << "Metallica" << "Metallica" << QDateTime::fromTime_t( 1372417055 ) << 5  << 6;
    QTest::newRow( "The Unforgiven" )
         << "Metallica" << "Metallica" << QDateTime::fromTime_t( 1372419217 ) << 5  << 10;
    QTest::newRow( "Wherever I May Roam" )
         << "Metallica" << "Metallica" << QDateTime::fromTime_t( 1372420415 ) << 15 << 4;
    QTest::newRow( "Don't Tread on Me" )
         << "Metallica" << "Metallica" << QDateTime::fromTime_t( 1372419457 ) << 4  << 4;
    QTest::newRow( "Through the Never" )
         << "Metallica" << "Metallica" << QDateTime::fromTime_t( 1372419702 ) << 20 << 6;
    QTest::newRow( "Nothing Else Matters" )
         << "Metallica" << "Metallica" << QDateTime::fromTime_t( 1372418573 ) << 17 << 10;
    QTest::newRow( "Of Wolf and Man" )
         << "Metallica" << "Metallica" << QDateTime::fromTime_t( 1372418830 ) << 2  << 8;
    QTest::newRow( "The God That Failed" )
         << "Metallica" << "Metallica" << QDateTime::fromTime_t( 1372420011 ) << 15 << 2;
    QTest::newRow( "My Friend of Misery" )
         << "Metallica" << "Metallica" << QDateTime::fromTime_t( 1372417060 ) << 6  << 2;
    QTest::newRow( "The Struggle Within" )
         << "Metallica" << "Metallica" << QDateTime::fromTime_t( 1372417061 ) << 18 << 4;
}

void
TestRhythmboxImporter::artistTracksShouldReturnPopulatedTracks()
{
    m_cfg.insert( "dbPath", QCoreApplication::applicationDirPath() +
                  "/importers_files/rhythmdb.xml" );
    RhythmboxProvider provider( m_cfg, 0 );

    QMap<QString, TrackPtr> trackForName;
    foreach( const TrackPtr &track, provider.artistTracks(  "Metallica" ) )
        trackForName.insert( track->name(), track );

    QCOMPARE( trackForName.size(), 12 );

    const QString name( QTest::currentDataTag() );
    QVERIFY( trackForName.contains( name ) );

    const TrackPtr &track = trackForName[name];
    QTEST( track->album(), "album" );
    QTEST( track->artist(), "artist" );
    QTEST( track->lastPlayed(), "lastPlayed" );
    QTEST( track->playCount(), "playCount" );
    QTEST( track->rating(), "rating" );
}

void
TestRhythmboxImporter::artistTracksShoulsHandleNonexistentStatistics_data()
{
    QTest::addColumn<QDateTime> ( "lastPlayed" );
    QTest::addColumn<int>       ( "playCount" );
    QTest::addColumn<int>       ( "rating" );

    QTest::newRow( "Enter Sandman" )
            << QDateTime::fromTime_t( 1372418184 ) << 0  << 8;
    QTest::newRow( "Sad but True" )
            << QDateTime::fromTime_t( 1372417063 ) << 7  << 0;
    QTest::newRow( "Holier Than Thou" )
            << QDateTime()                         << 5  << 6;
    QTest::newRow( "The Unforgiven" )
            << QDateTime()                         << 0  << 0;
    QTest::newRow( "Wherever I May Roam" )
            << QDateTime()                         << 15 << 0;
    QTest::newRow( "Don't Tread on Me" )
            << QDateTime::fromTime_t( 1372419457 ) << 4  << 4;
    QTest::newRow( "Through the Never" )
            << QDateTime()                         << 20 << 0;
    QTest::newRow( "Nothing Else Matters" )
            << QDateTime()                         << 0  << 10;
    QTest::newRow( "Of Wolf and Man" )
            << QDateTime::fromTime_t( 1372418830 ) << 0 << 0;
    QTest::newRow( "The God That Failed" )
            << QDateTime::fromTime_t( 1372420011 ) << 15 << 2;
    QTest::newRow( "My Friend of Misery" )
            << QDateTime()                         << 6  << 2;
    QTest::newRow( "The Struggle Within" )
            << QDateTime::fromTime_t( 1372417061 ) << 18 << 4;
}

void
TestRhythmboxImporter::artistTracksShoulsHandleNonexistentStatistics()
{
    m_cfg.insert( "dbPath", QCoreApplication::applicationDirPath() +
                  "/importers_files/rhythmbox-no-statistics.xml" );
    RhythmboxProvider provider( m_cfg, 0 );

    QMap<QString, TrackPtr> trackForName;
    foreach( const TrackPtr &track, provider.artistTracks(  "Metallica" ) )
        trackForName.insert( track->name(), track );

    const QString name( QTest::currentDataTag() );
    QVERIFY( trackForName.contains( name ) );

    const TrackPtr &track = trackForName[name];
    QTEST( track->lastPlayed(), "lastPlayed" );
    QTEST( track->playCount(), "playCount" );
    QTEST( track->rating(), "rating" );
}
