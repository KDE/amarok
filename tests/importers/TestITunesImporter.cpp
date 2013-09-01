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

#include "TestITunesImporter.h"

#include "importers/itunes/ITunesConfigWidget.h"
#include "importers/itunes/ITunesProvider.h"

#include <QApplication>

#include <qtest_kde.h>

QTEST_KDEMAIN( TestITunesImporter, GUI )

using namespace StatSyncing;

void
TestITunesImporter::init()
{
    // Set a default config
    m_cfg = ITunesConfigWidget( QVariantMap() ).config();
}

void
TestITunesImporter::providerShouldHandleNonexistentDbFile()
{
    m_cfg.insert( "dbPath", "/wdawd\\wdadwgd/das4hutyf" );

    ITunesProvider provider( m_cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestITunesImporter::providerShouldHandleInvalidDbFile()
{
    m_cfg.insert( "dbPath", QApplication::applicationFilePath() );

    ITunesProvider provider( m_cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestITunesImporter::providerShouldHandleIllFormedDbFile()
{
    m_cfg.insert( "dbPath", QCoreApplication::applicationDirPath() +
                  "/importers_files/illFormedLibrary.xml" );

    ITunesProvider provider( m_cfg, 0 );
    QVERIFY( provider.artistTracks( "NonSuch" ).empty() );
}

void
TestITunesImporter::providerShouldHandleErroneousConfigValues()
{
    m_cfg.insert( "dbPath", "\\wd%aw@d/sdsd2'vodk0-=$$" );
    m_cfg.insert( "name", QColor( Qt::white ) );

    ITunesProvider provider( m_cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestITunesImporter::providerShouldHandleNonexistentArtist()
{
    m_cfg.insert( "dbPath", QCoreApplication::applicationDirPath() +
                  "/importers_files/iTunes Music Library.xml" );

    ITunesProvider provider( m_cfg, 0 );
    QVERIFY( provider.artistTracks( "NonSuch" ).empty() );
}

void
TestITunesImporter::artistsShouldReturnExistingArtists()
{
    m_cfg.insert( "dbPath", QCoreApplication::applicationDirPath() +
                  "/importers_files/iTunes Music Library.xml" );

    ITunesProvider provider( m_cfg, 0 );
    QVERIFY( provider.artists().contains( "Metallica" ) );
}

void
TestITunesImporter::artistTracksShouldReturnPopulatedTracks_data()
{
    QTest::addColumn<QString>   ( "album" );
    QTest::addColumn<QString>   ( "artist" );
    QTest::addColumn<QString>   ( "composer" );
    QTest::addColumn<QDateTime> ( "lastPlayed" );
    QTest::addColumn<int>       ( "playCount" );
    QTest::addColumn<int>       ( "rating" );

    QTest::newRow( "Enter Sandman" )
            << "Metallica" << "Metallica" << "James Hetfield, Kirk Hammett, Lars Ulrich"
            << QDateTime::fromTime_t( 1372418184 ) << 14 << 9;
    QTest::newRow( "Sad but True" )
            << "Metallica" << "Metallica" << "James Hetfield, Lars Ulrich"
            << QDateTime::fromTime_t( 1372417063 ) << 7  << 5;
    QTest::newRow( "Holier Than Thou" )
            << "Metallica" << "Metallica" << "James Hetfield, Lars Ulrich"
            << QDateTime::fromTime_t( 1372417055 ) << 5  << 6;
    QTest::newRow( "The Unforgiven" )
            << "Metallica" << "Metallica" << "James Hetfield, Kirk Hammett, Lars Ulrich"
            << QDateTime::fromTime_t( 1372419217 ) << 5  << 10;
    QTest::newRow( "Wherever I May Roam" )
            << "Metallica" << "Metallica" << "James Hetfield, Lars Ulrich"
            << QDateTime::fromTime_t( 1372420415 ) << 15 << 5;
    QTest::newRow( "Don't Tread on Me" )
            << "Metallica" << "Metallica" << "James Hetfield, Lars Ulrich"
            << QDateTime::fromTime_t( 1372419457 ) << 4  << 4;
    QTest::newRow( "Through the Never" )
            << "Metallica" << "Metallica" << "James Hetfield, Kirk Hammett, Lars Ulrich"
            << QDateTime::fromTime_t( 1372419702 ) << 20 << 7;
    QTest::newRow( "Nothing Else Matters" )
            << "Metallica" << "Metallica" << "James Hetfield, Lars Ulrich"
            << QDateTime::fromTime_t( 1372418573 ) << 17 << 10;
    QTest::newRow( "Of Wolf and Man" )
            << "Metallica" << "Metallica" << "James Hetfield, Kirk Hammett, Lars Ulrich"
            << QDateTime::fromTime_t( 1372418830 ) << 2  << 8;
    QTest::newRow( "The God That Failed" )
            << "Metallica" << "Metallica" << "James Hetfield, Lars Ulrich"
            << QDateTime::fromTime_t( 1372420011 ) << 15 << 2;
    QTest::newRow( "My Friend of Misery" )
            << "Metallica" << "Metallica" << "James Hetfield, Kirk Hammett, Lars Ulrich"
            << QDateTime::fromTime_t( 1372417060 ) << 6  << 1;
    QTest::newRow( "The Struggle Within" )
            << "Metallica" << "Metallica" << "James Hetfield, Lars Ulrich"
            << QDateTime::fromTime_t( 1372417061 ) << 18 << 3;
}

void
TestITunesImporter::artistTracksShouldReturnPopulatedTracks()
{
    m_cfg.insert( "dbPath", QCoreApplication::applicationDirPath() +
                  "/importers_files/iTunes Music Library.xml" );
    ITunesProvider provider( m_cfg, 0 );

    QMap<QString, TrackPtr> trackForName;
    foreach( const TrackPtr &track, provider.artistTracks(  "Metallica" ) )
        trackForName.insert( track->name(), track );

    QCOMPARE( trackForName.size(), 12 );

    const QString name( QTest::currentDataTag() );
    QVERIFY( trackForName.contains( name ) );

    const TrackPtr &track = trackForName[name];
    QTEST( track->album(), "album" );
    QTEST( track->artist(), "artist" );
    QTEST( track->composer(), "composer" );
    QTEST( track->lastPlayed(), "lastPlayed" );
    QTEST( track->playCount(), "playCount" );
    QTEST( track->rating(), "rating" );
}

void
TestITunesImporter::artistTracksShouldHandleNonexistentStatistics_data()
{
    QTest::addColumn<QDateTime> ( "lastPlayed" );
    QTest::addColumn<int>       ( "playCount" );
    QTest::addColumn<int>       ( "rating" );

    QTest::newRow( "Enter Sandman" )
            << QDateTime::fromTime_t( 1372418184 ) << 14 << 9;
    QTest::newRow( "Sad but True" )
            << QDateTime::fromTime_t( 1372417063 ) << 7  << 5;
    QTest::newRow( "Holier Than Thou" )
            << QDateTime::fromTime_t( 1372417055 ) << 0  << 0;
    QTest::newRow( "The Unforgiven" )
            << QDateTime::fromTime_t( 1372419217 ) << 5  << 10;
    QTest::newRow( "Wherever I May Roam" )
            << QDateTime::fromTime_t( 1372420415 ) << 15 << 0;
    QTest::newRow( "Don't Tread on Me" )
            << QDateTime::fromTime_t( 1372419457 ) << 0  << 4;
    QTest::newRow( "Through the Never" )
            << QDateTime::fromTime_t( 1372419702 ) << 20 << 7;
    QTest::newRow( "Nothing Else Matters" )
            << QDateTime()                         << 17 << 0;
    QTest::newRow( "Of Wolf and Man" )
            << QDateTime()                         << 0  << 0;
    QTest::newRow( "The God That Failed" )
            << QDateTime()                         << 0  << 2;
    QTest::newRow( "My Friend of Misery" )
            << QDateTime::fromTime_t( 1372417060 ) << 6  << 1;
    QTest::newRow( "The Struggle Within" )
            << QDateTime()                         << 18 << 3;
}

void
TestITunesImporter::artistTracksShouldHandleNonexistentStatistics()
{
    m_cfg.insert( "dbPath", QCoreApplication::applicationDirPath() +
                  "/importers_files/itunes-no-statistics.xml" );
    ITunesProvider provider( m_cfg, 0 );

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
