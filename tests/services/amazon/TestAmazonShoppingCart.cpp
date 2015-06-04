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

#include "TestAmazonShoppingCart.h"

#include "core/support/Components.h"
#include "mocks/MockLogger.h"
#include "services/amazon/AmazonConfig.h"
#include "services/amazon/AmazonShoppingCart.h"
#include "services/amazon/AmazonShoppingCartItem.h"

#include <QTest>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestAmazonShoppingCart )

TestAmazonShoppingCart::TestAmazonShoppingCart() :
    QObject()
{
}

void
TestAmazonShoppingCart::initTestCase()
{
    // AmazonShoppingCart::add() calls Amarok::Components::logger()->longMessage()
    Amarok::Components::setLogger( new Amarok::MockLogger() );
}

void
TestAmazonShoppingCart::cleanupTestCase()
{
    delete Amarok::Components::setLogger( 0 );
}

void
TestAmazonShoppingCart::testAdd()
{
    QCOMPARE( AmazonShoppingCart::instance()->size(), 0 );
    AmazonShoppingCart::instance()->add( "ASIN", "price", "name" );
    QCOMPARE( AmazonShoppingCart::instance()->size(), 1 );

    AmazonShoppingCartItem item = AmazonShoppingCart::instance()->at( 0 );
    QCOMPARE( item.asin(), QString( "ASIN" ) );
    QCOMPARE( item.price(), QString( "price" ) );
    QCOMPARE( item.prettyName(), QString( "name" ) );

    AmazonShoppingCart::instance()->add( "something", "99", "something" );
    QCOMPARE( AmazonShoppingCart::instance()->size(), 2 );
}

void
TestAmazonShoppingCart::testClear()
{
    AmazonShoppingCart::instance()->clear();
    QCOMPARE( AmazonShoppingCart::instance()->size(), 0 );

    AmazonShoppingCart::instance()->add( "ASIN", "price", "name" );
    AmazonShoppingCart::instance()->add( "something", "99", "something" );
    QCOMPARE( AmazonShoppingCart::instance()->size(), 2 );

    AmazonShoppingCart::instance()->clear();
    QCOMPARE( AmazonShoppingCart::instance()->size(), 0 );
}

void
TestAmazonShoppingCart::testStringList()
{
    // for numerical prices (eg available and non free items) the output depends on the locale

    // no crash on empty cart
    AmazonShoppingCart::instance()->stringList();

    AmazonShoppingCart::instance()->add( "ASIN", "price", "name" );
    QString result = AmazonShoppingCart::instance()->stringList().at( 0 );
    QCOMPARE( result, QString( "name (price)" ) );

    AmazonShoppingCart::instance()->add( "ASIN2", "price2", "name2" );
    result = AmazonShoppingCart::instance()->stringList().at( 1 );
    QCOMPARE( result, QString( "name2 (price2)" ) );
}

void
TestAmazonShoppingCart::testPrice()
{
    AmazonShoppingCart::instance()->clear();
    QCOMPARE( AmazonShoppingCart::instance()->price(), QString( "0" ) );

    AmazonShoppingCart::instance()->add( "ASIN", "0", "name" );
    QCOMPARE( AmazonShoppingCart::instance()->price(), QString( "0" ) );

    AmazonShoppingCart::instance()->add( "ASIN", "GRATIS", "name" );
    QCOMPARE( AmazonShoppingCart::instance()->price(), QString( "0" ) );

    AmazonShoppingCart::instance()->add( "ASIN", "99", "name" );
    QCOMPARE( AmazonShoppingCart::instance()->price(), QString( "99" ) );

    AmazonShoppingCart::instance()->add( "ASIN", "1", "name" );
    QCOMPARE( AmazonShoppingCart::instance()->price(), QString( "100" ) );

    AmazonShoppingCart::instance()->add( "ASIN", "1790", "name" );
    QCOMPARE( AmazonShoppingCart::instance()->price(), QString( "1890" ) );
}

void
TestAmazonShoppingCart::testRemove()
{
    AmazonShoppingCart::instance()->clear();
    QCOMPARE( AmazonShoppingCart::instance()->size(), 0 );

    // removing some items that do not exist -> no crash
    AmazonShoppingCart::instance()->remove( 0 );
    AmazonShoppingCart::instance()->remove( 1 );
    AmazonShoppingCart::instance()->remove( 42 );
    AmazonShoppingCart::instance()->remove( -3 );

    AmazonShoppingCart::instance()->add( "ASIN", "price", "name" );
    QCOMPARE( AmazonShoppingCart::instance()->size(), 1 );
    AmazonShoppingCart::instance()->remove( 0 );
    QCOMPARE( AmazonShoppingCart::instance()->size(), 0 );

    AmazonShoppingCart::instance()->add( "ASIN", "price", "name" );
    AmazonShoppingCart::instance()->add( "ASIN", "price", "name" );
    AmazonShoppingCart::instance()->add( "ASIN", "price", "name" );
    QCOMPARE( AmazonShoppingCart::instance()->size(), 3 );
    AmazonShoppingCart::instance()->remove( 1 );
    QCOMPARE( AmazonShoppingCart::instance()->size(), 2 );
    AmazonShoppingCart::instance()->remove( 1 );
    QCOMPARE( AmazonShoppingCart::instance()->size(), 1 );
}

void
TestAmazonShoppingCart::testCheckoutUrl()
{
    AmazonShoppingCart::instance()->clear();
    QString country = AmazonConfig::instance()->country(); // to restore the config later
    AmazonConfig::instance()->setCountry( "de" ); // so we have a fixed, well known result string

    QUrl result = AmazonShoppingCart::instance()->checkoutUrl();
    QCOMPARE( result, QUrl() );

    result = AmazonShoppingCart::instance()->checkoutUrl( "B003MJQB9A" );
    QCOMPARE( result, QUrl( "http://www.mp3-music-store.de/index.php?apikey=27274503cb405cb1929f353fc507f09c&redirect=true&method=CreateCart&Location=de&Player=amarok&ASINs[]=B003MJQB9A" ) );

    AmazonShoppingCart::instance()->add( "B003MJQB9A", "price", "name" );
    result = AmazonShoppingCart::instance()->checkoutUrl();
    QCOMPARE( result, QUrl( "http://www.mp3-music-store.de/index.php?apikey=27274503cb405cb1929f353fc507f09c&redirect=true&method=CreateCart&Location=de&Player=amarok&ASINs[]=B003MJQB9A" ) );

    AmazonShoppingCart::instance()->add( "B007XBRH3C", "price", "name" );
    result = AmazonShoppingCart::instance()->checkoutUrl();
    QCOMPARE( result, QUrl( "http://www.mp3-music-store.de/index.php?apikey=27274503cb405cb1929f353fc507f09c&redirect=true&method=CreateCart&Location=de&Player=amarok&ASINs[]=B003MJQB9A&ASINs[]=B007XBRH3C" ) );

    result = AmazonShoppingCart::instance()->checkoutUrl( "B003MJQB9A" );
    QCOMPARE( result, QUrl( "http://www.mp3-music-store.de/index.php?apikey=27274503cb405cb1929f353fc507f09c&redirect=true&method=CreateCart&Location=de&Player=amarok&ASINs[]=B003MJQB9A" ) );

    AmazonConfig::instance()->setCountry( country );
}
