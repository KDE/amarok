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

#include "core/support/Components.h"
#include "core/collections/CollectionLocation.h"
#include "core/support/Debug.h"
#include "core/meta/support/MetaConstants.h"
#include "../tests/mocks/MetaMock.h"
#include "MockCollectionLocationDelegate.h"

#include <QMutex>
#include <QSignalSpy>
#include <QVariantMap>

#include <gmock/gmock.h>

QTEST_GUILESS_MAIN( CollectionLocationTest )

using ::testing::Return;
using ::testing::AnyNumber;
using ::testing::_;

CollectionLocationTest::CollectionLocationTest()
{
    qRegisterMetaType<Meta::TrackList>();
    qRegisterMetaType<Meta::AlbumList>();
    qRegisterMetaType<Meta::ArtistList>();
}

namespace Collections {

class TestRemoveCL : public CollectionLocation
{
public:

    void removeUrlsFromCollection( const Meta::TrackList &tracks ) override
    {
        count += tracks.count();
        slotRemoveOperationFinished();
    }

    MOCK_METHOD( bool, isWritable, (), (const, override) );
    MOCK_METHOD( bool, isOrganizable, (), (const, override) );

    int count;
};

} //namespace Collections

void CollectionLocationTest::testSuccessfulCopy()
{
    Collections::MockCollectionLocationDelegate *cld = new Collections::MockCollectionLocationDelegate();
    EXPECT_CALL( *cld, reallyDelete( _, _) ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    Amarok::Components::setCollectionLocationDelegate( cld );

    Collections::TestRemoveCL *cl = new Collections::TestRemoveCL();
    QSignalSpy spy( cl, &Collections::TestRemoveCL::destroyed );
    EXPECT_CALL( *cl, isWritable() ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    cl->setProperty( "removeSources", true );
    cl->count = 0;
    QVariantMap map;
    map.insert( Meta::Field::URL,  QUrl(QStringLiteral("file:///IDoNotExist.mp3")) );
    Meta::TrackPtr file1( new MetaMock( map ) );
    cl->transferSuccessful( file1 );

    QVERIFY2( cl->metaObject()->invokeMethod( cl, "slotFinishCopy", Qt::DirectConnection ), "Calling slot failed" );
    QCOMPARE( cl->count, 1 );
    QVERIFY( spy.wait( 5000 ) );
    delete Amarok::Components::setCollectionLocationDelegate( nullptr );
}

void CollectionLocationTest::testFailedCopy()
{
    Collections::MockCollectionLocationDelegate *cld = new Collections::MockCollectionLocationDelegate();
    EXPECT_CALL( *cld, reallyDelete( _, _) ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( *cld, errorDeleting( _, _ ) ).Times( AnyNumber() );
    Amarok::Components::setCollectionLocationDelegate( cld );

    Collections::TestRemoveCL *cl = new Collections::TestRemoveCL();
    EXPECT_CALL( *cl, isWritable() ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    QSignalSpy spy( cl, &Collections::TestRemoveCL::destroyed );
    cl->setProperty( "removeSources", true );
    cl->count = 0;
    QVariantMap map;
    map.insert( Meta::Field::URL,  QUrl(QStringLiteral("file:///IDoNotExist.mp3")) );
    Meta::TrackPtr file1( new MetaMock( map ) );
    cl->transferError( file1, QStringLiteral("Test of CollectionLocation") );

    QVERIFY2( cl->metaObject()->invokeMethod( cl, "slotFinishCopy", Qt::DirectConnection ), "Calling slot failed"  );
    QCOMPARE( cl->count, 0 );
    QVERIFY( spy.wait( 5000 ) );
    delete Amarok::Components::setCollectionLocationDelegate( nullptr );
}

void CollectionLocationTest::testCopyMultipleTracks()
{
    Collections::MockCollectionLocationDelegate *cld = new Collections::MockCollectionLocationDelegate();
    EXPECT_CALL( *cld, reallyDelete( _, _) ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( *cld, errorDeleting( _, _ ) ).Times( AnyNumber() );
    Amarok::Components::setCollectionLocationDelegate( cld );

    Collections::TestRemoveCL *cl = new Collections::TestRemoveCL();
    QSignalSpy spy( cl, &Collections::TestRemoveCL::destroyed );
    EXPECT_CALL( *cl, isWritable() ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );

    cl->setProperty( "removeSources", true );
    cl->count = 0;
    QVariantMap map;
    map.insert( Meta::Field::URL,  QUrl(QStringLiteral("file:///IDoNotExist.mp3")) );
    Meta::TrackPtr file1( new MetaMock( map ) );
    map.insert( Meta::Field::URL, QUrl(QStringLiteral("file:///IDoNotExistAsWell.mp3")) );
    Meta::TrackPtr file2( new MetaMock( map )  );
    map.insert( Meta::Field::URL, QUrl(QStringLiteral("file:///IDoNotExistAsWell.mp3")) );
    Meta::TrackPtr file3( new MetaMock( map ) );
    cl->transferError( file1, QStringLiteral("Test of CollectionLocation") );
    cl->transferSuccessful( file2 );
    cl->transferSuccessful( file3 );

    cl->metaObject()->invokeMethod( cl, "slotFinishCopy", Qt::DirectConnection );
    QCOMPARE( cl->count, 2 );
    QVERIFY( spy.wait( 5000 ) );
    delete Amarok::Components::setCollectionLocationDelegate( nullptr );
}

void CollectionLocationTest::testFailedCopyWithIncorrectUsageOfCopySuccesful()
{
    Collections::MockCollectionLocationDelegate *cld = new Collections::MockCollectionLocationDelegate();
    EXPECT_CALL( *cld, reallyDelete( _, _) ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( *cld, errorDeleting( _, _ ) ).Times( AnyNumber() );
    Amarok::Components::setCollectionLocationDelegate( cld );

    Collections::TestRemoveCL *cl = new Collections::TestRemoveCL();
    EXPECT_CALL( *cl, isWritable() ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    QSignalSpy spy1( cl, &Collections::TestRemoveCL::destroyed );
    cl->setProperty( "removeSources", true );
    cl->count = 0;
    QVariantMap map;
    map.insert( Meta::Field::URL, QUrl(QStringLiteral("file:///IDoNotExist.mp3")) );
    Meta::TrackPtr file1( new MetaMock( map ) );
    cl->transferError( file1, QStringLiteral("Test of CollectionLocation") );
    cl->transferSuccessful( file1 );

    cl->metaObject()->invokeMethod( cl, "slotFinishCopy", Qt::DirectConnection );
    QVERIFY2( cl->count == 0, "Expected no call to remove");

    QVERIFY( spy1.wait( 5000 ) );
    delete Amarok::Components::setCollectionLocationDelegate( nullptr );

    cld = new Collections::MockCollectionLocationDelegate();
    EXPECT_CALL( *cld, reallyDelete( _, _) ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( *cld, errorDeleting( _, _ ) ).Times( AnyNumber() );
    Amarok::Components::setCollectionLocationDelegate( cld );

    cl = new Collections::TestRemoveCL();
    EXPECT_CALL( *cl, isWritable() ).Times( AnyNumber() ).WillRepeatedly( Return( true ) );
    QSignalSpy spy2( cl, &Collections::TestRemoveCL::destroyed );
    cl->setProperty( "removeSources", true );
    cl->count = 0;
    file1 = Meta::TrackPtr( new MetaMock( map ) );
    cl->transferSuccessful( file1 );
    cl->transferError( file1, QStringLiteral("Test of CollectionLocation") );

    cl->metaObject()->invokeMethod( cl, "slotFinishCopy", Qt::DirectConnection );
    QVERIFY2( cl->count == 0, "Expected no call to remove after reversed method call");
    QVERIFY( spy2.wait( 5000 ) );
    delete Amarok::Components::setCollectionLocationDelegate( nullptr );
}


