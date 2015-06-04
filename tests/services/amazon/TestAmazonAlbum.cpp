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

#include "TestAmazonAlbum.h"

#include "services/amazon/AmazonMeta.h"

#include <QTest>

#include <qtest_kde.h>

QTEST_KDEMAIN( TestAmazonAlbum, GUI )

TestAmazonAlbum::TestAmazonAlbum() :
    QObject()
{
}

void
TestAmazonAlbum::testSetAndGetCoverUrl()
{
    QStringList list, list2;
    list << "id" << "name" << "description" << "artistID" << "price" << "coverUrl" << "ASIN";
    list2 << "23" << "name" << "description" << "42" << "price" << "coverUrl" << "ASIN";
    Meta::AmazonAlbum album( list );
    Meta::AmazonAlbum album2( list2 );

    QCOMPARE( album.id(), 0 );
    QCOMPARE( album.name(), QString( "name" ) );
    QCOMPARE( album.description(), QString( "description" ) );
    QCOMPARE( album.artistId(), 0 );
    QCOMPARE( album.price(), QString( "price" ) );
    QCOMPARE( album.coverUrl(), QString( "coverUrl" ) );
    QCOMPARE( album.asin(), QString( "ASIN" ) );

    QCOMPARE( album2.id(), 23 );
    QCOMPARE( album2.artistId(), 42 );

    album.setCoverUrl( "http://ecx.images-amazon.com/images/I/61aUtzxMIWL._SL500_SS110_.jpg" );
    QCOMPARE( album.coverUrl(), QString( "http://ecx.images-amazon.com/images/I/61aUtzxMIWL._SL500_SS110_.jpg" ) );
}
