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

#include <QTest>


using namespace StatSyncing;

TestImporterBase::TestImporterBase()
{
}

ProviderPtr
TestImporterBase::getWritableProvider()
{
    return getProvider();
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
        const QString msg = QStringLiteral( "Tested provider does not support %1 metadata" ) \
                                                .arg( Meta::nameForField( metavalue ) ); \
        QSKIP( msg.toLocal8Bit().constData(), SkipAll ); \
    } \
} do {} while(false)

#define amarokProviderSkipIfNoMysqld( provider ) \
    if( QString( provider->prettyName() ) == QStringLiteral("Amarok2Test") ) \
        if( !QFileInfo( QStringLiteral("/usr/sbin/mysqld") ).isExecutable() ) \
            QSKIP( "/usr/sbin/mysqld not executable, skipping Amarok provider tests", \
                   SkipAll )

void
TestImporterBase::titleShouldBeCaseSensitive()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );

    const QString artist = QStringLiteral("caseSensitiveTitle");
    QVERIFY( provider->artists().contains( artist ) );

    QSet<QString> trackNames;
    for( auto const &track : provider->artistTracks( artist ) )
        trackNames.insert( track->name() );

    QCOMPARE( trackNames.size(), 3 );
    QVERIFY( trackNames.contains( QStringLiteral("title") ) );
    QVERIFY( trackNames.contains( QStringLiteral("Title") ) );
    QVERIFY( trackNames.contains( QStringLiteral("tiTle") ) );
}

void
TestImporterBase::artistShouldBeCaseSensitive()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );

    const QVector<QString> artists = QVector<QString>()
            << QStringLiteral("caseSensitiveArtist") << QStringLiteral("casesensitiveartist") << QStringLiteral("caseSensitiveartist");

    for( auto const &artist : artists )
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
    amarokProviderSkipIfNoMysqld( provider );

    const QString artist = QStringLiteral("caseSensitiveAlbum");
    QVERIFY( provider->artists().contains( artist ) );

    QSet<QString> trackAlbums;
    for( auto const &track : provider->artistTracks( artist ) )
        trackAlbums.insert( track->album() );

    QCOMPARE( trackAlbums.size(), 3 );
    QVERIFY( trackAlbums.contains( QStringLiteral("album") ) );
    QVERIFY( trackAlbums.contains( QStringLiteral("Album") ) );
    QVERIFY( trackAlbums.contains( QStringLiteral("alBum") ) );
}

void
TestImporterBase::composerShouldBeCaseSensitive()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    skipIfNoSupport( provider->reliableTrackMetaData(), Meta::valComposer );

    const QString artist = QStringLiteral("caseSensitiveComposer");
    QVERIFY( provider->artists().contains( artist ) );

    QSet<QString> trackComposers;
    for( auto const &track : provider->artistTracks( artist ) )
        trackComposers.insert( track->composer() );

    QCOMPARE( trackComposers.size(), 3 );
    QVERIFY( trackComposers.contains( QStringLiteral("composer") ) );
    QVERIFY( trackComposers.contains( QStringLiteral("Composer") ) );
    QVERIFY( trackComposers.contains( QStringLiteral("comPoser") ) );
}

void
TestImporterBase::titleShouldSupportUTF()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );

    const QString artist = QStringLiteral("utfTitle");
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->name(), QString::fromWCharArray( L"\xF906\xF907\xF908") );
}

void
TestImporterBase::artistShouldSupportUTF()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );

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
    amarokProviderSkipIfNoMysqld( provider );

    const QString artist = QStringLiteral("utfAlbum");
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->album(), QString::fromWCharArray( L"\xF903\xF904\xF905") );
}

void
TestImporterBase::composerShouldSupportUTF()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    skipIfNoSupport( provider->reliableTrackMetaData(), Meta::valComposer );

    const QString artist = QStringLiteral("utfComposer");
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->composer(),
              QString::fromWCharArray( L"\xF900\xF901\xF902") );
}

void
TestImporterBase::titleShouldSupportMultipleWords()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );

    const QString artist = QStringLiteral("multiWordTitle");
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->name(), QStringLiteral( "ti tl e") );
}

void
TestImporterBase::artistShouldSupportMultipleWords()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );

    const QString artist = QStringLiteral("multi Word Artist");
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->artist(), artist );
}

void
TestImporterBase::albumShouldSupportMultipleWords()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );

    const QString artist = QStringLiteral("multiWordAlbum");
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->album(), QStringLiteral( "al b um") );
}

void
TestImporterBase::composerShouldSupportMultipleWords()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    skipIfNoSupport( provider->reliableTrackMetaData(), Meta::valComposer );

    const QString artist = QStringLiteral("multiWordComposer");
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->composer(), QStringLiteral( "com po ser") );
}

void
TestImporterBase::titleShouldBeWhitespaceTrimmed()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );

    const QString artist = QStringLiteral("trimTitle");
    QVERIFY( provider->artists().contains( artist ) );

    QSet<QString> trackNames;
    for( auto const &track : provider->artistTracks( artist ) )
        trackNames.insert( track->name() );

    QCOMPARE( trackNames.size(), 3 );
    QVERIFY( trackNames.contains( QStringLiteral("title1") ) );
    QVERIFY( trackNames.contains( QStringLiteral("title2") ) );
    QVERIFY( trackNames.contains( QStringLiteral("title3") ) );
}

void
TestImporterBase::artistShouldBeWhitespaceTrimmed()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );

    const QString artist = QStringLiteral("trimArtist");
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 3 );

    for( auto const &track : tracks )
        QCOMPARE( track->artist(), artist );
}

void
TestImporterBase::albumShouldBeWhitespaceTrimmed()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );

    const QString artist = QStringLiteral("trimAlbum");
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 3 );

    for( auto const &track : tracks )
        QCOMPARE( track->album(), QStringLiteral( "album") );
}

void
TestImporterBase::composerShouldBeWhitespaceTrimmed()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    skipIfNoSupport( provider->reliableTrackMetaData(), Meta::valComposer );

    const QString artist = QStringLiteral("trimComposer");
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 3 );

    for( auto const &track : tracks )
        QCOMPARE( track->composer(), QStringLiteral( "composer") );
}

void
TestImporterBase::albumShouldBeUnsetIfTagIsUnset()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );

    const QString artist = QStringLiteral("albumUnset");
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->album(), QString() );
}

void
TestImporterBase::composerShouldBeUnsetIfTagIsUnset()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    skipIfNoSupport( provider->reliableTrackMetaData(), Meta::valComposer );

    const QString artist = QStringLiteral("composerUnset");
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->composer(), QString() );
}

void
TestImporterBase::yearShouldBeUnsetIfTagIsUnset()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    skipIfNoSupport( provider->reliableTrackMetaData(), Meta::valYear );

    const QString artist = QStringLiteral("yearUnset");
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->year(), 0 );
}

void
TestImporterBase::trackShouldBeUnsetIfTagIsUnset()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    skipIfNoSupport( provider->reliableTrackMetaData(), Meta::valTrackNr );

    const QString artist = QStringLiteral("trackUnset");
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->trackNumber(), 0 );
}

void
TestImporterBase::discShouldBeUnsetIfTagIsUnset()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    skipIfNoSupport( provider->reliableTrackMetaData(), Meta::valDiscNr );

    const QString artist = QStringLiteral("discUnset");
    QVERIFY( provider->artists().contains( artist ) );

    const TrackList tracks = provider->artistTracks( artist );
    QCOMPARE( tracks.size(), 1 );
    QCOMPARE( tracks.front()->discNumber(), 0 );
}

void
TestImporterBase::checkStatistics( const QString &artist )
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    QVERIFY( provider->artists().contains( artist ) );

    QMap<QString, TrackPtr> trackForName;
    for( auto const &track : provider->artistTracks( artist ) )
        trackForName.insert( track->name(), track );

    const QString testName( QLatin1String( QTest::currentDataTag() ) );
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
        d.push_back( QDateTime::fromSecsSinceEpoch( 1378125780u + t ) );

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
    checkStatistics( QStringLiteral("testStatistics") );
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
        d.push_back( QDateTime::fromSecsSinceEpoch( 1378125780u + t ) );

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
    checkStatistics( QStringLiteral("testStatisticsNotSet") );
}

void TestImporterBase::labels( const ProviderPtr &provider, const QString &trackName )
{
    m_lbl.clear();

    const QString artist = QStringLiteral("testStatistics");

    QVERIFY( provider->artists().contains( artist ) );

    QMap<QString, TrackPtr> trackForName;
    for( auto track : provider->artistTracks( artist ) )
    {
        QVERIFY( !trackForName.contains( track->name() ) );
        trackForName.insert( track->name(), track );
    }

    QVERIFY( trackForName.contains( trackName ) );
    m_lbl = trackForName.value( trackName )->labels();
}

void
TestImporterBase::tracksShouldWorkWithSingleLabel()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    skipIfNoSupport( reliableStatistics(), Meta::valLabel );

    labels( provider, QStringLiteral("title0") );

    QCOMPARE( m_lbl.size(), 1 );
    QVERIFY( m_lbl.contains( QStringLiteral("singleTag") ) );
}

void
TestImporterBase::tracksShouldWorkWithMultipleLabels()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    skipIfNoSupport( reliableStatistics(), Meta::valLabel );

    labels( provider, QStringLiteral("title1") );

    QCOMPARE( m_lbl.size(), 2 );
    QVERIFY( m_lbl.contains( QStringLiteral("multiple") ) );
    QVERIFY( m_lbl.contains( QStringLiteral("tags") ) );
}

void
TestImporterBase::tracksShouldWorkWithCaseSensitiveLabels()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    skipIfNoSupport( reliableStatistics(), Meta::valLabel );

    labels( provider, QStringLiteral("title2") );

    QCOMPARE( m_lbl.size(), 2 );
    QVERIFY( m_lbl.contains( QStringLiteral("caseSensitive") ) );
    QVERIFY( m_lbl.contains( QStringLiteral("casesensitive") ) );
}

void
TestImporterBase::tracksShouldWorkWithUTFLabels()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    skipIfNoSupport( reliableStatistics(), Meta::valLabel );

    labels( provider, QStringLiteral("title3") );

    QCOMPARE( m_lbl.size(), 1 );
    QVERIFY( m_lbl.contains( QString::fromWCharArray( L"\x2622" ) ) );
}

void
TestImporterBase::providerShouldReturnNoTracksForNonexistentArtist()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );

    const QString artist = QStringLiteral("I'mNotHere");
    QVERIFY( !provider->artists().contains( artist ) );
    QVERIFY( provider->artistTracks( artist ).isEmpty() );
}

void
TestImporterBase::providerShouldNotBreakOnLittleBobbyTables()
{
    ProviderPtr provider( getProvider() );
    amarokProviderSkipIfNoMysqld( provider );

    const QString artist = QStringLiteral("Robert'); DROP TABLE students;--");
    QVERIFY( !provider->artists().contains( artist ) );
    QVERIFY( provider->artistTracks( artist ).isEmpty() );
}

static TrackPtr
trackForName( ProviderPtr &provider, const QString &name, const QString &artist )
{
    for( auto const &track : provider->artistTracks( artist ) )
        if( track->name() == name )
            return track;

    return TrackPtr( nullptr );
}

static Meta::FieldHash
saveData( const TrackPtr &track )
{
    Meta::FieldHash data;
    data.insert( Meta::valTitle, track->name() );
    data.insert( Meta::valArtist, track->artist() );
    data.insert( Meta::valAlbum, track->album() );
    data.insert( Meta::valComposer, track->composer() );
    data.insert( Meta::valTrackNr, track->trackNumber() );
    data.insert( Meta::valDiscNr, track->discNumber() );
    data.insert( Meta::valFirstPlayed, track->firstPlayed() );
    data.insert( Meta::valLastPlayed, track->lastPlayed() );
    data.insert( Meta::valRating, track->rating() );
    data.insert( Meta::valPlaycount, track->playCount() );
    data.insert( Meta::valLabel, QStringList( track->labels().values() ) );
    return data;
}

static void
verifyEqualExcept( const Meta::FieldHash &lhs, const TrackPtr &track,
                   const qint64 except )
{
    const QList<qint64> fields = QList<qint64>() << Meta::valTitle << Meta::valArtist
                              << Meta::valAlbum << Meta::valComposer << Meta::valTrackNr
                              << Meta::valDiscNr << Meta::valFirstPlayed
                              << Meta::valLastPlayed << Meta::valRating
                              << Meta::valPlaycount << Meta::valLabel;

    const Meta::FieldHash rhs = saveData( track );
    for( auto field : fields )
        if( !( except & field ) )
            QCOMPARE( lhs.value( field ), rhs.value( field ) );
}

void
TestImporterBase::commitAfterSettingAllStatisticsShouldSaveThem_data()
{
    QTest::addColumn<QString>( "title" );
    QTest::addColumn<QString>( "artist" );
    QTest::addColumn<QDateTime>( "newFirstPlayed" );
    QTest::addColumn<QDateTime>( "newLastPlayed" );
    QTest::addColumn<int>( "newRating" );
    QTest::addColumn<int>( "newPlayCount" );
    QTest::addColumn<QStringList>( "newLabels" );

    const uint now = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();

    QTest::newRow( "Replace all" ) << "title0" << "testStatistics"
                                   << QDateTime::fromSecsSinceEpoch( now - 100 )
                                   << QDateTime::fromSecsSinceEpoch( now + 100 )
                                   << 9 << 25 << ( QStringList() << QStringLiteral("teh") << QStringLiteral("lab'ls") );

    QTest::newRow( "Add all" ) << "title0" << "testStatisticsNotSet"
                               << QDateTime::fromSecsSinceEpoch( now - 100 )
                               << QDateTime::fromSecsSinceEpoch(now + 100 )
                               << 9 << 25 << ( QStringList() << QStringLiteral("teh") << QStringLiteral("lab'ls") );

    QTest::newRow( "Add some 1" ) << "title2" << "testStatisticsNotSet"
                                  << QDateTime::fromSecsSinceEpoch( now - 100 )
                                  << QDateTime::fromSecsSinceEpoch( now + 100 )
                                  << 9 << 25 << ( QStringList() << QStringLiteral("teh") << QStringLiteral("lab'ls") );

    QTest::newRow( "Add some 1" ) << "title4" << "testStatisticsNotSet"
                                  << QDateTime::fromSecsSinceEpoch( now - 100 )
                                  << QDateTime::fromSecsSinceEpoch( now + 100 )
                                  << 9 << 25 << ( QStringList() << QStringLiteral("teh") << QStringLiteral("lab'ls") );

    QTest::newRow( "Add some 1" ) << "title6" << "testStatisticsNotSet"
                                  << QDateTime::fromSecsSinceEpoch( now - 100 )
                                  << QDateTime::fromSecsSinceEpoch( now + 100 )
                                  << 9 << 25 << ( QStringList() << QStringLiteral("teh") << QStringLiteral("lab'ls") );
}

void
TestImporterBase::commitAfterSettingAllStatisticsShouldSaveThem()
{
    ProviderPtr provider( getWritableProvider() );
    amarokProviderSkipIfNoMysqld( provider );

    QFETCH( QString, title );
    QFETCH( QString, artist );
    TrackPtr track = trackForName( provider, title, artist );
    QVERIFY( track );

    const Meta::FieldHash data = saveData( track );

    if( provider->writableTrackStatsData() & Meta::valFirstPlayed )
    {
        QFETCH( QDateTime, newFirstPlayed );
        track->setFirstPlayed( newFirstPlayed );
    }
    if( provider->writableTrackStatsData() & Meta::valLastPlayed )
    {
        QFETCH( QDateTime, newLastPlayed );
        track->setLastPlayed( newLastPlayed );
    }
    if( provider->writableTrackStatsData() & Meta::valRating )
    {
        QFETCH( int, newRating );
        track->setRating( newRating );
    }
    if( provider->writableTrackStatsData() & Meta::valPlaycount )
    {
        QFETCH( int, newPlayCount );
        track->setPlayCount( newPlayCount );
    }
    if( provider->writableTrackStatsData() & Meta::valLabel )
    {
        QFETCH( QStringList, newLabels );
        track->setLabels( QSet<QString> ( newLabels.begin(), newLabels.end() ) );
    }

    track->commit();
    provider->commitTracks();

    track = trackForName( provider, title, artist );
    QVERIFY( track );

    if( provider->writableTrackStatsData() & Meta::valFirstPlayed )
    {
        QFETCH( QDateTime, newFirstPlayed );
        QCOMPARE( track->firstPlayed(), newFirstPlayed );
    }
    if( provider->writableTrackStatsData() & Meta::valLastPlayed )
    {
        QFETCH( QDateTime, newLastPlayed );
        QCOMPARE( track->lastPlayed(), newLastPlayed );
    }
    if( provider->writableTrackStatsData() & Meta::valRating )
    {
        QFETCH( int, newRating );
        if( !hasOddRatings() && (newRating & 1) )
            ++newRating;
        QCOMPARE( track->rating(), newRating );
    }
    if( provider->writableTrackStatsData() & Meta::valPlaycount )
    {
        QFETCH( int, newPlayCount );
        QCOMPARE( track->playCount(), newPlayCount );
    }
    if( provider->writableTrackStatsData() & Meta::valLabel )
    {
        QFETCH( QStringList, newLabels );
        QCOMPARE( track->labels(), QSet<QString> ( newLabels.begin(), newLabels.end() ) );
    }

    verifyEqualExcept( data, track, Meta::valFirstPlayed | Meta::valLastPlayed |
                       Meta::valRating | Meta::valPlaycount | Meta::valLabel );
}

void
TestImporterBase::commitAfterSettingFirstPlayedShouldSaveIt_data()
{
     QTest::addColumn<QString>( "title" );
     QTest::addColumn<QDateTime>( "newFirstPlayed" );

     const uint now = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();

     QTest::newRow( "Add stat 1" ) << "title0" << QDateTime::fromSecsSinceEpoch( now );
     QTest::newRow( "Add stat 2" ) << "title1" << QDateTime::fromSecsSinceEpoch( now + 2 );
     QTest::newRow( "Add stat 3" ) << "title2" << QDateTime::fromSecsSinceEpoch( now + 3 );

     QTest::newRow( "Replace stat 1" ) << "title5" << QDateTime::fromSecsSinceEpoch( now + 11 );
     QTest::newRow( "Replace stat 2" ) << "title6" << QDateTime::fromSecsSinceEpoch( now + 13 );
     QTest::newRow( "Replace stat 3" ) << "title7" << QDateTime::fromSecsSinceEpoch( now + 17 );

     QTest::newRow( "Remove stat 1" ) << "title5" << QDateTime();
     QTest::newRow( "Remove stat 2" ) << "title6" << QDateTime();
     QTest::newRow( "Remove stat 3" ) << "title7" << QDateTime();
}

void
TestImporterBase::commitAfterSettingFirstPlayedShouldSaveIt()
{
    ProviderPtr provider( getWritableProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    skipIfNoSupport( provider->writableTrackStatsData(), Meta::valFirstPlayed );

    QFETCH( QString, title );
    QFETCH( QDateTime, newFirstPlayed );

    TrackPtr track = trackForName( provider, title, QStringLiteral("testStatisticsNotSet") );
    QVERIFY( track );

    const Meta::FieldHash data = saveData( track );
    track->setFirstPlayed( newFirstPlayed );
    track->commit();
    provider->commitTracks();

    track = trackForName( provider, title, QStringLiteral("testStatisticsNotSet") );
    QVERIFY( track );
    QCOMPARE( track->firstPlayed(), newFirstPlayed );
    verifyEqualExcept( data, track, Meta::valFirstPlayed );
}

void
TestImporterBase::commitAfterSettingLastPlayedShouldSaveIt_data()
{
    QTest::addColumn<QString>( "title" );
    QTest::addColumn<QDateTime>( "newLastPlayed" );

    const uint now = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();

    QTest::newRow( "Add stat 1" ) << "title0" << QDateTime::fromSecsSinceEpoch( now );
    QTest::newRow( "Add stat 2" ) << "title2" << QDateTime::fromSecsSinceEpoch( now + 2 );
    QTest::newRow( "Add stat 3" ) << "title4" << QDateTime::fromSecsSinceEpoch( now + 3 );

    QTest::newRow( "Replace stat 1" ) << "title1" << QDateTime::fromSecsSinceEpoch( now + 11 );
    QTest::newRow( "Replace stat 2" ) << "title3" << QDateTime::fromSecsSinceEpoch( now + 13 );
    QTest::newRow( "Replace stat 3" ) << "title5" << QDateTime::fromSecsSinceEpoch( now + 17 );

    QTest::newRow( "Remove stat 1" ) << "title1" << QDateTime();
    QTest::newRow( "Remove stat 2" ) << "title3" << QDateTime();
    QTest::newRow( "Remove stat 3" ) << "title5" << QDateTime();
}

void
TestImporterBase::commitAfterSettingLastPlayedShouldSaveIt()
{
    ProviderPtr provider( getWritableProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    skipIfNoSupport( provider->writableTrackStatsData(), Meta::valLastPlayed );

    QFETCH( QString, title );
    QFETCH( QDateTime, newLastPlayed );

    TrackPtr track = trackForName( provider, title, QStringLiteral("testStatisticsNotSet") );
    QVERIFY( track );

    const Meta::FieldHash data = saveData( track );
    track->setLastPlayed( newLastPlayed );
    track->commit();
    provider->commitTracks();

    track = trackForName( provider, title, QStringLiteral("testStatisticsNotSet") );
    QVERIFY( track );
    QCOMPARE( track->lastPlayed(), newLastPlayed );
    verifyEqualExcept( data, track, Meta::valLastPlayed );
}

void
TestImporterBase::commitAfterSettingRatingShouldSaveIt_data()
{
    QTest::addColumn<QString>( "title" );
    QTest::addColumn<int>( "newRating" );

    QTest::newRow( "Add stat 1" ) << "title0" << 2;
    QTest::newRow( "Add stat 2" ) << "title3" << 3;
    QTest::newRow( "Add stat 3" ) << "title6" << 5;

    QTest::newRow( "Replace stat 1" ) << "title1" << 1;
    QTest::newRow( "Replace stat 2" ) << "title2" << 3;
    QTest::newRow( "Replace stat 3" ) << "title4" << 6;

    QTest::newRow( "Remove stat 1" ) << "title1" << 0;
    QTest::newRow( "Remove stat 2" ) << "title2" << 0;
    QTest::newRow( "Remove stat 3" ) << "title4" << 0;
}

void
TestImporterBase::commitAfterSettingRatingShouldSaveIt()
{
    ProviderPtr provider( getWritableProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    skipIfNoSupport( provider->writableTrackStatsData(), Meta::valRating );

    QFETCH( QString, title );
    QFETCH( int, newRating );

    TrackPtr track = trackForName( provider, title, QStringLiteral("testStatisticsNotSet") );
    QVERIFY( track );

    const Meta::FieldHash data = saveData( track );
    track->setRating( newRating );
    track->commit();
    provider->commitTracks();

    if( !hasOddRatings() && (newRating & 1) )
        ++newRating;

    track = trackForName( provider, title, QStringLiteral("testStatisticsNotSet") );
    QVERIFY( track );
    QCOMPARE( track->rating(), newRating );
    verifyEqualExcept( data, track, Meta::valRating );
}

void
TestImporterBase::commitAfterSettingPlaycountShouldSaveIt_data()
{
    QTest::addColumn<QString>( "title" );
    QTest::addColumn<int>( "newPlayCount" );

    QTest::newRow( "Add stat 1" ) << "title0" << 13;
    QTest::newRow( "Add stat 2" ) << "title4" << 17;
    QTest::newRow( "Add stat 3" ) << "title8" << 23;

    QTest::newRow( "Replace stat 1" ) << "title1" << 1;
    QTest::newRow( "Replace stat 2" ) << "title2" << 3;
    QTest::newRow( "Replace stat 3" ) << "title3" << 6;

    QTest::newRow( "Remove stat 1" ) << "title1" << 0;
    QTest::newRow( "Remove stat 2" ) << "title2" << 0;
    QTest::newRow( "Remove stat 3" ) << "title3" << 0;
}

void
TestImporterBase::commitAfterSettingPlaycountShouldSaveIt()
{
    ProviderPtr provider( getWritableProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    skipIfNoSupport( provider->writableTrackStatsData(), Meta::valPlaycount );

    QFETCH( QString, title );
    QFETCH( int, newPlayCount );

    TrackPtr track = trackForName( provider, title, QStringLiteral("testStatisticsNotSet") );
    QVERIFY( track );

    const Meta::FieldHash data = saveData( track );
    track->setPlayCount( newPlayCount );
    track->commit();
    provider->commitTracks();

    track = trackForName( provider, title, QStringLiteral("testStatisticsNotSet") );
    QVERIFY( track );
    QCOMPARE( track->playCount(), newPlayCount );
    verifyEqualExcept( data, track, Meta::valPlaycount );
}

void
TestImporterBase::commitAfterSettingLabelsShouldSaveThem_data()
{
    QTest::addColumn<QString>( "title" );
    QTest::addColumn<QStringList>( "newLabels" );

    QTest::newRow( "Add new label" ) << "title4" << ( QStringList() << QStringLiteral("singleTag2") );
    QTest::newRow( "Add existing label" ) << "title5" << ( QStringList() << QStringLiteral("singleTag") );
    QTest::newRow( "Add labels" ) << "title6" << ( QStringList() << QStringLiteral("multi") << QStringLiteral("labels") );
    QTest::newRow( "Add existing labels" ) << "title7"
                                           << ( QStringList() << QStringLiteral("multiple") << QStringLiteral("labels") );
    QTest::newRow( "Add case-sensitive labels" ) << "title8"
                                                 << ( QStringList() << QStringLiteral("cs") << QStringLiteral("Cs") );

    QTest::newRow( "Replace all labels" ) << "title1" << ( QStringList() << QStringLiteral("a") << QStringLiteral("l") );
    QTest::newRow( "Replace some labels" ) << "title1"
                                           << ( QStringList() << QStringLiteral("a") << QStringLiteral("tags") );
    QTest::newRow( "Add additional labels" ) << "title1"
                                      << ( QStringList() << QStringLiteral("multiple") << QStringLiteral("tags") << QStringLiteral("2") );

    QTest::newRow( "Remove labels 1" ) << "title0" << QStringList();
    QTest::newRow( "Remove labels 2" ) << "title1" << QStringList();
    QTest::newRow( "Remove labels 3" ) << "title2" << QStringList();
}

void
TestImporterBase::commitAfterSettingLabelsShouldSaveThem()
{
    ProviderPtr provider( getWritableProvider() );
    amarokProviderSkipIfNoMysqld( provider );
    skipIfNoSupport( provider->writableTrackStatsData(), Meta::valLabel );

    QFETCH( QString, title );
    QFETCH( QStringList, newLabels );

    TrackPtr track = trackForName( provider, title, QStringLiteral("testStatistics") );
    QVERIFY( track );

    const Meta::FieldHash data = saveData( track );
    track->setLabels( QSet<QString> ( newLabels.begin(), newLabels.end() ) );
    track->commit();
    provider->commitTracks();

    track = trackForName( provider, title, QStringLiteral("testStatistics") );
    QVERIFY( track );
    QCOMPARE( track->labels(), QSet<QString> ( newLabels.begin(), newLabels.end() ) );
    verifyEqualExcept( data, track, Meta::valLabel );
}
