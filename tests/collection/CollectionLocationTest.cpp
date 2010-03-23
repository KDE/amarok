/****************************************************************************************
 * Copyright (c) 2009,2010 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#include "Components.h"
#include "collection/CollectionLocation.h"
#include "Debug.h"
#include "core/meta/support/MetaConstants.h"
#include "../tests/mocks/MetaMock.h"
#include "MockCollectionLocationDelegate.h"

#include <QMutex>
#include <QVariantMap>

#include <KCmdLineArgs>
#include <KGlobal>

#include <qtest_kde.h>

#include <gmock/gmock.h>

QTEST_KDEMAIN_CORE( CollectionLocationTest )

using ::testing::Return;
using ::testing::AnyNumber;
using ::testing::_;

CollectionLocationTest::CollectionLocationTest()
{
    KCmdLineArgs::init( KGlobal::activeComponent().aboutData() );
    ::testing::InitGoogleMock( &KCmdLineArgs::qtArgc(), KCmdLineArgs::qtArgv() );
    qRegisterMetaType<Meta::TrackList>();
    qRegisterMetaType<Meta::AlbumList>();
    qRegisterMetaType<Meta::ArtistList>();
}

class TestRemoveCL : public CollectionLocation
{
public:

    void removeUrlsFromCollection( const Meta::TrackList &tracks )
    {
        count += tracks.count();
        slotRemoveOperationFinished();
    }

    MOCK_CONST_METHOD0( isWritable, bool() );
    MOCK_CONST_METHOD0( isOrganizable, bool() );

    int count;
};

void CollectionLocationTest::testSuccessfulCopy()
{
    MockCollectionLocationDelegate *cld = new MockCollectionLocationDelegate();
    EXPECT_CALL( *cld, reallyDelete( _, _) ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    Amarok::Components::setCollectionLocationDelegate( cld );

    TestRemoveCL *cl = new TestRemoveCL();
    EXPECT_CALL( *cl, isWritable() ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    cl->setProperty( "removeSources", true );
    cl->count = 0;
    QVariantMap map;
    map.insert( Meta::Field::URL,  KUrl( "file:///IDoNotExist.mp3" ) );
    Meta::TrackPtr file1( new MetaMock( map ) );
    cl->transferSuccessful( file1 );

    QVERIFY2( cl->metaObject()->invokeMethod( cl, "slotFinishCopy", Qt::DirectConnection ), "Calling slot failed" );
    QCOMPARE( cl->count, 1 );
    QVERIFY( QTest::kWaitForSignal( cl, SIGNAL( destroyed() ), 500 ) );
    delete Amarok::Components::setCollectionLocationDelegate( 0 );
}

void CollectionLocationTest::testFailedCopy()
{
    MockCollectionLocationDelegate *cld = new MockCollectionLocationDelegate();
    EXPECT_CALL( *cld, reallyDelete( _, _) ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( *cld, errorDeleting( _, _ ) ).Times( AnyNumber() );
    Amarok::Components::setCollectionLocationDelegate( cld );

    TestRemoveCL *cl = new TestRemoveCL();
    EXPECT_CALL( *cl, isWritable() ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    cl->setProperty( "removeSources", true );
    cl->count = 0;
    QVariantMap map;
    map.insert( Meta::Field::URL,  KUrl( "file:///IDoNotExist.mp3" ) );
    Meta::TrackPtr file1( new MetaMock( map ) );
    cl->transferError( file1, "Test of CollectionLocation" );

    cl->metaObject()->invokeMethod( cl, "slotFinishCopy", Qt::DirectConnection );
    QCOMPARE( cl->count, 0 );
    QVERIFY( QTest::kWaitForSignal( cl, SIGNAL( destroyed() ), 500 ) );
    delete Amarok::Components::setCollectionLocationDelegate( 0 );
}

void CollectionLocationTest::testCopyMultipleTracks()
{
    MockCollectionLocationDelegate *cld = new MockCollectionLocationDelegate();
    EXPECT_CALL( *cld, reallyDelete( _, _) ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( *cld, errorDeleting( _, _ ) ).Times( AnyNumber() );
    Amarok::Components::setCollectionLocationDelegate( cld );

    TestRemoveCL *cl = new TestRemoveCL();
    EXPECT_CALL( *cl, isWritable() ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );

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
    QVERIFY( QTest::kWaitForSignal( cl, SIGNAL( destroyed() ), 500 ) );
    delete Amarok::Components::setCollectionLocationDelegate( 0 );
}

void CollectionLocationTest::testFailedCopyWithIncorrectUsageOfCopySuccesful()
{
    MockCollectionLocationDelegate *cld = new MockCollectionLocationDelegate();
    EXPECT_CALL( *cld, reallyDelete( _, _) ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( *cld, errorDeleting( _, _ ) ).Times( AnyNumber() );
    Amarok::Components::setCollectionLocationDelegate( cld );

    TestRemoveCL *cl = new TestRemoveCL();
    EXPECT_CALL( *cl, isWritable() ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    cl->setProperty( "removeSources", true );
    cl->count = 0;
    QVariantMap map;
    map.insert( Meta::Field::URL,  KUrl( "file:///IDoNotExist.mp3" ) );
    Meta::TrackPtr file1( new MetaMock( map ) );
    cl->transferError( file1, "Test of CollectionLocation" );
    cl->transferSuccessful( file1 );

    cl->metaObject()->invokeMethod( cl, "slotFinishCopy", Qt::DirectConnection );
    QVERIFY2( cl->count == 0, "Expected no call to remove");

    QVERIFY( QTest::kWaitForSignal( cl, SIGNAL( destroyed() ), 500 ) );
    delete Amarok::Components::setCollectionLocationDelegate( 0 );

    cld = new MockCollectionLocationDelegate();
    EXPECT_CALL( *cld, reallyDelete( _, _) ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( *cld, errorDeleting( _, _ ) ).Times( AnyNumber() );
    Amarok::Components::setCollectionLocationDelegate( cld );

    cl = new TestRemoveCL();
    EXPECT_CALL( *cl, isWritable() ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    cl->setProperty( "removeSources", true );
    cl->count = 0;
    file1 = Meta::TrackPtr( new MetaMock( map ) );
    cl->transferSuccessful( file1 );
    cl->transferError( file1, "Test of CollectionLocation" );

    cl->metaObject()->invokeMethod( cl, "slotFinishCopy", Qt::DirectConnection );
    QVERIFY2( cl->count == 0, "Expected no call to remove after reversed method call");
    QVERIFY( QTest::kWaitForSignal( cl, SIGNAL( destroyed() ), 500 ) );
    delete Amarok::Components::setCollectionLocationDelegate( 0 );
}

#include "CollectionLocationTest.moc"

