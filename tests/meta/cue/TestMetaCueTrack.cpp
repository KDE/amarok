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

#include "TestMetaCueTrack.h"

#include <KStandardDirs>


TestMetaCueTrack::TestMetaCueTrack( QStringList testArgumentList )
{
    testArgumentList.replace( 2, testArgumentList.at( 2 ) + "MetaCueTrack.log" );
    QTest::qExec( this, testArgumentList );
}

void TestMetaCueTrack::initTestCase()
{
    m_isoCuePath = new QString( KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/cue/testsheet01-iso8859-1.cue" ) );
    m_utfCuePath = new QString( KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/cue/testsheet01-utf8.cue" ) );
    m_testSongPath = new QString( KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/cue/test_silence.ogg" ) );

    m_testTrack1 = new MetaCue::Track( *m_testSongPath, *m_isoCuePath );
    m_testTrack2 = new MetaCue::Track( *m_testSongPath, *m_utfCuePath );
}

void TestMetaCueTrack::cleanupTestCase()
{
    delete m_isoCuePath;
    delete m_utfCuePath;
    delete m_testSongPath;

    delete m_testTrack1;
    delete m_testTrack2;
}

void TestMetaCueTrack::testCueItems()
{
    MetaCue::CueFileItemMap cueContentMap = m_testTrack1->cueItems();

    QCOMPARE( cueContentMap[ 0 ].getArtist(), QString( "Die Toten Hosen" ) );
    QCOMPARE( cueContentMap[ 0 ].getAlbum(), QString( "In aller Stille (2008)" ) );
    QCOMPARE( cueContentMap[ 0 ].getTitle(), QString( "Strom" ) );
    QCOMPARE( cueContentMap[ 0 ].getTrackNumber(), 1 );

    QCOMPARE( cueContentMap[ 727920 ].getArtist(), QString( "Die Toten Hosen" ) );
    QCOMPARE( cueContentMap[ 727920 ].getAlbum(), QString( "In aller Stille (2008)" ) );
    QCOMPARE( cueContentMap[ 727920 ].getTitle(), QString( "Auflösen" ) );
    QCOMPARE( cueContentMap[ 727920 ].getTrackNumber(), 5 );


    cueContentMap = m_testTrack2->cueItems();
    QCOMPARE( cueContentMap[ 0 ].getArtist(), QString( "Die Toten Hosen" ) );
    QCOMPARE( cueContentMap[ 0 ].getAlbum(), QString( "In aller Stille (2008)" ) );
    QCOMPARE( cueContentMap[ 0 ].getTitle(), QString( "Strom" ) );
    QCOMPARE( cueContentMap[ 0 ].getTrackNumber(), 1 );

    QCOMPARE( cueContentMap[ 727920 ].getArtist(), QString( "Die Toten Hosen" ) );
    QCOMPARE( cueContentMap[ 727920 ].getAlbum(), QString( "In aller Stille (2008)" ) );
    QCOMPARE( cueContentMap[ 727920 ].getTitle(), QString( "Auflösen" ) );
    QCOMPARE( cueContentMap[ 727920 ].getTrackNumber(), 5 );
}

void TestMetaCueTrack::testLocateCueSheet()
{
    QCOMPARE( MetaCue::Track::locateCueSheet( QString( "" ) ).toLocalFile(), QString( "" ) );
    QCOMPARE( MetaCue::Track::locateCueSheet( KStandardDirs::installPath( "data" ) ).toLocalFile(), QString( "" ) );

    QCOMPARE( MetaCue::Track::locateCueSheet( KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/cue/testsheet01-utf8.ogg" ) ).toLocalFile(), *m_utfCuePath );
    QCOMPARE( MetaCue::Track::locateCueSheet( KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/cue/testsheet01-iso8859-1.ogg" ) ).toLocalFile(), *m_isoCuePath );
}

void TestMetaCueTrack::testValidateCueSheet()
{
    QVERIFY( !MetaCue::Track::validateCueSheet( "" ) );
    QVERIFY( !MetaCue::Track::validateCueSheet( KStandardDirs::installPath( "data" ) ) );

    QVERIFY( MetaCue::Track::validateCueSheet( *m_isoCuePath ) );
    QVERIFY( MetaCue::Track::validateCueSheet( *m_utfCuePath ) );
    QVERIFY( !MetaCue::Track::validateCueSheet( *m_testSongPath) );
}

void TestMetaCueTrack::testName()
{
    QCOMPARE( m_testTrack1->name(), QString( "Test Silence" ) );
}

void TestMetaCueTrack::testPrettyName()
{
    QCOMPARE( m_testTrack1->prettyName(), QString( "Test Silence" ) );
}

void TestMetaCueTrack::testFullPrettyName()
{
    QCOMPARE( m_testTrack1->fullPrettyName(), QString( "Test Silence" ) );
}

void TestMetaCueTrack::testSortableName()
{
    QCOMPARE( m_testTrack1->sortableName(), QString( "Test Silence" ) );
}

void TestMetaCueTrack::testTrackNumber()
{
    QCOMPARE( m_testTrack1->trackNumber(), 0 );
}

void TestMetaCueTrack::testLength()
{
    QCOMPARE( m_testTrack1->length(), 0 ); // why?
}

void TestMetaCueTrack::testAlbum()
{
    QCOMPARE( m_testTrack1->album()->name(), QString( "" ) );
}

void TestMetaCueTrack::testArtist()
{
    QCOMPARE( m_testTrack1->artist()->name(), QString( "Amarok" ) );
}

void TestMetaCueTrack::testHasCapabilityInterface()
{
}
