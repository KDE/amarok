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
    QCOMPARE( m_testString.args( testArgs ) , QStringLiteral( "" ) );

    m_testString = Amarok::QStringx("test");
    QCOMPARE( m_testString.args( testArgs ), QStringLiteral( "test" ) );

    m_testString = Amarok::QStringx("");
    testArgs.append( QStringLiteral("test") );
    QCOMPARE( m_testString.args( testArgs ), QStringLiteral( "" ) );

    m_testString = Amarok::QStringx("test%12abc");
    QCOMPARE( m_testString.args( testArgs ) , QStringLiteral( "testtestabc" ) );

    m_testString = Amarok::QStringx("%12test abc");
    QCOMPARE( m_testString.args( testArgs ) , QStringLiteral( "testtest abc" ) );

    m_testString = Amarok::QStringx("te%st%12abc");
    QCOMPARE( m_testString.args( testArgs ) , QStringLiteral( "te%sttestabc" ) );

    testArgs.clear();
    testArgs.append( QStringLiteral("test") );
    testArgs.append( QStringLiteral("abc") );
    m_testString = Amarok::QStringx("test%12abc%2xyz");
    QCOMPARE( m_testString.args( testArgs ) , QStringLiteral( "testtestabcabcxyz" ) );

    m_testString = Amarok::QStringx("%12test%23abc");
    QCOMPARE( m_testString.args( testArgs ) , QStringLiteral( "testtestabcabc" ) );
}

void TestQStringx::testNamedArgs()
{
    QMap<QString, QString> testArgs;

    m_testString = Amarok::QStringx("");
    QCOMPARE( m_testString.namedArgs( testArgs ) , QStringLiteral( "" ) );

    m_testString = Amarok::QStringx("test");
    QCOMPARE( m_testString.namedArgs( testArgs ) , QStringLiteral( "test" ) );

    testArgs[ QStringLiteral("artist") ] = QStringLiteral("Pornophonique");
    m_testString = Amarok::QStringx("test");
    QCOMPARE( m_testString.namedArgs( testArgs ) , QStringLiteral( "test" ) );

    m_testString = Amarok::QStringx("artist: %artist%");
    QCOMPARE( m_testString.namedArgs( testArgs ) , QStringLiteral( "artist: Pornophonique" ) );

    m_testString = Amarok::QStringx("artist: %artist% - %album%");
    QCOMPARE( m_testString.namedArgs( testArgs ) , QStringLiteral( "artist: Pornophonique - " ) );

    testArgs[ QStringLiteral("album") ] = QStringLiteral("8-Bit Lagerfeuer");
    QCOMPARE( m_testString.namedArgs( testArgs ) , QStringLiteral( "artist: Pornophonique - 8-Bit Lagerfeuer" ) );

    m_testString = Amarok::QStringx("%artist%: %artist% - %album%");
    QCOMPARE( m_testString.namedArgs( testArgs ) , QStringLiteral( "Pornophonique: Pornophonique - 8-Bit Lagerfeuer" ) );

    testArgs[ QStringLiteral("year") ] = QStringLiteral("2007");
    QCOMPARE( m_testString.namedArgs( testArgs ) , QStringLiteral( "Pornophonique: Pornophonique - 8-Bit Lagerfeuer" ) );
}

void TestQStringx::testNamedOptArgs()
{
    QMap<QString, QString> testArgs;

    m_testString = Amarok::QStringx("");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "" ) );

    m_testString = Amarok::QStringx("test");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "test" ) );

    m_testString = Amarok::QStringx("%test%");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "" ) );

    m_testString = Amarok::QStringx("{ %test% }");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "" ) );

    m_testString = Amarok::QStringx("test{%test%}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "test" ) );

    m_testString = Amarok::QStringx("{test{%test%}}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "test" ) );

    m_testString = Amarok::QStringx("%test%{%test%}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "" ) );

    m_testString = Amarok::QStringx("test%test% ");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "test " ) );

    testArgs[ QStringLiteral("artist") ] = QStringLiteral("All:My:Faults");
    m_testString = Amarok::QStringx("%artist%");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "All:My:Faults" ) );

    m_testString = Amarok::QStringx("{%test% }{%artist%}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "All:My:Faults" ) );

    m_testString = Amarok::QStringx("{%test% {%artist%}}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "" ) );

    m_testString = Amarok::QStringx("{%artist% {%test%}}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "All:My:Faults " ) );

    testArgs[ QStringLiteral("track") ] = QStringLiteral("Some track");
    m_testString = Amarok::QStringx("{%test% {%artist%}}%track%");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "Some track" ) );

    m_testString = Amarok::QStringx("{%artist% {%track%}} %test%");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "All:My:Faults Some track " ) );

    testArgs[ QStringLiteral("test") ] = QStringLiteral("");
    m_testString = Amarok::QStringx("{%test%}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "" ) );

    m_testString = Amarok::QStringx("before{%test%}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "before" ) );

    m_testString = Amarok::QStringx("{%test%}after");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "after" ) );

    m_testString = Amarok::QStringx("before{%test%}after");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "beforeafter" ) );

    m_testString = Amarok::QStringx("{%test% }{%artist%}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "All:My:Faults" ) );

    m_testString = Amarok::QStringx("{%test% {%artist%}}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "" ) );

    m_testString = Amarok::QStringx("{%artist% {%test%}}");
    QCOMPARE( m_testString.namedOptArgs( testArgs ) , QStringLiteral( "All:My:Faults " ) );

    m_testString = Amarok::QStringx("[%test2%:test {%artist%}%test%{ [%test%]}]");
    QCOMPARE( m_testString.namedOptArgs( testArgs ), QStringLiteral( "test All:My:Faults Unknown test" ) );
}
