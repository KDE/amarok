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

#include "core/meta/impl/cue/CueFileSupport.h"

#include <KStandardDirs>

#include <QtDebug>

#include <QtTest/QTest>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestCueFileSupport )

using namespace MetaCue;

TestCueFileSupport::TestCueFileSupport()
    : QObject()
{
}

void TestCueFileSupport::testLocateCueFile()
{
  
  //Check that we found the right .cue file and that it passed the validator
  
  //we use a non existant filename here, so only look for the patch and then append filename 
  //as KStandardDirs::locate will return an empty url otherwise. 
  KUrl testUrl = KStandardDirs::locate( "data", "amarok/testdata/cue/" );
  testUrl.addPath( "testsheet01-iso8859-1.mp3" );
  
  KUrl cueTestUrl = KStandardDirs::locate( "data", "amarok/testdata/cue/testsheet01-iso8859-1.cue" );
  KUrl cueResultUrl = CueFileSupport::locateCueSheet( testUrl );
  
  QVERIFY( !cueResultUrl.url().isEmpty() );
  QCOMPARE( cueResultUrl.url(), cueTestUrl.url() );
  
  //Check that a nonexisting cue file returns an empty url
  testUrl = KStandardDirs::locate( "data", "amarok/testdata/cue/test_silence.ogg" );
  cueResultUrl = CueFileSupport::locateCueSheet( testUrl );
  
  QVERIFY( cueResultUrl.isEmpty() );
  
  //check that an existing but invalid cue file returns an empty url
  testUrl = KStandardDirs::locate( "data", "amarok/testdata/cue/invalid.ogg" );
  cueResultUrl = CueFileSupport::locateCueSheet( testUrl );
   
  QVERIFY( cueResultUrl.isEmpty() );
}

void TestCueFileSupport::testIso88591Cue()
{
  
    KUrl testUrl = KStandardDirs::locate( "data", "amarok/testdata/cue/testsheet01-iso8859-1.cue" );   
    CueFileItemMap cueItemMap = CueFileSupport::loadCueFile( testUrl, 48000 );
      
    QCOMPARE( cueItemMap.size(), 14 );
    QCOMPARE( cueItemMap.value( cueItemMap.keys()[2] ).title(), QString( "Disco" ) );
    QCOMPARE( cueItemMap.value( cueItemMap.keys()[2] ).artist(), QString( "Die Toten Hosen" ) );
    
}

void TestCueFileSupport::testUtf8Cue()
{
    KUrl testUrl = KStandardDirs::locate( "data", "amarok/testdata/cue/testsheet01-utf8.cue" );   
    CueFileItemMap cueItemMap = CueFileSupport::loadCueFile( testUrl, 48000 );
      
    QCOMPARE( cueItemMap.size(), 14 );
    QCOMPARE( cueItemMap.value( cueItemMap.keys()[6] ).title(), QString( "Ertrinken" ) );
    QCOMPARE( cueItemMap.value( cueItemMap.keys()[6] ).artist(), QString( "Die Toten Hosen" ) );
}