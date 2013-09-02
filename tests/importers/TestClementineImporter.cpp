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

#include "TestClementineImporter.h"

#include "importers/clementine/ClementineConfigWidget.h"

#include <qtest_kde.h>

QTEST_KDEMAIN( TestClementineImporter, GUI )

using namespace StatSyncing;

void
TestClementineImporter::init()
{
    m_cfg = ClementineConfigWidget( QVariantMap() ).config();
}

void
TestClementineImporter::providerShouldHandleNonexistentArtist()
{
    ProviderPtr provider( getProvider( "clementine.db" ) );
    QVERIFY( provider->artistTracks( "NonSuch" ).empty() );
}

void
TestClementineImporter::artistsShouldReturnExistingArtists()
{
    ProviderPtr provider( getProvider( "clementine.db" ) );

    const QSet<QString> artists = provider->artists();
    QVERIFY( artists.contains( "Darren Korb" ) );
    QVERIFY( artists.contains( "Star One" ) );
}

void
TestClementineImporter::artistTracksShouldReturnPopulatedTracks_data()
{
    QTest::addColumn<QString>   ( "album" );
    QTest::addColumn<QString>   ( "artist" );
    QTest::addColumn<QString>   ( "composer" );
    QTest::addColumn<int>       ( "year" );
    QTest::addColumn<int>       ( "trackNumber" );
    QTest::addColumn<int>       ( "discNumber" );
    QTest::addColumn<QDateTime> ( "lastPlayed" );
    QTest::addColumn<int>       ( "playCount" );
    QTest::addColumn<int>       ( "rating" );

    QTest::newRow( "Mother, I'm Here (Zulf's Theme)" )
            << "Bastion Original Soundtrack" << "Darren Korb" << "TestComposer"
            << 2011 << 17 << 0 << QDateTime::fromTime_t( 1378111478 ) << 20 << 10;
    QTest::newRow( "Pale Watchers" )
            << "Bastion Original Soundtrack" << "Darren Korb" << "TestComposer"
            << 2011 << 18 << 0 << QDateTime::fromTime_t( 1378111476 ) <<  7 << 9;
    QTest::newRow( "The Bottom Feeders" )
            << "Bastion Original Soundtrack" << "Darren Korb" << "TestComposer"
            << 2011 << 19 << 0 << QDateTime::fromTime_t( 1378111473 ) << 15 << 8;
    QTest::newRow( "From Wharf to Wilds" )
            << "Bastion Original Soundtrack" << "Darren Korb" << "TestComposer"
            << 2011 << 20 << 0 << QDateTime::fromTime_t( 1378111474 ) <<  9 << 7;
    QTest::newRow( "Setting Sail, Coming Home (End Theme)" )
            << "Bastion Original Soundtrack" << "Darren Korb" << "TestComposer"
            << 2011 << 21 << 0 << QDateTime::fromTime_t( 1378111481 ) << 17 << 6;
    QTest::newRow( "The Pantheon (Ain't Gonna Catch You)" )
            << "Bastion Original Soundtrack" << "Darren Korb" << "TestComposer"
            << 2011 << 22 << 0 << QDateTime::fromTime_t( 1378111482 ) <<  6 << 1;
    QTest::newRow( "Down the Rabbit Hole" )
            << "Victims of the Modern Age"   << "Star One" << "TestComposer"
            << 2010 <<  1 << 0 << QDateTime::fromTime_t( 1378111475 ) <<  6 << 5;
    QTest::newRow( "Digital Rain" )
            << "Victims of the Modern Age"   << "Star One" << "TestComposer"
            << 2010 <<  2 << 0 << QDateTime::fromTime_t( 1378111479 ) <<  7 << 4;
    QTest::newRow( "Earth That Was" )
            << "Victims of the Modern Age"   << "Star One" << "TestComposer"
            << 2010 <<  3 << 0 << QDateTime::fromTime_t( 1378111480 ) << 10 << 3;
    QTest::newRow( "Victim of the Modern Age" )
            << "Victims of the Modern Age"   << "Star One" << "TestComposer"
            << 2010 <<  4 << 0 << QDateTime::fromTime_t( 1378111477 ) <<  1 << 2;
}

void
TestClementineImporter::artistTracksShouldReturnPopulatedTracks()
{
    ProviderPtr provider( getProvider( "clementine.db" ) );

    QFETCH( QString, artist );

    QMap<QString, TrackPtr> trackForName;
    foreach( const TrackPtr &track, provider->artistTracks( artist ) )
        trackForName.insert( track->name(), track );

    QVERIFY( trackForName.size() > 1 );

    const QString name( QTest::currentDataTag() );
    QVERIFY( trackForName.contains( name ) );

    const TrackPtr &track = trackForName[name];
    QTEST( track->album(), "album" );
    QTEST( track->artist(), "artist" );
    QTEST( track->composer(), "composer" );
    QTEST( track->trackNumber(), "trackNumber" );
    QTEST( track->discNumber(), "discNumber" );
    QTEST( track->lastPlayed(), "lastPlayed" );
    QTEST( track->playCount(), "playCount" );
    QTEST( track->rating(), "rating" );
}

void
TestClementineImporter::artistTracksShouldHandleNonexistentStatistics_data()
{
    QTest::addColumn<QString>   ( "artist" );
    QTest::addColumn<QDateTime> ( "lastPlayed" );
    QTest::addColumn<int>       ( "playCount" );
    QTest::addColumn<int>       ( "rating" );

    QTest::newRow( "Mother, I'm Here (Zulf's Theme)" )
            << "Darren Korb" << QDateTime::fromTime_t( 1378111478 ) << 20 << 0;
    QTest::newRow( "Pale Watchers" )
            << "Darren Korb" << QDateTime()                         <<  0 << 0;
    QTest::newRow( "The Bottom Feeders" )
            << "Darren Korb" << QDateTime()                         << 15 << 8;
    QTest::newRow( "From Wharf to Wilds" )
            << "Darren Korb" << QDateTime::fromTime_t( 1378111474 ) <<  0 << 0;
    QTest::newRow( "Setting Sail, Coming Home (End Theme)" )
            << "Darren Korb" << QDateTime()                         <<  0 << 0;
    QTest::newRow( "The Pantheon (Ain't Gonna Catch You)" )
            << "Darren Korb" << QDateTime::fromTime_t( 1378111482 ) <<  6 << 1;
    QTest::newRow( "Down the Rabbit Hole" )
            << "Star One"    << QDateTime::fromTime_t( 1378111475 ) <<  6 << 5;
    QTest::newRow( "Digital Rain" )
            << "Star One"    << QDateTime::fromTime_t( 1378111479 ) <<  0 << 4;
    QTest::newRow( "Earth That Was" )
            << "Star One"    << QDateTime()                         << 10 << 3;
    QTest::newRow( "Victim of the Modern Age" )
            << "Star One"    << QDateTime::fromTime_t( 1378111477 ) <<  1 << 2;
}

void
TestClementineImporter::artistTracksShouldHandleNonexistentStatistics()
{
    ProviderPtr provider( getProvider( "clementine-no-statistics.db" ) );

    QFETCH( QString, artist );

    QMap<QString, TrackPtr> trackForName;
    foreach( const TrackPtr &track, provider->artistTracks( artist ) )
        trackForName.insert( track->name(), track );

    const QString name( QTest::currentDataTag() );
    QVERIFY( trackForName.contains( name ) );

    const TrackPtr &track = trackForName[name];
    QTEST( track->lastPlayed(), "lastPlayed" );
    QTEST( track->playCount(), "playCount" );
    QTEST( track->rating(), "rating" );
}

void
TestClementineImporter::artistTracksShouldHandleNonexistentData_data()
{
    QTest::addColumn<QString>   ( "artist" );
    QTest::addColumn<QString>   ( "composer" );
    QTest::addColumn<int>       ( "year" );
    QTest::addColumn<int>       ( "trackNumber" );
    QTest::addColumn<int>       ( "discNumber" );

    QTest::newRow( "Mother, I'm Here (Zulf's Theme)" )
            << "Darren Korb" << "TestComposer" << 2011 << 17 << 0;
    QTest::newRow( "Pale Watchers" )
            << "Darren Korb" << "TestComposer" << 2011 << 18 << 0;
    QTest::newRow( "The Bottom Feeders" )
            << "Darren Korb" << ""             << 2011 << 19 << 0;
    QTest::newRow( "From Wharf to Wilds" )
            << "Darren Korb" << "TestComposer" << 2011 <<  0 << 0;
    QTest::newRow( "Setting Sail, Coming Home (End Theme)" )
            << "Darren Korb" << ""             << 2011 << 21 << 0;
    QTest::newRow( "The Pantheon (Ain't Gonna Catch You)" )
            << "Darren Korb" << "TestComposer" << 2011 <<  0 << 0;
    QTest::newRow( "Down the Rabbit Hole" )
            << "Star One"    << "TestComposer" << 2010 <<  1 << 0;
    QTest::newRow( "Digital Rain" )
            << "Star One"    << ""             << 2010 <<  2 << 0;
    QTest::newRow( "Earth That Was" )
            << "Star One"    << "TestComposer" << 2010 <<  0 << 0;
    QTest::newRow( "Victim of the Modern Age" )
            << "Star One"    << ""             << 2010 <<  4 << 0;
}

void
TestClementineImporter::artistTracksShouldHandleNonexistentData()
{
    ProviderPtr provider( getProvider( "clementine-no-data.db" ) );

    QFETCH( QString, artist );

    QMap<QString, TrackPtr> trackForName;
    foreach( const TrackPtr &track, provider->artistTracks( artist ) )
        trackForName.insert( track->name(), track );

    const QString name( QTest::currentDataTag() );
    QVERIFY( trackForName.contains( name ) );

    const TrackPtr &track = trackForName[name];
    QTEST( track->artist(), "artist" );
    QTEST( track->composer(), "composer" );
    QTEST( track->trackNumber(), "trackNumber" );
    QTEST( track->discNumber(), "discNumber" );
}
