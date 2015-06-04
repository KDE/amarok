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

#include "TestAmazonItem.h"

#include "services/amazon/AmazonMeta.h"

#include <QTest>

#include <qtest_kde.h>

QTEST_KDEMAIN( TestAmazonItem, GUI )

TestAmazonItem::TestAmazonItem() :
    QObject()
{
}

void
TestAmazonItem::testSetAndGetAsin()
{
    Meta::AmazonItem item;

    item.setAsin( "" );
    QCOMPARE( item.asin(), QString( "" ) );

    item.setAsin( "B003MJQB9A" );
    QCOMPARE( item.asin(), QString( "B003MJQB9A" ) );
}

void
TestAmazonItem::testSetAndGetPrice()
{
    Meta::AmazonItem item;

    item.setPrice( "" );
    QCOMPARE( item.price(), QString( "" ) );

    item.setPrice( "GRATIS" );
    QCOMPARE( item.price(), QString( "GRATIS" ) );

    item.setPrice( "99" );
    QCOMPARE( item.price(), QString( "99" ) );
}
