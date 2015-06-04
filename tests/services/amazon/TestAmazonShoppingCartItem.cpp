/****************************************************************************************
 * Copyright (c) 2012 Sven Krohlas <sven@asbest-online.de>                              *
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

#include "TestAmazonShoppingCartItem.h"

#include "services/amazon/AmazonShoppingCartItem.h"

#include <QTest>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestAmazonShoppingCartItem )

TestAmazonShoppingCartItem::TestAmazonShoppingCartItem() :
    QObject()
{
}

void
TestAmazonShoppingCartItem::testAsin()
{
    AmazonShoppingCartItem item( "", "price", "prettyName" );
    QCOMPARE( item.asin(), QString( "" ) );

    AmazonShoppingCartItem item2( "B003MJQB9A", "price", "prettyName" );
    QCOMPARE( item2.asin(), QString( "B003MJQB9A" ) );
}

void
TestAmazonShoppingCartItem::testPrettyName()
{
    AmazonShoppingCartItem item( "ASIN", "price", "" );
    QCOMPARE( item.prettyName(), QString( "" ) );

    AmazonShoppingCartItem item2( "ASIN", "price", "The Cure - Disintegration" );
    QCOMPARE( item2.prettyName(), QString( "The Cure - Disintegration" ) );
}

void
TestAmazonShoppingCartItem::testPrice()
{
    AmazonShoppingCartItem item( "ASIN", "", "prettyName" );
    QCOMPARE( item.price(), QString( "" ) );

    AmazonShoppingCartItem item2( "ASIN", "99", "prettyName" );
    QCOMPARE( item2.price(), QString( "99" ) );
}
