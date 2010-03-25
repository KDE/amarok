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

#include "TestTimecodeTrackProvider.h"
#include "config-amarok-test.h"
#include "core/meta/impl/timecode/TimecodeTrackProvider.h"

#include <QtTest/QTest>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestTimecodeTrackProvider )

TestTimecodeTrackProvider::TestTimecodeTrackProvider()
{}

void TestTimecodeTrackProvider::initTestCase()
{
    m_testProvider = new TimecodeTrackProvider();
}

void TestTimecodeTrackProvider::cleanupTestCase()
{
    delete m_testProvider;
}

QString
TestTimecodeTrackProvider::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + '/' + relPath );
}

void TestTimecodeTrackProvider::testPossiblyContainsTrack()
{
    QVERIFY( !m_testProvider->possiblyContainsTrack( KUrl( "file:///home/test/test.mp3" ) ) );
    QVERIFY( m_testProvider->possiblyContainsTrack( KUrl( "file:///home/test/test.mp3:0-23" ) ) );
    QVERIFY( m_testProvider->possiblyContainsTrack( KUrl( "file:///home/test/test.mp3:23-42" ) ) );
    QVERIFY( m_testProvider->possiblyContainsTrack( KUrl( "file:///home/test/test.mp3:42-23" ) ) );
    QVERIFY( !m_testProvider->possiblyContainsTrack( KUrl( "file:///home/test/test.mp3:-12-42" ) ) );
}

void TestTimecodeTrackProvider::testTrackForUrl()
{
    KUrl testUrl;
    testUrl = dataPath( "data/audio/album/" );
    testUrl.addPath( "Track01.ogg:23-42" );

    Meta::TrackPtr resultTrack = m_testProvider->trackForUrl( testUrl );

    QVERIFY( resultTrack );

    QCOMPARE( resultTrack->playableUrl().pathOrUrl(), dataPath( "data/audio/album/Track01.ogg" ) );
}
