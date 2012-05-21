/****************************************************************************************
 * Copyright (c) 2012 Sven Krohlas <sven@getamarok.com>                                 *
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

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtTest/QTest>

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
    QMap<QString, int> *artistIDMap = m_collection->artistIDMap();
    QVERIFY( artistIDMap );
    QCOMPARE( artistIDMap->size(), 2 );
    QCOMPARE( artistIDMap->key( 1 ), QString( "The Cure" ) );
    QCOMPARE( artistIDMap->key( 2 ), QString( "Jah Cure" ) );

    QMap<QString, int> *albumIDMap = m_collection->albumIDMap();
    QVERIFY( albumIDMap );
    QCOMPARE( albumIDMap->size(), 23 );
    QCOMPARE( albumIDMap->key( 1 ), QString( "B001SW8TFC" ) );
    QCOMPARE( albumIDMap->key( 23 ), QString( "B001SVHLE8" ) );

    QMap<QString, int> *trackIDMap = m_collection->trackIDMap();
    QVERIFY( trackIDMap );
    QCOMPARE( trackIDMap->size(), 50 );
    QCOMPARE( trackIDMap->key( 1 ), QString( "B001SW3X58" ) );
    QCOMPARE( trackIDMap->key( 50 ), QString( "B001SQBJQE" ) );
}
