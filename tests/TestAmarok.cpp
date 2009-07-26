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

#include <QDir>
#include <QString>

TestAmarok::TestAmarok( QStringList testArgumentList )
{
    testArgumentList.replace( 2, testArgumentList.at( 2 ) + "Amarok.log" );
    QTest::qExec( this, testArgumentList );
}

void TestAmarok::testCleanPath()
{
    /* no changes expected here */
    QCOMPARE( Amarok::cleanPath( QString( "" ) ), QString( "" ) );
    QCOMPARE( Amarok::cleanPath( QString( "abcdefghijklmnopqrstuvwxyz" ) ), QString( "abcdefghijklmnopqrstuvwxyz" ) );
    QCOMPARE( Amarok::cleanPath( QString( "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ) ), QString( "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ) );
    QCOMPARE( Amarok::cleanPath( QString( "/\\.,-+" ) ), QString( "/\\.,-+" ) );

    /* German */
    QCOMPARE( Amarok::cleanPath( QString( "äöüß" ) ), QString( "aeoeuess" ) );
    QCOMPARE( Amarok::cleanPath( QString( "ÄÖÜß" ) ), QString( "AeOeUess" ) ); // capital ß only exists in theory in the German language, but had been defined some time ago, iirc

    /* French */
    QCOMPARE( Amarok::cleanPath( QString( "áàéèêô" ) ), QString( "aaeeeo" ) );
    QCOMPARE( Amarok::cleanPath( QString( "ÁÀÉÈÊÔ" ) ), QString( "AAEEEO" ) );
    QCOMPARE( Amarok::cleanPath( QString( "æ" ) ), QString( "ae" ) );

    /* Czech and other east European langauges */
    QCOMPARE( Amarok::cleanPath( QString( "çńǹýỳź" ) ), QString( "cnnyyz" ) );
    QCOMPARE( Amarok::cleanPath( QString( "ÇŃǸÝỲŹ" ) ), QString( "CNNYYZ" ) );
    QCOMPARE( Amarok::cleanPath( QString( "ěĺľôŕřůž" ) ), QString( "ellorruz" ) );
    QCOMPARE( Amarok::cleanPath( QString( "ÁČĎÉĚÍŇÓŘŠŤÚŮÝŽ" ) ), QString( "ACDEEINORSTUUYZ" ) );

    /* Skandinavian languages */
    QCOMPARE( Amarok::cleanPath( QString( "åø" ) ), QString( "ao" ) );
    QCOMPARE( Amarok::cleanPath( QString( "ÅØ" ) ), QString( "AO" ) );

    /* Spanish */
    QCOMPARE( Amarok::cleanPath( QString( "ñóÿ" ) ), QString( "noy" ) );
    QCOMPARE( Amarok::cleanPath( QString( "ÑÓŸ" ) ), QString( "NOY" ) );

    /* if they exist: add missing ones here */
}

void TestAmarok::testComputeScore()
{
     QVERIFY( 50 < Amarok::computeScore( 50, 1,  1 ) ); // greater score if played completely
     QVERIFY(  0 < Amarok::computeScore(  0, 1,  1 ) ); // handle 0 score
     QVERIFY( 50 > Amarok::computeScore( 50, 1,  0.1 ) ); // lower score if aborted early
     QVERIFY( 50 > Amarok::computeScore( 50, 1,  0 ) ); // handle 0% played fraction
     QVERIFY( 50 > Amarok::computeScore( 50, 0,  0 ) ); // handle 0 playcount
}

void TestAmarok::testConciseTimeSince()
{
    QCOMPARE( Amarok::conciseTimeSince( 0 ).isEmpty(), false );
    QCOMPARE( Amarok::conciseTimeSince( 10 ).isEmpty(), false );
    QCOMPARE( Amarok::conciseTimeSince( 100 ).isEmpty(), false );
    QCOMPARE( Amarok::conciseTimeSince( 1000 ).isEmpty(), false );
    /* any other good ideas what to test here? */
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

void TestAmarok::testManipulateThe()
{
    QString teststring;

    Amarok::manipulateThe( teststring = "", true );
    QCOMPARE( teststring, QString( "" ) );

    Amarok::manipulateThe( teststring = "", false );
    QCOMPARE( teststring, QString( "" ) );

    Amarok::manipulateThe( teststring = "A", true );
    QCOMPARE( teststring, QString( "A" ) );

    Amarok::manipulateThe( teststring = "A", false );
    QCOMPARE( teststring, QString( "A" ) );

    Amarok::manipulateThe( teststring = "ABC", true );
    QCOMPARE( teststring, QString( "ABC" ) );

    Amarok::manipulateThe( teststring = "ABC", false );
    QCOMPARE( teststring, QString( "ABC" ) );

    Amarok::manipulateThe( teststring = "The Eagles", true );
    QCOMPARE( teststring, QString( "Eagles, The" ) );

    Amarok::manipulateThe( teststring = "Eagles, The", false );
    QCOMPARE( teststring, QString( "The Eagles" ) );

    Amarok::manipulateThe( teststring = "The The", true );
    QCOMPARE( teststring, QString( "The, The" ) );

    Amarok::manipulateThe( teststring = "The, The", false );
    QCOMPARE( teststring, QString( "The The" ) );

    Amarok::manipulateThe( teststring = "Something else", true );
    QCOMPARE( teststring, QString( "Something else" ) );

    Amarok::manipulateThe( teststring = "The Äöü", true );
    QCOMPARE( teststring, QString( "Äöü, The" ) );

    Amarok::manipulateThe( teststring = "Äöü, The", false );
    QCOMPARE( teststring, QString( "The Äöü" ) );
}

void TestAmarok::testSaveLocation()
{
    QString saveLocation = Amarok::saveLocation();
    QDir saveLocationDir( saveLocation );

    QCOMPARE( saveLocationDir.exists(), true );
    QCOMPARE( QDir::isAbsolutePath( saveLocation ), true );
    QCOMPARE( saveLocationDir.isReadable(), true );
    /* any other good ideas what to test here? */
}

void TestAmarok::testVerboseTimeSince()
{
    /* There are two overloaded variants of this function */
    QCOMPARE( Amarok::verboseTimeSince( 0 ).isEmpty(), false );
    QCOMPARE( Amarok::verboseTimeSince( QDateTime::fromTime_t( 0 ) ).isEmpty(), false );

    QCOMPARE( Amarok::verboseTimeSince( 10 ).isEmpty(), false );
    QCOMPARE( Amarok::verboseTimeSince( QDateTime::fromTime_t( 10 ) ).isEmpty(), false );

    QCOMPARE( Amarok::verboseTimeSince( 100 ).isEmpty(), false );
    QCOMPARE( Amarok::verboseTimeSince( QDateTime::fromTime_t( 100 ) ).isEmpty(), false );

    QCOMPARE( Amarok::verboseTimeSince( 1000 ).isEmpty(), false );
    QCOMPARE( Amarok::verboseTimeSince( QDateTime::fromTime_t( 1000 ) ).isEmpty(), false );
    /* any other good ideas what to test here? */
}

