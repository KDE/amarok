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

#include "TestBansheeImporter.h"

#include "importers/banshee/BansheeConfigWidget.h"

#include <qtest_kde.h>

QTEST_KDEMAIN( TestBansheeImporter, GUI )

using namespace StatSyncing;

void
TestBansheeImporter::init()
{
    m_cfg = BansheeConfigWidget( QVariantMap() ).config();
}

void
TestBansheeImporter::providerShouldHandleNonexistentArtist()
{
    ProviderPtr provider( getProvider( "banshee.db" ) );
    QVERIFY( provider->artistTracks( "NonSuch" ).empty() );
}

void
TestBansheeImporter::artistsShouldReturnExistingArtists()
{
    ProviderPtr provider( getProvider( "banshee.db" ) );
    QVERIFY( provider->artists().contains( "Daft Punk" ) );
}


void
TestBansheeImporter::artistTracksShouldReturnPopulatedTracks_data()
{
    QTest::addColumn<QString>   ( "album" );
    QTest::addColumn<QString>   ( "artist" );
    QTest::addColumn<int>       ( "trackNumber" );
    QTest::addColumn<int>       ( "discNumber" );
    QTest::addColumn<QDateTime> ( "lastPlayed" );
    QTest::addColumn<int>       ( "playCount" );
    QTest::addColumn<int>       ( "rating" );

    QTest::newRow( "One More Time" )
         << "Discovery" << "Daft Punk" << 1 << 0
         << QDateTime::fromTime_t( 1378049427 ) << 19 << 10;
    QTest::newRow( "Aerodynamic" )
         << "Discovery" << "Daft Punk" << 2 << 0
         << QDateTime::fromTime_t( 1378049411 ) << 21 << 6;
    QTest::newRow( "Digital Love" )
         << "Discovery" << "Daft Punk" << 3 << 0
         << QDateTime::fromTime_t( 1378049418 ) << 12 << 10;
    QTest::newRow( "Harder, Better, Faster, Stronger" )
         << "Discovery" << "Daft Punk" << 4 << 0
         << QDateTime::fromTime_t( 1378049423 ) << 14 << 4;
    QTest::newRow( "Crescendolls" )
         << "Discovery" << "Daft Punk" << 5 << 0
         << QDateTime::fromTime_t( 1378049461 ) << 21 << 8;
    QTest::newRow( "Nightvision" )
         << "Discovery" << "Daft Punk" << 6 << 0
         << QDateTime::fromTime_t( 1378049426 ) <<  3 << 8;
    QTest::newRow( "Superheroes" )
         << "Discovery" << "Daft Punk" << 7 << 0
         << QDateTime::fromTime_t( 1378049431 ) << 20 << 6;
    QTest::newRow( "High Life" )
         << "Discovery" << "Daft Punk" << 8 << 0
         << QDateTime::fromTime_t( 1378049425 ) << 19 << 6;
    QTest::newRow( "Something About Us" )
         << "Discovery" << "Daft Punk" << 9 << 0
         << QDateTime::fromTime_t( 1378049430 ) <<  4 << 4;
    QTest::newRow( "Voyager" )
         << "Discovery" << "Daft Punk" << 10 << 0
         << QDateTime::fromTime_t( 1378049435 ) <<  7 << 2;
    QTest::newRow( "Veridis Quo" )
         << "Discovery" << "Daft Punk" << 11 << 0
         << QDateTime::fromTime_t( 1378049434 ) << 12 << 10;
    QTest::newRow( "Short Circuit" )
         << "Discovery" << "Daft Punk" << 12 << 0
         << QDateTime::fromTime_t( 1378049429 ) <<  4 << 2;
    QTest::newRow( "Face to Face" )
         << "Discovery" << "Daft Punk" << 13 << 0
         << QDateTime::fromTime_t( 1378049420 ) << 21 << 2;
    QTest::newRow( "Too Long" )
         << "Discovery" << "Daft Punk" << 14 << 0
         << QDateTime::fromTime_t( 1378049433 ) << 21 << 8;
}

void
TestBansheeImporter::artistTracksShouldReturnPopulatedTracks()
{
    ProviderPtr provider( getProvider( "banshee.db" ) );

    QMap<QString, TrackPtr> trackForName;
    foreach( const TrackPtr &track, provider->artistTracks( "Daft Punk" ) )
        trackForName.insert( track->name(), track );

    QCOMPARE( trackForName.size(), 14 );

    const QString name( QTest::currentDataTag() );
    QVERIFY( trackForName.contains( name ) );

    const TrackPtr &track = trackForName[name];
    QTEST( track->album(), "album" );
    QTEST( track->artist(), "artist" );
    QTEST( track->trackNumber(), "trackNumber" );
    QTEST( track->discNumber(), "discNumber" );
    QTEST( track->lastPlayed(), "lastPlayed" );
    QTEST( track->playCount(), "playCount" );
    QTEST( track->rating(), "rating" );
}

void
TestBansheeImporter::artistTracksStringsShouldBeTrimmed()
{
    ProviderPtr provider( getProvider( "banshee-trim.db" ) );

    const TrackList &tracks = provider->artistTracks( "Nirvana" );
    QCOMPARE( tracks.size(), 1 );

    const TrackPtr &track = tracks.first();
    QCOMPARE( track->name(), QString( "Smells Like Teen Spirit" ) );
    QCOMPARE( track->album(), QString( "Nevermind" ) );
}

void
TestBansheeImporter::artistTracksShouldHandleNonexistentStatistics_data()
{
    QTest::addColumn<QDateTime> ( "lastPlayed" );
    QTest::addColumn<int>       ( "playCount" );
    QTest::addColumn<int>       ( "rating" );

    QTest::newRow( "One More Time" )
          << QDateTime::fromTime_t( 1378049427 ) <<  0 << 10;
    QTest::newRow( "Aerodynamic" )
          << QDateTime()                         << 21 << 0;
    QTest::newRow( "Digital Love" )
          << QDateTime::fromTime_t( 1378049418 ) << 12 << 10;
    QTest::newRow( "Harder, Better, Faster, Stronger" )
          << QDateTime::fromTime_t( 1378049423 ) <<  0 << 0;
    QTest::newRow( "Crescendolls" )
          << QDateTime()                         << 21 << 8;
    QTest::newRow( "Nightvision" )
          << QDateTime::fromTime_t( 1378049426 ) <<  0 << 8;
    QTest::newRow( "Superheroes" )
          << QDateTime::fromTime_t( 1378049431 ) <<  0 << 6;
    QTest::newRow( "High Life" )
          << QDateTime()                         << 19 << 0;
    QTest::newRow( "Something About Us" )
          << QDateTime::fromTime_t( 1378049430 ) <<  0 << 4;
    QTest::newRow( "Voyager" )
          << QDateTime::fromTime_t( 1378049435 ) <<  7 << 2;
    QTest::newRow( "Veridis Quo" )
          << QDateTime::fromTime_t( 1378049434 ) <<  0 << 10;
    QTest::newRow( "Short Circuit" )
          << QDateTime::fromTime_t( 1378049429 ) <<  4 << 0;
    QTest::newRow( "Face to Face" )
          << QDateTime()                         << 21 << 0;
    QTest::newRow( "Too Long" )
          << QDateTime()                         << 21 << 0;
}

void
TestBansheeImporter::artistTracksShouldHandleNonexistentStatistics()
{
    ProviderPtr provider( getProvider( "banshee-no-statistics.db" ) );

    QMap<QString, TrackPtr> trackForName;
    foreach( const TrackPtr &track, provider->artistTracks( "Daft Punk" ) )
        trackForName.insert( track->name(), track );

    const QString name( QTest::currentDataTag() );
    QVERIFY( trackForName.contains( name ) );

    const TrackPtr &track = trackForName[name];
    QTEST( track->lastPlayed(), "lastPlayed" );
    QTEST( track->playCount(), "playCount" );
    QTEST( track->rating(), "rating" );
}
