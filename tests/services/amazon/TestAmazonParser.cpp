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

#include "config-amarok-test.h"

#include "TestAmazonParser.h"

#include "ServiceBase.h"

#include <QDir>
#include <QFile>
#include <QTest>

#include <qtest_kde.h>

QTEST_KDEMAIN( TestAmazonParser, GUI )

TestAmazonParser::TestAmazonParser()
    :AmazonParser( QDir::tempPath() + QDir::separator() + "searchresponse.xml",
                   new Collections::AmazonCollection( (ServiceBase*)NULL, "amazon", "MP3 Music Store" ), // HACK...
                   new AmazonMetaFactory( "amazon" ) )
{
}

void
TestAmazonParser::testRun()
{
    // AmazonParser deletes the xml after parsing, so we have to make a copy
    QFile testFile( QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + "/data/services/amazon/searchresponse.xml" ) );
    QVERIFY( testFile.copy( QDir::tempPath() + QDir::separator() + "searchresponse.xml" ) );

    // parse searchresponse.xml
    run();

    // test the result
    QMap<QString, int> &artistIDMap = m_collection->artistIDMap();
    QCOMPARE( artistIDMap.size(), 2 );
    QCOMPARE( artistIDMap.key( 1 ), QString( "The Cure" ) );
    QCOMPARE( artistIDMap.key( 2 ), QString( "Jah Cure" ) );

    QMap<QString, int> &albumIDMap = m_collection->albumIDMap();
    QCOMPARE( albumIDMap.size(), 23 );
    QCOMPARE( albumIDMap.key( 1 ), QString( "B001SW8TFC" ) );
    QCOMPARE( albumIDMap.key( 23 ), QString( "B001SVHLE8" ) );

    QMap<QString, int> &trackIDMap = m_collection->trackIDMap();
    QCOMPARE( trackIDMap.size(), 50 );
    QCOMPARE( trackIDMap.key( 1 ), QString( "B001SW3X58" ) );
    QCOMPARE( trackIDMap.key( 50 ), QString( "B001SQBJQE" ) );

    // let's check some tracks
    Meta::AmazonTrack* track;

    track = dynamic_cast<Meta::AmazonTrack*>( m_collection->trackById( 1 ).data() );
    QVERIFY( track );
    QCOMPARE( track->asin(), QString( "B001SW3X58" ) );
    QCOMPARE( track->name(), QString( "Lullaby" ) );
    QCOMPARE( track->price(), QString( "99" ) );
    QCOMPARE( track->artistId(), 1 );
    QCOMPARE( track->albumId(), 1 );

    track = dynamic_cast<Meta::AmazonTrack*>( m_collection->trackById( 2 ).data() );
    QVERIFY( track );
    QCOMPARE( track->asin(), QString( "B001SW8U92" ) );
    QCOMPARE( track->name(), QString( "Friday I'm In Love" ) );
    QCOMPARE( track->price(), QString( "99" ) );
    QCOMPARE( track->artistId(), 1 );
    QCOMPARE( track->albumId(), 1 );

    track = dynamic_cast<Meta::AmazonTrack*>( m_collection->trackById( 50 ).data() );
    QVERIFY( track );
    QCOMPARE( track->asin(), QString( "B001SQBJQE" ) );
    QCOMPARE( track->name(), QString( "From The Edge Of The Deep Green Sea" ) );
    QCOMPARE( track->price(), QString( "99" ) );
    QCOMPARE( track->artistId(), 1 );
    QCOMPARE( track->albumId(), 18 );


    // let's check some albums
    Meta::AmazonAlbum* album;

    album = dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( 1 ).data() );
    QVERIFY( album );
    QCOMPARE( album->asin(), QString( "B001SW8TFC" ) );
    QCOMPARE( album->coverUrl(), QString( "http://ecx.images-amazon.com/images/I/51L5Z-JXB3L._SL500_SS110_.jpg" ) );
    QCOMPARE( album->name(), QString( "Greatest Hits" ) );
    QCOMPARE( album->price(), QString( "989" ) );

    album = dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( 2 ).data() );
    QVERIFY( album );
    QCOMPARE( album->asin(), QString( "B00666RKLE" ) );
    QCOMPARE( album->coverUrl(), QString( "http://ecx.images-amazon.com/images/I/61p6dd%2BShZL._SL500_AA240_.jpg" ) );
    QCOMPARE( album->name(), QString( "Bestival Live 2011" ) );
    QCOMPARE( album->price(), QString( "699" ) );

    album = dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( 3 ).data() );
    QVERIFY( album );
    QCOMPARE( album->asin(), QString( "B003TZV8T0" ) );
    QCOMPARE( album->coverUrl(), QString( "http://ecx.images-amazon.com/images/I/51LN4o1DxYL._SL500_SS110_.jpg" ) );
    QCOMPARE( album->name(), QString( "Staring At The Sea - The Singles" ) );
    QCOMPARE( album->price(), QString( "599" ) );

    album = dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( 4 ).data() );
    QVERIFY( album );
    QCOMPARE( album->asin(), QString( "B0045M8SDK" ) );
    QCOMPARE( album->coverUrl(), QString( "http://ecx.images-amazon.com/images/I/411ncIcC8vL._SL500_SS110_.jpg" ) );
    QCOMPARE( album->name(), QString( "Kiss Me, Kiss Me, Kiss Me" ) );
    QCOMPARE( album->price(), QString( "599" ) );

    album = dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( 5 ).data() );
    QVERIFY( album );
    QCOMPARE( album->asin(), QString( "B001SVN2NM" ) );
    QCOMPARE( album->coverUrl(), QString( "http://ecx.images-amazon.com/images/I/418turaImmL._SL500_SS110_.jpg" ) );
    QCOMPARE( album->name(), QString( "Pornography" ) );
    QCOMPARE( album->price(), QString( "599" ) );

    album = dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( 23 ).data() );
    QVERIFY( album );
    QCOMPARE( album->asin(), QString( "B001SVHLE8" ) );
    QCOMPARE( album->coverUrl(), QString( "" ) );
    QCOMPARE( album->name(), QString( "Three Imaginary Boys" ) );
    QCOMPARE( album->price(), QString( "" ) );

    m_collection->clear();
    QCOMPARE( artistIDMap.size(), 0 );
    QCOMPARE( albumIDMap.size(), 0 );
    QCOMPARE( trackIDMap.size(), 0 );
}
