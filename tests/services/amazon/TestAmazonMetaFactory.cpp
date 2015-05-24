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

#include "TestAmazonMetaFactory.h"

#include "services/amazon/AmazonMeta.h"

#include <QtTest/QTest>

#include <qtest_kde.h>

QTEST_KDEMAIN( TestAmazonMetaFactory, GUI )

TestAmazonMetaFactory::TestAmazonMetaFactory() :
    QObject()
{
}

void
TestAmazonMetaFactory::testCreateAlbum()
{
    QStringList list, list2;
    list << "id" << "name" << "description" << "artistID" << "price" << "coverUrl" << "ASIN";
    list2 << "23" << "name" << "description" << "42" << "price" << "coverUrl" << "ASIN";

    AmazonMetaFactory factory( "Amazontest" );
    Meta::AlbumPtr albumPtr = factory.createAlbum( list );
    Meta::AlbumPtr albumPtr2 = factory.createAlbum( list2 );

    QVERIFY( albumPtr );
    QVERIFY( albumPtr2 );

    Meta::AmazonAlbum* amazonAlbum = dynamic_cast<Meta::AmazonAlbum*>( albumPtr.data() );
    Meta::AmazonAlbum* amazonAlbum2 = dynamic_cast<Meta::AmazonAlbum*>( albumPtr2.data() );

    QVERIFY( amazonAlbum );
    QVERIFY( amazonAlbum2 );

    QCOMPARE( amazonAlbum->id(), 0 );
    QCOMPARE( amazonAlbum->name(), QString( "name" ) );
    QCOMPARE( amazonAlbum->description(), QString( "description" ) );
    QCOMPARE( amazonAlbum->artistId(), 0 );
    QCOMPARE( amazonAlbum->price(), QString( "price" ) );
    QCOMPARE( amazonAlbum->coverUrl(), QString( "coverUrl" ) );
    QCOMPARE( amazonAlbum->asin(), QString( "ASIN" ) );

    QCOMPARE( amazonAlbum2->id(), 23 );
    QCOMPARE( amazonAlbum2->artistId(), 42 );
}

void
TestAmazonMetaFactory::testCreateArtist()
{
    QStringList list, list2;
    list << "id" << "name" << "description";
    list2 << "23" << "name" << "description";

    AmazonMetaFactory factory( "Amazontest" );
    Meta::ArtistPtr artistPtr = factory.createArtist( list );
    Meta::ArtistPtr artistPtr2 = factory.createArtist( list2 );

    QVERIFY( artistPtr );
    QVERIFY( artistPtr2 );

    Meta::AmazonArtist* amazonArtist = dynamic_cast<Meta::AmazonArtist*>( artistPtr.data() );
    Meta::AmazonArtist* amazonArtist2 = dynamic_cast<Meta::AmazonArtist*>( artistPtr2.data() );

    QVERIFY( amazonArtist );
    QVERIFY( amazonArtist2 );

    QCOMPARE( amazonArtist->id(), 0 );
    QCOMPARE( amazonArtist->name(), QString( "name" ) );
    QCOMPARE( amazonArtist->description(), QString( "description" ) );

    QCOMPARE( amazonArtist2->id(), 23 );
}

void
TestAmazonMetaFactory::testCreateTrack()
{
    QStringList list, list2;
    list << "id" << "name" << "trackNumber" << "length" << "playableUrl" << "albumId" << "artistId" << "price" << "ASIN";
    list2 << "23" << "name" << "5" << "300" << "http://www.amazon.de/gp/dmusic/get_sample_url.html?ASIN=B007NV28OK" << "42" << "12" << "99" << "B007NV28OK";

    AmazonMetaFactory factory( "Amazontest" );
    Meta::TrackPtr trackPtr = factory.createTrack( list );
    Meta::TrackPtr trackPtr2 = factory.createTrack( list2 );

    QVERIFY( trackPtr );
    QVERIFY( trackPtr2 );

    Meta::AmazonTrack* amazonTrack = dynamic_cast<Meta::AmazonTrack*>( trackPtr.data() );
    Meta::AmazonTrack* amazonTrack2 = dynamic_cast<Meta::AmazonTrack*>( trackPtr2.data() );

    QVERIFY( amazonTrack );
    QVERIFY( amazonTrack2 );

    QCOMPARE( amazonTrack->id(), 0 );
    QCOMPARE( amazonTrack->name(), QString( "name" ) );
    QCOMPARE( amazonTrack->trackNumber(), 0 );
    QCOMPARE( amazonTrack->length(), (qint64)0 );
    QCOMPARE( amazonTrack->playableUrl(), QUrl( "playableUrl" ) );
    QCOMPARE( amazonTrack->albumId(), 0 );
    QCOMPARE( amazonTrack->artistId(), 0 );
    QCOMPARE( amazonTrack->price(), QString( "price" ) );
    QCOMPARE( amazonTrack->asin(), QString( "ASIN" ) );

    QCOMPARE( amazonTrack2->id(), 23 );
    QCOMPARE( amazonTrack2->trackNumber(), 5 );
    QCOMPARE( amazonTrack2->length(), (qint64)300 );
    QCOMPARE( amazonTrack2->playableUrl(), QUrl("http://www.amazon.de/gp/dmusic/get_sample_url.html?ASIN=B007NV28OK") );
    QCOMPARE( amazonTrack2->albumId(), 42 );
    QCOMPARE( amazonTrack2->artistId(), 12 );
    QCOMPARE( amazonTrack2->price(), QString( "99" ) );
    QCOMPARE( amazonTrack2->asin(), QString( "B007NV28OK" ) );
}
