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

#include "TestCaseConverter.h"

#include "CaseConverter.h"
#include "Debug.h"

#include <QtTest/QTest>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestCaseConverter )

//required for Debug.h
QMutex Debug::mutex;

TestCaseConverter::TestCaseConverter()
{
}

void TestCaseConverter::testToCapitalizedCase()
{
    QCOMPARE( Amarok::CaseConverter::toCapitalizedCase( "" ), QString( "" ) );

    QCOMPARE( Amarok::CaseConverter::toCapitalizedCase( "A" ), QString( "A" ) );
    QCOMPARE( Amarok::CaseConverter::toCapitalizedCase( "a" ), QString( "A" ) );
    QCOMPARE( Amarok::CaseConverter::toCapitalizedCase( "A tale of true love" ), QString( "A Tale Of True Love" ) );
    QCOMPARE( Amarok::CaseConverter::toCapitalizedCase( "A horse with no name" ), QString( "A Horse With No Name" ) );
    QCOMPARE( Amarok::CaseConverter::toCapitalizedCase( "riding on a dead horse" ), QString( "Riding On A Dead Horse" ) );
    // ätest -> Ätest
    QCOMPARE( Amarok::CaseConverter::toCapitalizedCase( QChar( 0x00E4 ) + QString( "test" ) ), QChar( 0x00C4 ) + QString( "test" ) );
    QCOMPARE( Amarok::CaseConverter::toCapitalizedCase( "a an in of on" ), QString( "A An In Of On" ) );
}

void TestCaseConverter::testToTitleCase()
{
    QCOMPARE( Amarok::CaseConverter::toTitleCase( "" ), QString( "" ) );

    QCOMPARE( Amarok::CaseConverter::toTitleCase( "A" ), QString( "A" ) );
    QCOMPARE( Amarok::CaseConverter::toTitleCase( "a" ), QString( "A" ) );
    QCOMPARE( Amarok::CaseConverter::toTitleCase( "a tale of true love" ), QString( "A Tale of True Love" ) );
    QCOMPARE( Amarok::CaseConverter::toTitleCase( "a horse with no name" ), QString( "A Horse With No Name" ) );
    QCOMPARE( Amarok::CaseConverter::toTitleCase( "riding on a dead horse" ), QString( "Riding on a Dead Horse" ) );
    // ätest -> Ätest
    QCOMPARE( Amarok::CaseConverter::toTitleCase( QChar( 0x00E4 ) + QString( "test" ) ), QChar( 0x00C4 ) + QString( "test" ) );
    QCOMPARE( Amarok::CaseConverter::toTitleCase( "a a an in of on" ), QString( "A a an in of on" ) );
}
