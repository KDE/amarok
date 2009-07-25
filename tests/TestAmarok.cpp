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

#include "Amarok.h"
#include "TestAmarok.h"

#include <QString>

TestAmarok::TestAmarok( QStringList testArgumentList )
{
    testArgumentList.replace( 2, testArgumentList.at( 2 ) + "Amarok.log" );
    QTest::qExec( this, testArgumentList );
}

void TestAmarok::testComputeScore()
{
     QVERIFY( 50 < Amarok::computeScore( 50, 1,  1 ) ); // greater score if played completely
     QVERIFY(  0 < Amarok::computeScore(  0, 1,  1 ) ); // handle 0 score
     QVERIFY( 50 > Amarok::computeScore( 50, 1,  0.1 ) ); // lower score if aborted early
     QVERIFY( 50 > Amarok::computeScore( 50, 1,  0 ) ); // handle 0% played fraction
     QVERIFY( 50 > Amarok::computeScore( 50, 0,  0 ) ); // handle 0 playcount
}

void TestAmarok::testExtension()
{
    QCOMPARE( Amarok::extension( "" ), QString( "" ) );
    QCOMPARE( Amarok::extension( "..." ), QString( "" ) );
    QCOMPARE( Amarok::extension( "test" ), QString( "" ) );
    QCOMPARE( Amarok::extension( "test.mp3" ), QString( "mp3" ) );
    QCOMPARE( Amarok::extension( "test.mP3" ), QString( "mp3" ) );
    QCOMPARE( Amarok::extension( "test.MP3" ), QString( "mp3" ) );
    QCOMPARE( Amarok::extension( "test.longextension" ), QString( "longextension" ) );
    QCOMPARE( Amarok::extension( "test.long.extension" ), QString( "extension" ) );
    QCOMPARE( Amarok::extension( "test.m" ), QString( "m" ) );
    QCOMPARE( Amarok::extension( "test.äöü" ), QString( "äöü" ) );
    QCOMPARE( Amarok::extension( "test.ÄÖÜ" ), QString( "äöü" ) );
    QCOMPARE( Amarok::extension( "..test.mp3" ), QString( "mp3" ) );
    QCOMPARE( Amarok::extension( "..te st.mp3" ), QString( "mp3" ) );
    QCOMPARE( Amarok::extension( "..te st.m p3" ), QString( "m p3" ) );
}

