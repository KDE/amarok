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

#include "TestAmazonTrack.h"

#include "services/amazon/AmazonMeta.h"

#include <QTest>

#include <qtest_kde.h>

QTEST_KDEMAIN( TestAmazonTrack, GUI )

TestAmazonTrack::TestAmazonTrack() :
    QObject()
{
}

void
TestAmazonTrack::testConstructor()
{
    QStringList list, list2;
    list << "id" << "name" << "trackNumber" << "length" << "playableUrl" << "albumId" << "artistId" << "price" << "ASIN";
    list2 << "23" << "name" << "5" << "300" << "http://www.amazon.de/gp/dmusic/get_sample_url.html?ASIN=B007NV28OK" << "42" << "12" << "99" << "B007NV28OK";
    Meta::AmazonTrack track( list );
    Meta::AmazonTrack track2( list2 );

    QCOMPARE( track.id(), 0 );
    QCOMPARE( track.name(), QString( "name" ) );
    QCOMPARE( track.trackNumber(), 0 );
    QCOMPARE( track.length(), (qint64)0 );
    QCOMPARE( track.playableUrl(), QUrl( "playableUrl" ) );
    QCOMPARE( track.albumId(), 0 );
    QCOMPARE( track.artistId(), 0 );
    QCOMPARE( track.price(), QString( "price" ) );
    QCOMPARE( track.asin(), QString( "ASIN" ) );

    QCOMPARE( track2.id(), 23 );
    QCOMPARE( track2.trackNumber(), 5 );
    QCOMPARE( track2.length(), (qint64)300 );
    QCOMPARE( track2.playableUrl(), QUrl("http://www.amazon.de/gp/dmusic/get_sample_url.html?ASIN=B007NV28OK") );
    QCOMPARE( track2.albumId(), 42 );
    QCOMPARE( track2.artistId(), 12 );
    QCOMPARE( track2.price(), QString( "99" ) );
    QCOMPARE( track2.asin(), QString( "B007NV28OK" ) );
}

void
TestAmazonTrack::testEmblem()
{
    QStringList list;
    list << "id" << "name" << "trackNumber" << "length" << "playableUrl" << "albumId" << "artistId" << "price" << "ASIN";
    Meta::AmazonTrack track( list );

    QSKIP("Emblem is only available once Amarok is installed.", SkipSingle);
    QVERIFY( !track.emblem().isNull() );
}
