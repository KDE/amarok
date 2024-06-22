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

#include "TestExpression.h"

#include "core/support/Debug.h"
#include "core-impl/collections/support/Expression.h"

#include <QTest>

QTEST_GUILESS_MAIN( TestExpression )

//required for Debug.h
QRecursiveMutex Debug::mutex;

TestExpression::TestExpression()
{
}

void TestExpression::testParse()
{
    ParsedExpression result;
    expression_element element;
    result = ExpressionParser::parse( QStringLiteral("") );
    QCOMPARE( result.isEmpty(), true );

    result = ExpressionParser::parse( QStringLiteral("love artist:cure album:\"Best of\" year:<1990 playcount:>2 -score:<50") );
    int i = 6;
    QCOMPARE( result.size(), i );

    while( !result.isEmpty() )
    {
        element = result.takeFirst().takeFirst();

        if( element.text == QStringLiteral("love") )
        {
            QCOMPARE( element.field, QStringLiteral( "" ) );
            QCOMPARE( element.negate, false );
            QVERIFY( element.match == expression_element::Contains );
        }

        else if( element.text == QStringLiteral("cure") )
        {
            QCOMPARE( element.field, QStringLiteral( "artist" ) );
            QCOMPARE( element.negate, false );
            QVERIFY( element.match == expression_element::Contains );
        }

        else if( element.text == QStringLiteral("Best of") )
        {
            QCOMPARE( element.field, QStringLiteral( "album" ) );
            QCOMPARE( element.negate, false );
            QVERIFY( element.match == expression_element::Contains );
        }

        else if( element.text == QStringLiteral("1990") )
        {
            QCOMPARE( element.field, QStringLiteral( "year" ) );
            QCOMPARE( element.negate, false );
            QVERIFY( element.match == expression_element::Less );
        }

        else if( element.text == QStringLiteral("2") )
        {
            QCOMPARE( element.field, QStringLiteral( "playcount" ) );
            QCOMPARE( element.negate, false );
            QVERIFY( element.match == expression_element::More );
        }

        else if( element.text == QStringLiteral("50") )
        {
            QCOMPARE( element.field, QStringLiteral( "score" ) );
            QCOMPARE( element.negate, true );
            QVERIFY( element.match == expression_element::Less );
        }

        i--;
        QCOMPARE( result.size(), i );
    }

    /* another more complex one */
    result = ExpressionParser::parse( QStringLiteral("artist:cure OR album:\"Best of\" OR year:2009") );
    i = 1;
    QCOMPARE( result.size(), i ); // only 1 or_list

    QList<expression_element> elementList;

    if( !result.isEmpty() )
        elementList = result.at(0);

    QCOMPARE( elementList.size(), 3 );

    while( !elementList.isEmpty() )
    {
        element = elementList.takeFirst();

        if( element.text == QStringLiteral("cure") )
        {
            QCOMPARE( element.field, QStringLiteral( "artist" ) );
            QCOMPARE( element.negate, false );
            QVERIFY( element.match == expression_element::Contains );
        }

        else if( element.text == QStringLiteral("Best of") )
        {
            QCOMPARE( element.field, QStringLiteral( "album" ) );
            QCOMPARE( element.negate, false );
            QVERIFY( element.match == expression_element::Contains );
        }

        else if( element.text == QStringLiteral("2009") )
        {
            QCOMPARE( element.field, QStringLiteral( "year" ) );
            QCOMPARE( element.negate, false );
            QVERIFY( element.match == expression_element::Contains );
        }
    }
}

void TestExpression::testIsAdvancedExpression()
{
    QCOMPARE( ExpressionParser::isAdvancedExpression( QStringLiteral("") ), false );

    QCOMPARE( ExpressionParser::isAdvancedExpression( QStringLiteral("test") ), false );
    QCOMPARE( ExpressionParser::isAdvancedExpression( QStringLiteral("foo bar") ), false );
    QCOMPARE( ExpressionParser::isAdvancedExpression( QStringLiteral("\"foo bar\"") ), true );
    QCOMPARE( ExpressionParser::isAdvancedExpression( QStringLiteral("artist:cure") ), true );
    QCOMPARE( ExpressionParser::isAdvancedExpression( QStringLiteral("year:<1990") ), true );
    QCOMPARE( ExpressionParser::isAdvancedExpression( QStringLiteral("artist:cure year:<1990") ), true );
    QCOMPARE( ExpressionParser::isAdvancedExpression( QStringLiteral("artist:cure AND year:<1990") ), true );
    QCOMPARE( ExpressionParser::isAdvancedExpression( QStringLiteral("artist:cure OR year:<1990") ), true );
    QCOMPARE( ExpressionParser::isAdvancedExpression( QStringLiteral("-artist:madonna") ), true );
    QCOMPARE( ExpressionParser::isAdvancedExpression( QStringLiteral("album:\"Best of\"") ), true );
}
