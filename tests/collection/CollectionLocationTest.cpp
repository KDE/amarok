/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "CollectionLocationTest.h"

#include "collection/CollectionLocation.h"
#include "Debug.h"
#include "meta/MetaConstants.h"
#include "../tests/mocks/MetaMock.h"

#include <QMutex>
#include <QVariantMap>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( CollectionLocationTest )

//required for Debug.h
QMutex Debug::mutex;

CollectionLocationTest::CollectionLocationTest()
{
}

class TestRemoveCL : public CollectionLocation
{
public:
    bool remove( const Meta::TrackPtr &track )
    {
        count++;
        return true;
    }

    int count;
};

void CollectionLocationTest::testSuccessfulCopy()
{
    TestRemoveCL *cl = new TestRemoveCL();
    cl->setProperty( "removeSources", true );
    cl->count = 0;
    QVariantMap map;
    map.insert( Meta::Field::URL,  KUrl( "file:///IDoNotExist.mp3" ) );
    Meta::TrackPtr file1( new MetaMock( map ) );
    cl->transferSuccessful( file1 );

    QVERIFY2( cl->metaObject()->invokeMethod( cl, "slotFinishCopy", Qt::DirectConnection ), "Calling slot failed" );
    QCOMPARE( cl->count, 1 );
}

void CollectionLocationTest::testFailedCopy()
{
    TestRemoveCL *cl = new TestRemoveCL();
    cl->setProperty( "removeSources", true );
    cl->count = 0;
    QVariantMap map;
    map.insert( Meta::Field::URL,  KUrl( "file:///IDoNotExist.mp3" ) );
    Meta::TrackPtr file1( new MetaMock( map ) );
    cl->transferError( file1, "Test of CollectionLocation" );

    cl->metaObject()->invokeMethod( cl, "slotFinishCopy", Qt::DirectConnection );
    QCOMPARE( cl->count, 0 );
}

void CollectionLocationTest::testCopyMultipleTracks()
{
    TestRemoveCL *cl = new TestRemoveCL();
    cl->setProperty( "removeSources", true );
    cl->count = 0;
    QVariantMap map;
    map.insert( Meta::Field::URL,  KUrl( "file:///IDoNotExist.mp3" ) );
    Meta::TrackPtr file1( new MetaMock( map ) );
    map.insert( Meta::Field::URL, KUrl( "file:///IDoNotExistAsWell.mp3" ) );
    Meta::TrackPtr file2( new MetaMock( map )  );
    map.insert( Meta::Field::URL, KUrl( "file:///IDoNotExistAsWell.mp3" ) );
    Meta::TrackPtr file3( new MetaMock( map ) );
    cl->transferError( file1, "Test of CollectionLocation" );
    cl->transferSuccessful( file2 );
    cl->transferSuccessful( file3 );

    cl->metaObject()->invokeMethod( cl, "slotFinishCopy", Qt::DirectConnection );
    QCOMPARE( cl->count, 2 );
}

void CollectionLocationTest::testFailedCopyWithIncorrectUsageOfCopySuccesful()
{
    TestRemoveCL *cl = new TestRemoveCL();
    cl->setProperty( "removeSources", true );
    cl->count = 0;
    QVariantMap map;
    map.insert( Meta::Field::URL,  KUrl( "file:///IDoNotExist.mp3" ) );
    Meta::TrackPtr file1( new MetaMock( map ) );
    cl->transferError( file1, "Test of CollectionLocation" );
    cl->transferSuccessful( file1 );

    cl->metaObject()->invokeMethod( cl, "slotFinishCopy", Qt::DirectConnection );
    QVERIFY2( cl->count == 0, "Expected no call to remove");

    cl = new TestRemoveCL();
    cl->setProperty( "removeSources", true );
    cl->count = 0;
    file1 = Meta::TrackPtr( new MetaMock( map ) );
    cl->transferSuccessful( file1 );
    cl->transferError( file1, "Test of CollectionLocation" );

    cl->metaObject()->invokeMethod( cl, "slotFinishCopy", Qt::DirectConnection );
    QVERIFY2( cl->count == 0, "Expected no call to remove after reversed method call");
}

#include "CollectionLocationTest.moc"

