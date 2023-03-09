/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@asbest-online.de>               *
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

#include "TestQStringx.h"

#include "core/support/Debug.h"

#include <QTest>
#include <QString>
#include <QMap>

QTEST_GUILESS_MAIN( TestQStringx )

//required for Debug.h
QRecursiveMutex Debug::mutex;

TestQStringx::TestQStringx()
{
}

void TestQStringx::testArgs()
{
    QStringList testArgs;

    m_testString = Amarok::QStringx("");
    QCOMPARE( m_testString.args( testArgs ) , QString( "" ) );

    m_testString = Amarok::QStringx("test");
    QCOMPARE( m_testString.args( testArgs ), QString( "test" ) );

    m_testString = Amarok::QStringx("");
    testArgs.append( "test" );
    QCOMPARE( m_testString.args( testArgs ), QString( "" ) );

    m_testString = Amarok::QStringx("test%12abc");
    QCOMPARE( m_testString.args( testArgs ) , QString( "testtestabc" ) );

    m_testString = Amarok::QStringx("%12test abc");
    QCOMPARE( m_testString.args( testArgs ) , QString( "testtest abc" ) );

    m_testString = Amarok::QStringx("te%st%12abc");
    QCOMPARE( m_testString.args( testArgs ) , QString( "te%sttestabc" ) );

    testArgs.clear();
    testArgs.append( "test" );
    testArgs.append( "abc" );
    m_testString = Amarok::QStringx("test%12abc%2xyz");
    QCOMPARE( m_testString.args( testArgs ) , QString( "testtestabcabcxyz" ) );

    m_testString = Amarok::QStringx("%12test%23abc");
    QCOMPARE( m_testString.args( testArgs ) , QString( "testtestabcabc" ) );
}

void TestQStringx::testNamedArgs()
{
    QMap<QString, QString> testArgs;

    m_testString = Amarok::QStringx("");
    QCOMPARE( m_testString.namedArgs( testArgs ) , QString( "" ) );

    m_testString = Amarok::QStringx("test");
    QCOMPARE( m_testString.namedArgs( testArgs ) , QString( "test" ) );

    testArgs[ "artist" ] = "Pornophonique";
    m_testString = Amarok::QStringx("test");
    QCOMPARE( m_testString.namedArgs( testArgs ) , QString( "test" ) );

    m_testString = Amarok::QStringx("artist: %artist%");
    QCOMPARE( m_testString.namedArgs( testArgs ) , QString( "artist: Pornophonique" ) );

    m_testString = Amarok::QStringx("artist: %artist% - %album%");
    QCOMPARE( m_testString.namedArgs( testArgs ) , QString( "artist: Pornophonique - " ) );

    testArgs[ "album" ] = "8-Bit Lagerfeuer";
    QCOMPARE( m_testString.namedArgs( testArgs ) , QString( "artist: Pornophonique - 8-Bit Lagerfeuer" ) );

    m_testString = Amarok::QStringx("%artist%: %artist% - %album%");
    QCOMPARE( m_testString.namedArgs( testArgs ) , QString( "Pornophonique: Pornophonique - 8-Bit Lagerfeuer" ) );

    testArgs[ "year" ] = "2007";
    QCOMPARE( m_testString.namedArgs( testArgs ) , QString( "Pornophonique: Pornophonique - 8-Bit Lagerfeuer" ) );
}

void TestQStringx::testNamedOptArgs()
{
    QMap<QString, QString> testArgs;

    m_testString = Amarok::QStringx("");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "" ) );

    m_testString = Amarok::QStringx("test");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "test" ) );

    m_testString = Amarok::QStringx("%test%");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "" ) );

    m_testString = Amarok::QStringx("{ %test% }");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "" ) );

    m_testString = Amarok::QStringx("test{%test%}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "test" ) );

    m_testString = Amarok::QStringx("{test{%test%}}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "test" ) );

    m_testString = Amarok::QStringx("%test%{%test%}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "" ) );

    m_testString = Amarok::QStringx("test%test% ");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "test " ) );

    testArgs[ "artist" ] = "All:My:Faults";
    m_testString = Amarok::QStringx("%artist%");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "All:My:Faults" ) );

    m_testString = Amarok::QStringx("{%test% }{%artist%}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "All:My:Faults" ) );

    m_testString = Amarok::QStringx("{%test% {%artist%}}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "" ) );

    m_testString = Amarok::QStringx("{%artist% {%test%}}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "All:My:Faults " ) );

    testArgs[ "track" ] = "Some track";
    m_testString = Amarok::QStringx("{%test% {%artist%}}%track%");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "Some track" ) );

    m_testString = Amarok::QStringx("{%artist% {%track%}} %test%");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "All:My:Faults Some track " ) );

    testArgs[ "test" ] = "";
    m_testString = Amarok::QStringx("{%test%}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "" ) );

    m_testString = Amarok::QStringx("before{%test%}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "before" ) );

    m_testString = Amarok::QStringx("{%test%}after");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "after" ) );

    m_testString = Amarok::QStringx("before{%test%}after");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "beforeafter" ) );

    m_testString = Amarok::QStringx("{%test% }{%artist%}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "All:My:Faults" ) );

    m_testString = Amarok::QStringx("{%test% {%artist%}}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "" ) );

    m_testString = Amarok::QStringx("{%artist% {%test%}}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QString( "All:My:Faults " ) );

    m_testString = Amarok::QStringx("[%test2%:test {%artist%}%test%{ [%test%]}]");
    QCOMPARE( m_testString.namedOptArgs( testArgs ), QString( "test All:My:Faults Unknown test" ) );
}
