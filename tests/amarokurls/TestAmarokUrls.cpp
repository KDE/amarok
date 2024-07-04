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

#include "TestAmarokUrls.h"

#include "core/support/Components.h"
#include "EngineController.h"

#include "config-amarok-test.h"

#include "amarokurls/AmarokUrl.h"
#include "amarokurls/AmarokUrlHandler.h"

#include <KLocalizedString>

#include <QTest>

QTEST_GUILESS_MAIN( TestAmarokUrls )

TestAmarokUrls::TestAmarokUrls()
  : QObject()
{
    KLocalizedString::setApplicationDomain("amarok-test");
    //apparently the engine controller is needed somewhere, or we will get a crash...
    EngineController *controller = new EngineController();
    Amarok::Components::setEngineController( controller );
}

void
TestAmarokUrls::testConstructUrl()
{

    AmarokUrl url;

    url.setCommand( QStringLiteral("navigate") );
    url.setPath( QStringLiteral("collections") );
    url.setArg( QStringLiteral("filter"), QStringLiteral("artist:\"Code Monkeys\"") );
    url.setArg( QStringLiteral("levels"), QStringLiteral("artist-album") );

    QCOMPARE( url.command(), QStringLiteral( "navigate" ) );
    QCOMPARE( url.path(), QStringLiteral( "collections" ) );
    QCOMPARE( url.args().size(), 2 );
    QVERIFY( url.args().keys().contains( QStringLiteral("filter") ) );
    QVERIFY( url.args().keys().contains( QStringLiteral("levels") ) );
    QCOMPARE( url.args().value( QStringLiteral("filter") ), QStringLiteral( "artist:\"Code Monkeys\"" ) );
    QCOMPARE( url.args().value( QStringLiteral("levels") ), QStringLiteral( "artist-album") );

    QCOMPARE( url.prettyCommand(), The::amarokUrlHandler()->prettyCommand( QStringLiteral("navigate") ) );


}


void
TestAmarokUrls::testUrlFromString()
{

    AmarokUrl url( QStringLiteral("amarok://navigate/collections?filter=artist:\"Code Monkeys\"&levels=artist-album") );

    QCOMPARE( url.command(), QStringLiteral( "navigate" ) );
    QCOMPARE( url.path(), QStringLiteral( "collections" ) );
    QCOMPARE( url.args().size(), 2 );
    QVERIFY( url.args().keys().contains( QStringLiteral("filter") ) );
    QVERIFY( url.args().keys().contains( QStringLiteral("levels") ) );
    QCOMPARE( url.args().value( QStringLiteral("filter") ), QStringLiteral( "artist:\"Code Monkeys\"" ) );
    QCOMPARE( url.args().value( QStringLiteral("levels") ), QStringLiteral( "artist-album") );

    QCOMPARE( url.prettyCommand(), The::amarokUrlHandler()->prettyCommand( QStringLiteral("navigate") ) );

}

void TestAmarokUrls::testEncoding()
{
    QString urlString( QStringLiteral("amarok://navigate/collections?filter=artist:\"Code Monkeys\"&levels=artist-album") );
    AmarokUrl url( urlString );

    QUrl qUrl( urlString );

    QCOMPARE( url.url(), QString( qUrl.toString( QUrl::FullyEncoded ) ) );


    //check that we do  not "double encode" anything
    AmarokUrl url2( QStringLiteral("amarok://navigate/collections?filter=artist:%22Code%20Monkeys%22&levels=artist-album") );
    QCOMPARE( url2.url(), QString( qUrl.toString( QUrl::FullyEncoded ) ) );
}


