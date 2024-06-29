/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.com>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "TestCueFileSupport.h"

#include "config-amarok-test.h"
#include "core-impl/meta/cue/CueFileSupport.h"

#include <QtDebug>

#include <QTest>


QTEST_GUILESS_MAIN( TestCueFileSupport )

using namespace MetaCue;

TestCueFileSupport::TestCueFileSupport()
    : QObject()
{
}

void TestCueFileSupport::testLocateCueFile()
{
  //Check that we find the right .cue file and that it passes the validator
  QUrl cueTestUrl = QUrl::fromLocalFile(dataPath( QStringLiteral("data/cue/testsheet01-iso8859-1.cue") ));
  QUrl cueResultUrl = CueFileSupport::locateCueSheet( cueTestUrl );

  QVERIFY( !cueResultUrl.url().isEmpty() );
  QCOMPARE( cueResultUrl.url(), cueTestUrl.url() );

  //Check that a nonexisting cue file returns an empty url
  QUrl testUrl = QUrl::fromLocalFile(dataPath( QStringLiteral("data/cue/test_silence.ogg") ));
  cueResultUrl = CueFileSupport::locateCueSheet( testUrl );

  QVERIFY( cueResultUrl.isEmpty() );

  //Check that an existing but invalid cue file returns an empty url
  testUrl = QUrl::fromLocalFile(dataPath( QStringLiteral("data/cue/invalid.cue") ));
  cueResultUrl = CueFileSupport::locateCueSheet( testUrl );

  QVERIFY( cueResultUrl.isEmpty() );
}

void TestCueFileSupport::testIso88591Cue()
{
    QUrl testUrl = QUrl::fromLocalFile(dataPath( QStringLiteral("data/cue/testsheet01-iso8859-1.cue") ));
    QUrl testTrackUrl = QUrl::fromLocalFile((QStringLiteral( "Die Toten Hosen - In aller Stille (2008).mp3" )));
    CueFileItemMap cueItemMap = CueFileSupport::loadCueFile( testUrl, testTrackUrl, 48000 );

    QCOMPARE( cueItemMap.size(), 14 );
    QCOMPARE( cueItemMap.value( cueItemMap.keys()[2] ).title(), QStringLiteral( "Disco" ) );
    QCOMPARE( cueItemMap.value( cueItemMap.keys()[2] ).artist(), QStringLiteral( "Die Toten Hosen" ) );
}

void TestCueFileSupport::testUtf8Cue()
{
    QUrl testUrl = QUrl::fromLocalFile(dataPath( QStringLiteral("data/cue/testsheet01-utf8.cue") ));
    QUrl testTrackUrl = QUrl::fromLocalFile(QStringLiteral( "Die Toten Hosen - In aller Stille (2008).mp3" ));
    CueFileItemMap cueItemMap = CueFileSupport::loadCueFile( testUrl, testTrackUrl, 48000 );

    QCOMPARE( cueItemMap.size(), 14 );
    QCOMPARE( cueItemMap.value( cueItemMap.keys()[6] ).title(), QStringLiteral( "Ertrinken" ) );
    QCOMPARE( cueItemMap.value( cueItemMap.keys()[6] ).artist(), QStringLiteral( "Die Toten Hosen" ) );
}

QString TestCueFileSupport::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QStringLiteral( AMAROK_TEST_DIR ) + QLatin1Char('/') + relPath );
}
