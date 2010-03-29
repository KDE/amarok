/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@getamarok.com>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "TestMetaTrack.h"

#include "core-implementations/collections/support/CollectionManager.h"
#include "config-amarok-test.h"

#include <QtTest/QTest>

#include <qtest_kde.h>

QTEST_KDEMAIN( TestMetaTrack, GUI )

TestMetaTrack::TestMetaTrack()
    : m_trackPath( dataPath( "/data/audio/Platz 01.mp3" ) )
{}

QString
TestMetaTrack::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + '/' + relPath );
}

void TestMetaTrack::initTestCase()
{
    m_testTrack1 = CollectionManager::instance()->trackForUrl( m_trackPath );

    // If the pointer is 0, it makes no sense to continue. We would crash with a qFatal().
    QVERIFY2( m_testTrack1, "The pointer to the test track is 0." );
}

void TestMetaTrack::cleanupTestCase()
{
}


void TestMetaTrack::testPrettyName()
{
    QCOMPARE( m_testTrack1->prettyName(), QString( "Platz 01" ) );
}

void TestMetaTrack::testPlayableUrl()
{
    QCOMPARE( m_testTrack1->playableUrl().pathOrUrl(), m_trackPath );
}

void TestMetaTrack::testPrettyUrl()
{
    QCOMPARE( m_testTrack1->prettyUrl(), m_trackPath );
}

void TestMetaTrack::testUidUrl()
{
    QCOMPARE( m_testTrack1->uidUrl(), KUrl( m_trackPath ).url() );
}

void TestMetaTrack::testIsPlayable()
{
    QCOMPARE( m_testTrack1->isPlayable(), true );
}

void TestMetaTrack::testAlbum()
{
    QCOMPARE( m_testTrack1->album().data()->name() , QString( "" ) );
}

void TestMetaTrack::testArtist()
{
    QCOMPARE( m_testTrack1->artist().data()->name(), QString( "Free Music Charts" ) );
}

void TestMetaTrack::testComposer()
{
    QCOMPARE( m_testTrack1->composer().data()->name(), QString( "" ) );
}

void TestMetaTrack::testGenre()
{
    QCOMPARE( m_testTrack1->genre().data()->name(), QString( "Vocal" ) );
}

void TestMetaTrack::testYear()
{
    QCOMPARE( m_testTrack1->year().data()->name(), QString( "2010" ) );
}

void TestMetaTrack::testComment()
{
    QCOMPARE( m_testTrack1->comment(), QString( "" ) );
}

void TestMetaTrack::testSetAndGetScore()
{
    QCOMPARE( m_testTrack1->score(), 0.0 );

    m_testTrack1->setScore( 3 );
    QCOMPARE( m_testTrack1->score(), 3.0 );

    m_testTrack1->setScore( 12.55 );
    QCOMPARE( m_testTrack1->score(), 12.55 );

    m_testTrack1->setScore( 100 );
    QCOMPARE( m_testTrack1->score(), 100.0 );

    m_testTrack1->setScore( -12.55 ); // well...
    QCOMPARE( m_testTrack1->score(), -12.55 );

    m_testTrack1->setScore( 0 );
    QCOMPARE( m_testTrack1->score(), 0.0 );
}

void TestMetaTrack::testSetAndGetRating()
{
    QCOMPARE( m_testTrack1->rating(), 0 );

    m_testTrack1->setRating( 3 );
    QCOMPARE( m_testTrack1->rating(), 3 );

    m_testTrack1->setRating( 10 );
    QCOMPARE( m_testTrack1->rating(), 10 );

    m_testTrack1->setRating( 0 );
    QCOMPARE( m_testTrack1->rating(), 0 );
}

void TestMetaTrack::testLength()
{
    QCOMPARE( m_testTrack1->length(), 12000LL );
}

void TestMetaTrack::testFilesize()
{
    QCOMPARE( m_testTrack1->filesize(), 389454 );
}

void TestMetaTrack::testSampleRate()
{
    QCOMPARE( m_testTrack1->sampleRate(), 44100 );
}

void TestMetaTrack::testBitrate()
{
    QCOMPARE( m_testTrack1->bitrate(), 256 );
}

void TestMetaTrack::testTrackNumber()
{
    QCOMPARE( m_testTrack1->trackNumber(), 0 );
}

void TestMetaTrack::testDiscNumber()
{
    QCOMPARE( m_testTrack1->discNumber(), 0 );
}

void TestMetaTrack::testLastPlayed()
{
    QCOMPARE( m_testTrack1->lastPlayed(), 4294967295U ); // portability?
}

void TestMetaTrack::testFirstPlayed()
{
    QCOMPARE( m_testTrack1->firstPlayed(), 4294967295U ); // portability?
}

void TestMetaTrack::testPlayCount()
{
    QCOMPARE( m_testTrack1->playCount(), 0 );
}

void TestMetaTrack::testReplayGain()
{
    QCOMPARE( int(m_testTrack1->replayGain( Meta::Track::TrackReplayGain ) * 1000), -6655 );
    QCOMPARE( int(m_testTrack1->replayGain( Meta::Track::AlbumReplayGain ) * 1000), -6655 );
}

void TestMetaTrack::testReplayPeakGain()
{
    QCOMPARE( int(m_testTrack1->replayPeakGain( Meta::Track::TrackReplayGain ) * 10000), 41263 );
    QCOMPARE( int(m_testTrack1->replayPeakGain( Meta::Track::AlbumReplayGain ) * 10000), 41263 );
}

void TestMetaTrack::testType()
{
    QCOMPARE( m_testTrack1->type(), QString( "mp3" ) );
}

void TestMetaTrack::testInCollection()
{
    QVERIFY( !m_testTrack1->inCollection() );
}

void TestMetaTrack::testCollection()
{
    QVERIFY( !m_testTrack1->collection() );
}

void TestMetaTrack::testSetAndGetCachedLyrics()
{
    /* TODO: setCachedLyrics is not yet implemented
    QCOMPARE( m_testTrack1->cachedLyrics(), QString( "" ) );

    m_testTrack1->setCachedLyrics( "test" );
    QCOMPARE( m_testTrack1->cachedLyrics(), QString( "test" ) );

    m_testTrack1->setCachedLyrics( "aäaüoöß" );
    QCOMPARE( m_testTrack1->cachedLyrics(), QString( "aäaüoöß" ) );

    m_testTrack1->setCachedLyrics( "" );
    QCOMPARE( m_testTrack1->cachedLyrics(), QString( "" ) );
    */
}

void TestMetaTrack::testOperatorEquals()
{
    QVERIFY( m_testTrack1 == m_testTrack1 );
    QVERIFY( m_testTrack1 != m_testTrack2 );
}

void TestMetaTrack::testLessThan()
{
    Meta::TrackPtr albumTrack1, albumTrack2, albumTrack3;

    albumTrack1 = CollectionManager::instance()->trackForUrl( dataPath( "data/audio/album/Track01.ogg" ) );
    albumTrack2 = CollectionManager::instance()->trackForUrl( dataPath( "data/audio/album/Track02.ogg" ) );
    albumTrack3 = CollectionManager::instance()->trackForUrl( dataPath( "data/audio/album/Track03.ogg" ) );

    QVERIFY( albumTrack1 );
    QVERIFY( albumTrack2 );
    QVERIFY( albumTrack3 );

    QVERIFY( !Meta::Track::lessThan( m_testTrack1, m_testTrack1 ) );

    QVERIFY( Meta::Track::lessThan( albumTrack1, albumTrack2 ) );
    QVERIFY( Meta::Track::lessThan( albumTrack2, albumTrack3 ) );
    QVERIFY( Meta::Track::lessThan( albumTrack1, albumTrack3 ) );
    QVERIFY( !Meta::Track::lessThan( albumTrack3, albumTrack2 ) );
    QVERIFY( !Meta::Track::lessThan( albumTrack3, albumTrack1 ) );
    QVERIFY( !Meta::Track::lessThan( albumTrack3, albumTrack3 ) );
}
