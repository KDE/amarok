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

#include <TestImporterBase.h>

#include "MetaValues.h"
#include "core/meta/support/MetaConstants.h"
#include "statsyncing/Provider.h"
#include "statsyncing/Track.h"

#include <qtest_kde.h>

using namespace StatSyncing;

TestImporterBase::TestImporterBase()
{
    // This is normally set in App.cpp
    QTextCodec *utf8codec = QTextCodec::codecForName( "UTF-8" );
    QTextCodec::setCodecForCStrings( utf8codec );
}

bool
TestImporterBase::hasOddRatings() const
{
    return true;
}

#define skipIfNoSupport( fieldmask, metavalue ) \
{ \
    if( !( fieldmask & metavalue ) ) \
    { \
        const QString msg = QString( "Tested provider does not support %1 metadata" ) \
                                                .arg( Meta::nameForField( metavalue ) ); \
        QSKIP( msg.toLocal8Bit().constData(), SkipAll ); \
    } \
}

void
TestImporterBase::titleShouldBeCaseSensitive()
{
    ProviderPtr provider( getProvider() );

    const QString artist = "caseSensitiveTitle";
    QVERIFY( provider->artists().contains( artist ) );

    QSet<QString> trackNames;
    foreach( const TrackPtr &track, provider->artistTracks( artist ) )
        trackNames.insert( track->name() );

    QCOMPARE( trackNames.size(), 3 );
    QVERIFY( trackNames.contains( "title" ) );
    QVERIFY( trackNames.contains( "Title" ) );
    QVERIFY( trackNames.contains( "tiTle" ) );
}

void
TestImporterBase::artistShouldBeCaseSensitive()
{
    ProviderPtr provider( getProvider() );

    const QVector<QString> artists = QVector<QString>()
            << "caseSensitiveArtist" << "casesensitiveartist" << "caseSensitiveartist";

    foreach( const QString &artist, artists )
    {
        const TrackList tracks = provider->artistTracks( artist );
        QCOMPARE( tracks.size(), 1 );
        QCOMPARE( tracks.front()->artist(), artist );
    }
}

void
TestImporterBase::albumShouldBeCaseSensitive()
{
    ProviderPtr provider( getProvider() );

    const QString artist = "caseSensitiveAlbum";
    QVERIFY( provider->artists().contains( artist ) );

    QSet<QString> trackAlbums;
    foreach( const TrackPtr &track, provider->artistTracks( artist ) )
        trackAlbums.insert( track->album() );

    QCOMPARE( trackAlbums.size(), 3 );
    QVERIFY( trackAlbums.contains( "album" ) );
    QVERIFY( trackAlbums.contains( "Album" ) );
    QVERIFY( trackAlbums.contains( "alBum" ) );
}

void
TestImporterBase::composerShouldBeCaseSensitive()
{
    ProviderPtr provider( getProvider() );
    skipIfNoSupport( provider->reliableTrackMetaData(), Meta::valComposer );

    const QString artist = "caseSensitiveComposer";
    QVERIFY( provider->artists().contains( artist ) );

    QSet<QString> trackComposers;
    foreach( const TrackPtr &track, provider->artistTracks( artist ) )
        trackComposers.insert( track->composer() );

    QCOMPARE( trackComposers.size(), 3 );
    QVERIFY( trackComposers.contains( "composer" ) );
    QVERIFY( trackComposers.contains( "Composer" ) );
    QVERIFY( trackComposers.contains( "comPoser" ) );
}

void
TestImporterBase::titleShouldSupportUTF()
{
    ProviderPtr provider( getProvider() );

    const QString artist = "utfTitle";
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->name(), QString::fromWCharArray( L"\xF906\xF907\xF908" ) );
}

void
TestImporterBase::artistShouldSupportUTF()
{
    ProviderPtr provider( getProvider() );

    const QString artist = QString::fromWCharArray( L"utf\xF909\xF90A\xF90B" );
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->artist(), artist );
}

void
TestImporterBase::albumShouldSupportUTF()
{
    ProviderPtr provider( getProvider() );

    const QString artist = "utfAlbum";
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->album(), QString::fromWCharArray( L"\xF903\xF904\xF905" ) );
}

void
TestImporterBase::composerShouldSupportUTF()
{
    ProviderPtr provider( getProvider() );
    skipIfNoSupport( provider->reliableTrackMetaData(), Meta::valComposer );

    const QString artist = "utfComposer";
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->composer(),
              QString::fromWCharArray( L"\xF900\xF901\xF902" ) );
}

void
TestImporterBase::titleShouldSupportMultipleWords()
{
    ProviderPtr provider( getProvider() );

    const QString artist = "multiWordTitle";
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->name(), QString( "ti tl e" ) );
}

void
TestImporterBase::artistShouldSupportMultipleWords()
{
    ProviderPtr provider( getProvider() );

    const QString artist = "multi Word Artist";
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->artist(), artist );
}

void
TestImporterBase::albumShouldSupportMultipleWords()
{
    ProviderPtr provider( getProvider() );

    const QString artist = "multiWordAlbum";
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->album(), QString( "al b um" ) );
}

void
TestImporterBase::composerShouldSupportMultipleWords()
{
    ProviderPtr provider( getProvider() );
    skipIfNoSupport( provider->reliableTrackMetaData(), Meta::valComposer );

    const QString artist = "multiWordComposer";
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->composer(), QString( "com po ser" ) );
}

void
TestImporterBase::titleShouldBeWhitespaceTrimmed()
{
    ProviderPtr provider( getProvider() );

    const QString artist = "trimTitle";
    QVERIFY( provider->artists().contains( artist ) );

    QSet<QString> trackNames;
    foreach( const TrackPtr &track, provider->artistTracks( artist ) )
        trackNames.insert( track->name() );

    QCOMPARE( trackNames.size(), 3 );
    QVERIFY( trackNames.contains( "title1" ) );
    QVERIFY( trackNames.contains( "title2" ) );
    QVERIFY( trackNames.contains( "title3" ) );
}

void
TestImporterBase::artistShouldBeWhitespaceTrimmed()
{
    ProviderPtr provider( getProvider() );

    const QString artist = "trimArtist";
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 3 );

    foreach( const TrackPtr &track, tracks )
        QCOMPARE( track->artist(), artist );
}

void
TestImporterBase::albumShouldBeWhitespaceTrimmed()
{
    ProviderPtr provider( getProvider() );

    const QString artist = "trimAlbum";
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 3 );

    foreach( const TrackPtr &track, tracks )
        QCOMPARE( track->album(), QString( "album" ) );
}

void
TestImporterBase::composerShouldBeWhitespaceTrimmed()
{
    ProviderPtr provider( getProvider() );
    skipIfNoSupport( provider->reliableTrackMetaData(), Meta::valComposer );

    const QString artist = "trimComposer";
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 3 );

    foreach( const TrackPtr &track, tracks )
        QCOMPARE( track->composer(), QString( "composer" ) );
}

void
TestImporterBase::albumShouldBeUnsetIfTagIsUnset()
{
    ProviderPtr provider( getProvider() );

    const QString artist = "albumUnset";
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->album(), QString() );
}

void
TestImporterBase::composerShouldBeUnsetIfTagIsUnset()
{
    ProviderPtr provider( getProvider() );
    skipIfNoSupport( provider->reliableTrackMetaData(), Meta::valComposer );

    const QString artist = "composerUnset";
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->composer(), QString() );
}

void
TestImporterBase::yearShouldBeUnsetIfTagIsUnset()
{
    ProviderPtr provider( getProvider() );
    skipIfNoSupport( provider->reliableTrackMetaData(), Meta::valYear );

    const QString artist = "yearUnset";
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->year(), 0 );
}

void
TestImporterBase::trackShouldBeUnsetIfTagIsUnset()
{
    ProviderPtr provider( getProvider() );
    skipIfNoSupport( provider->reliableTrackMetaData(), Meta::valTrackNr );

    const QString artist = "trackUnset";
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->trackNumber(), 0 );
}

void
TestImporterBase::discShouldBeUnsetIfTagIsUnset()
{
    ProviderPtr provider( getProvider() );
    skipIfNoSupport( provider->reliableTrackMetaData(), Meta::valDiscNr );

    const QString artist = "discUnset";
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->discNumber(), 0 );
}

void
TestImporterBase::checkStatistics( const QString &artist )
{
    ProviderPtr provider( getProvider() );
    QVERIFY( provider->artists().contains( artist ) );

    QMap<QString, TrackPtr> trackForName;
    foreach( const TrackPtr &track, provider->artistTracks( artist ) )
        trackForName.insert( track->name(), track );

    const QString testName( QTest::currentDataTag() );
    QCOMPARE( trackForName.size(), 10 );
    QVERIFY( trackForName.contains( testName ) );

    const TrackPtr &track = trackForName.value( testName );

    if( reliableStatistics() & Meta::valFirstPlayed )
        QTEST( track->firstPlayed(), "firstPlayed" );
    if( reliableStatistics() & Meta::valLastPlayed )
        QTEST( track->lastPlayed(), "lastPlayed" );
    if( reliableStatistics() & Meta::valPlaycount )
        QTEST( track->playCount(), "playCount" );

    if( reliableStatistics() & Meta::valRating )
    {
        QFETCH( int, rating );
        if( !hasOddRatings() && (rating & 1) )
            ++rating;

        QCOMPARE( track->rating(), rating );
    }
}

void
TestImporterBase::tracksShouldHaveStatistics_data()
{
    QTest::addColumn<QDateTime> ( "firstPlayed" );
    QTest::addColumn<QDateTime> ( "lastPlayed" );
    QTest::addColumn<int>       ( "rating" );
    QTest::addColumn<int>       ( "playCount" );

    QVector<QDateTime> d;
    for( uint t = 0; t < 20; ++t )
        d.push_back( QDateTime::fromTime_t( 1378125780u + t ) );

    QTest::newRow( "title0" ) << d[ 0] << d[ 1] <<  1 << 20;
    QTest::newRow( "title1" ) << d[ 2] << d[ 3] <<  2 << 15;
    QTest::newRow( "title2" ) << d[ 4] << d[ 5] <<  3 << 14;
    QTest::newRow( "title3" ) << d[ 6] << d[ 7] <<  4 << 13;
    QTest::newRow( "title4" ) << d[ 8] << d[ 9] <<  5 << 11;
    QTest::newRow( "title5" ) << d[10] << d[11] <<  6 << 10;
    QTest::newRow( "title6" ) << d[12] << d[13] <<  7 <<  7;
    QTest::newRow( "title7" ) << d[14] << d[15] <<  8 <<  5;
    QTest::newRow( "title8" ) << d[16] << d[17] <<  9 <<  3;
    QTest::newRow( "title9" ) << d[18] << d[19] << 10 <<  2;
}

void
TestImporterBase::tracksShouldHaveStatistics()
{
    checkStatistics( "testStatistics" );
}

void
TestImporterBase::tracksShouldBehaveNicelyWithNoStatistics_data()
{
    QTest::addColumn<QDateTime> ( "firstPlayed" );
    QTest::addColumn<QDateTime> ( "lastPlayed" );
    QTest::addColumn<int>       ( "rating" );
    QTest::addColumn<int>       ( "playCount" );

    QVector<QDateTime> d;
    for( uint t = 0; t < 20; ++t )
        d.push_back( QDateTime::fromTime_t( 1378125780u + t ) );

    QTest::newRow( "title0" ) << QDateTime() << QDateTime() <<  0 <<  0;
    QTest::newRow( "title1" ) << QDateTime() <<       d[ 3] <<  2 << 15;
    QTest::newRow( "title2" ) << QDateTime() << QDateTime() <<  3 << 14;
    QTest::newRow( "title3" ) << QDateTime() <<       d[ 7] <<  0 << 13;
    QTest::newRow( "title4" ) << QDateTime() << QDateTime() <<  5 <<  0;
    QTest::newRow( "title5" ) <<       d[10] <<       d[11] <<  6 << 10;
    QTest::newRow( "title6" ) <<       d[12] << QDateTime() <<  0 <<  7;
    QTest::newRow( "title7" ) <<       d[14] <<       d[15] <<  8 <<  5;
    QTest::newRow( "title8" ) <<       d[16] << QDateTime() <<  9 <<  0;
    QTest::newRow( "title9" ) <<       d[18] <<       d[19] <<  0 <<  2;
}

void
TestImporterBase::tracksShouldBehaveNicelyWithNoStatistics()
{
    checkStatistics( "testStatisticsNotSet" );
}

void TestImporterBase::labels( const QString &trackName )
{
    m_lbl.clear();

    ProviderPtr provider( getProvider() );
    const QString artist = "testStatistics";

    QVERIFY( provider->artists().contains( artist ) );

    QMap<QString, TrackPtr> trackForName;
    foreach( const TrackPtr &track, provider->artistTracks( artist ) )
        trackForName.insert( track->name(), track );

    QVERIFY( trackForName.contains( trackName ) );
    m_lbl = trackForName.value( trackName )->labels();
}

void
TestImporterBase::tracksShouldWorkWithSingleLabel()
{
    skipIfNoSupport( reliableStatistics(), Meta::valLabel );
    labels( "title0" );

    QCOMPARE( m_lbl.size(), 1 );
    QVERIFY( m_lbl.contains( "singleTag" ) );
}

void
TestImporterBase::tracksShouldWorkWithMultipleLabels()
{
    skipIfNoSupport( reliableStatistics(), Meta::valLabel );
    labels( "title1" );

    QCOMPARE( m_lbl.size(), 2 );
    QVERIFY( m_lbl.contains( "multiple" ) );
    QVERIFY( m_lbl.contains( "tags" ) );
}

void
TestImporterBase::tracksShouldWorkWithCaseSensitiveLabels()
{
    skipIfNoSupport( reliableStatistics(), Meta::valLabel );
    labels( "title2" );

    QCOMPARE( m_lbl.size(), 2 );
    QVERIFY( m_lbl.contains( "caseSensitive" ) );
    QVERIFY( m_lbl.contains( "casesensitive" ) );
}

void
TestImporterBase::tracksShouldWorkWithUTFLabels()
{
    skipIfNoSupport( reliableStatistics(), Meta::valLabel );
    labels( "title3" );

    QCOMPARE( m_lbl.size(), 1 );
    QVERIFY( m_lbl.contains( QString::fromWCharArray( L"\x2622" ) ) );
}

void
TestImporterBase::providerShouldReturnNoTracksForNonexistentArtist()
{
    ProviderPtr provider( getProvider() );

    const QString artist = "I'mNotHere";
    QVERIFY( !provider->artists().contains( artist ) );
    QVERIFY( provider->artistTracks( artist ).isEmpty() );
}

void
TestImporterBase::providerShouldNotBreakOnLittleBobbyTables()
{
    ProviderPtr provider( getProvider() );

    const QString artist = "Robert'); DROP TABLE students;--";
    QVERIFY( !provider->artists().contains( artist ) );
    QVERIFY( provider->artistTracks( artist ).isEmpty() );
}
