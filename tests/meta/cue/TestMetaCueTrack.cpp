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

#include "meta/cue/CueFileSupport.h"

#include <KStandardDirs>

#include <QtTest/QTest>
#include <QtCore/QDir>
#include <QtCore/QString>


TestMetaCueTrack::TestMetaCueTrack( const QStringList args, const QString &logPath )
    : TestBase( "MetaCueTrack" )
{
    QStringList combinedArgs = args;
    addLogging( combinedArgs, logPath );
    QTest::qExec( this, combinedArgs );
}

void TestMetaCueTrack::initTestCase()
{
    m_isoCuePath = new QString( KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/cue/testsheet01-iso8859-1.cue" ) );
    m_utfCuePath = new QString( KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/cue/testsheet01-utf8.cue" ) );
    m_testSongPath = new QString( KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/cue/test_silence.ogg" ) );
}

void TestMetaCueTrack::cleanupTestCase()
{
    delete m_isoCuePath;
    delete m_utfCuePath;
    delete m_testSongPath;
}

void TestMetaCueTrack::testCueItems()
{

    MetaCue::CueFileItemMap cueContentMap = MetaCue::CueFileSupport::loadCueFile( KUrl( *m_isoCuePath ), 3000000 );

    QCOMPARE( cueContentMap[ 0 ].getArtist(), QString( "Die Toten Hosen" ) );
    QCOMPARE( cueContentMap[ 0 ].getAlbum(), QString( "In aller Stille (2008)" ) );
    QCOMPARE( cueContentMap[ 0 ].getTitle(), QString( "Strom" ) );
    QCOMPARE( cueContentMap[ 0 ].getTrackNumber(), 1 );

    QCOMPARE( cueContentMap[ 727920 ].getArtist(), QString( "Die Toten Hosen" ) );
    QCOMPARE( cueContentMap[ 727920 ].getAlbum(), QString( "In aller Stille (2008)" ) );
    QCOMPARE( cueContentMap[ 727920 ].getTitle(), QString::fromUtf8( "Auflösen" ) );
    QCOMPARE( cueContentMap[ 727920 ].getTrackNumber(), 5 );


    cueContentMap = MetaCue::CueFileSupport::loadCueFile( KUrl( *m_utfCuePath ), 3000000 );
    QCOMPARE( cueContentMap[ 0 ].getArtist(), QString( "Die Toten Hosen" ) );
    QCOMPARE( cueContentMap[ 0 ].getAlbum(), QString( "In aller Stille (2008)" ) );
    QCOMPARE( cueContentMap[ 0 ].getTitle(), QString( "Strom" ) );
    QCOMPARE( cueContentMap[ 0 ].getTrackNumber(), 1 );

    QCOMPARE( cueContentMap[ 727920 ].getArtist(), QString( "Die Toten Hosen" ) );
    QCOMPARE( cueContentMap[ 727920 ].getAlbum(), QString( "In aller Stille (2008)" ) );
    QCOMPARE( cueContentMap[ 727920 ].getTitle(), QString::fromUtf8( "Auflösen" ) );
    QCOMPARE( cueContentMap[ 727920 ].getTrackNumber(), 5 );
}

void TestMetaCueTrack::testLocateCueSheet()
{
    QCOMPARE( MetaCue::CueFileSupport::locateCueSheet( QString( "" ) ).toLocalFile(), QString( "" ) );
    QCOMPARE( MetaCue::CueFileSupport::locateCueSheet( KStandardDirs::installPath( "data" ) ).toLocalFile(), QString( "" ) );

    QCOMPARE( MetaCue::CueFileSupport::locateCueSheet( KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/cue/testsheet01-utf8.ogg" ) ).toLocalFile(), *m_utfCuePath );
    QCOMPARE( MetaCue::CueFileSupport::locateCueSheet( KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/cue/testsheet01-iso8859-1.ogg" ) ).toLocalFile(), *m_isoCuePath );
}

void TestMetaCueTrack::testValidateCueSheet()
{
    QVERIFY( !MetaCue::CueFileSupport::validateCueSheet( "" ) );
    QVERIFY( !MetaCue::CueFileSupport::validateCueSheet( KStandardDirs::installPath( "data" ) ) );

    QVERIFY( MetaCue::CueFileSupport::validateCueSheet( *m_isoCuePath ) );
    QVERIFY( MetaCue::CueFileSupport::validateCueSheet( *m_utfCuePath ) );
    QVERIFY( !MetaCue::CueFileSupport::validateCueSheet( *m_testSongPath) );
}

