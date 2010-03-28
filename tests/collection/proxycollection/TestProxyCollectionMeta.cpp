/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#include "TestProxyCollectionMeta.h"

#include "ProxyCollection.h"
#include "ProxyCollectionMeta.h"
#include "core/support/Debug.h"
#include "core/capabilities/Capability.h"
#include "core/meta/Meta.h"
#include "core/capabilities/EditCapability.h"
#include "core/capabilities/OrganiseCapability.h"

#include "mocks/MetaMock.h"
#include "mocks/MockTrack.h"

#include <QMap>
#include <QSignalSpy>

#include <KCmdLineArgs>
#include <KGlobal>

#include <qtest_kde.h>

#include <gmock/gmock.h>

QTEST_KDEMAIN_CORE( TestProxyCollectionMeta )

TestProxyCollectionMeta::TestProxyCollectionMeta()
{
    KCmdLineArgs::init( KGlobal::activeComponent().aboutData() );
    ::testing::InitGoogleMock( &KCmdLineArgs::qtArgc(), KCmdLineArgs::qtArgv() );
}

class MyTrackMock : public MetaMock
{
public:
    MyTrackMock() : MetaMock( QVariantMap() ) {}

    bool hasCapabilityInterface( Capabilities::Capability::Type type ) const
    {
        bool result = results.value( type );
        results.remove( type );
        return result;
    }

    Capabilities::Capability* createCapabilityInterface(Capabilities::Capability::Type type)
    {
        Capabilities::Capability* cap = capabilities.value( type );
        capabilities.remove( type );
        return cap;
    }


    mutable QMap<Capabilities::Capability::Type, bool> results;
    mutable QMap<Capabilities::Capability::Type, Capabilities::Capability*> capabilities;
};

class MyAlbumMock : public MockAlbum
{
public:
    MyAlbumMock() : MockAlbum( "testAlbum" ) {}

    bool hasCapabilityInterface( Capabilities::Capability::Type type ) const
    {
        bool result = results.value( type );
        results.remove( type );
        return result;
    }

    Capabilities::Capability* createCapabilityInterface(Capabilities::Capability::Type type)
    {
        Capabilities::Capability* cap = capabilities.value( type );
        capabilities.remove( type );
        return cap;
    }


    mutable QMap<Capabilities::Capability::Type, bool> results;
    mutable QMap<Capabilities::Capability::Type, Capabilities::Capability*> capabilities;
};

class MyArtistMock : public MockArtist
{
public:
    MyArtistMock() : MockArtist( "testArtist" ) {}

    bool hasCapabilityInterface( Capabilities::Capability::Type type ) const
    {
        bool result = results.value( type );
        results.remove( type );
        return result;
    }

    Capabilities::Capability* createCapabilityInterface(Capabilities::Capability::Type type)
    {
        Capabilities::Capability* cap = capabilities.value( type );
        capabilities.remove( type );
        return cap;
    }


    mutable QMap<Capabilities::Capability::Type, bool> results;
    mutable QMap<Capabilities::Capability::Type, Capabilities::Capability*> capabilities;
};

class MyGenreMock : public MockGenre
{
public:
    MyGenreMock() : MockGenre( "testGenre" ) {}

    bool hasCapabilityInterface( Capabilities::Capability::Type type ) const
    {
        bool result = results.value( type );
        results.remove( type );
        return result;
    }

    Capabilities::Capability* createCapabilityInterface(Capabilities::Capability::Type type)
    {
        Capabilities::Capability* cap = capabilities.value( type );
        capabilities.remove( type );
        return cap;
    }


    mutable QMap<Capabilities::Capability::Type, bool> results;
    mutable QMap<Capabilities::Capability::Type, Capabilities::Capability*> capabilities;
};

class MyComposerMock : public MockComposer
{
public:
    MyComposerMock() : MockComposer( "testComposer" ) {}

    bool hasCapabilityInterface( Capabilities::Capability::Type type ) const
    {
        bool result = results.value( type );
        results.remove( type );
        return result;
    }

    Capabilities::Capability* createCapabilityInterface(Capabilities::Capability::Type type)
    {
        Capabilities::Capability* cap = capabilities.value( type );
        capabilities.remove( type );
        return cap;
    }


    mutable QMap<Capabilities::Capability::Type, bool> results;
    mutable QMap<Capabilities::Capability::Type, Capabilities::Capability*> capabilities;
};

class MyYearMock : public MockYear
{
public:
    MyYearMock() : MockYear( "testYear" ) {}

    bool hasCapabilityInterface( Capabilities::Capability::Type type ) const
    {
        bool result = results.value( type );
        results.remove( type );
        return result;
    }

    Capabilities::Capability* createCapabilityInterface(Capabilities::Capability::Type type)
    {
        Capabilities::Capability* cap = capabilities.value( type );
        capabilities.remove( type );
        return cap;
    }


    mutable QMap<Capabilities::Capability::Type, bool> results;
    mutable QMap<Capabilities::Capability::Type, Capabilities::Capability*> capabilities;
};

class MyOrganiseCapability : public Capabilities::OrganiseCapability
{
public:
    void deleteTrack() {}
};

void
TestProxyCollectionMeta::testHasCapabilityOnSingleTrack()
{
    MyTrackMock *mock = new MyTrackMock();
    QMap<Capabilities::Capability::Type, bool> results;
    results.insert( Capabilities::Capability::Buyable, false );
    results.insert( Capabilities::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::TrackPtr ptr( mock );

    Meta::ProxyTrack cut( 0, ptr );

    QVERIFY( cut.hasCapabilityInterface( Capabilities::Capability::BookmarkThis ) );
    QVERIFY( !cut.hasCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestProxyCollectionMeta::testCreateCapabilityOnSingleTrack()
{
    MyTrackMock *mock = new MyTrackMock();
    QMap<Capabilities::Capability::Type, Capabilities::Capability*>  capabilities;
    capabilities.insert( Capabilities::Capability::Buyable, 0 );
    Capabilities::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Capabilities::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::TrackPtr ptr( mock );

    Meta::ProxyTrack cut( 0, ptr );

    QVERIFY( ! cut.createCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( cut.createCapabilityInterface( Capabilities::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

void
TestProxyCollectionMeta::testHasCapabilityOnSingleAlbum()
{
    MyAlbumMock *mock = new MyAlbumMock();
    QMap<Capabilities::Capability::Type, bool> results;
    results.insert( Capabilities::Capability::Buyable, false );
    results.insert( Capabilities::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::AlbumPtr ptr( mock );

    Meta::ProxyAlbum album( 0, ptr );

    QVERIFY( album.hasCapabilityInterface( Capabilities::Capability::BookmarkThis ) );
    QVERIFY( !album.hasCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestProxyCollectionMeta::testCreateCapabilityOnSingleAlbum()
{
    MyAlbumMock *mock = new MyAlbumMock();
    QMap<Capabilities::Capability::Type, Capabilities::Capability*>  capabilities;
    capabilities.insert( Capabilities::Capability::Buyable, 0 );
    Capabilities::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Capabilities::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::AlbumPtr ptr( mock );

    Meta::ProxyAlbum album( 0, ptr );

    QVERIFY( ! album.createCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( album.createCapabilityInterface( Capabilities::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

void
TestProxyCollectionMeta::testHasCapabilityOnSingleArtist()
{
    MyArtistMock *mock = new MyArtistMock();
    QMap<Capabilities::Capability::Type, bool> results;
    results.insert( Capabilities::Capability::Buyable, false );
    results.insert( Capabilities::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::ArtistPtr ptr( mock );

    Meta::ProxyArtist artist( 0, ptr );

    QVERIFY( artist.hasCapabilityInterface( Capabilities::Capability::BookmarkThis ) );
    QVERIFY( !artist.hasCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestProxyCollectionMeta::testCreateCapabilityOnSingleArtist()
{
    MyArtistMock *mock = new MyArtistMock();
    QMap<Capabilities::Capability::Type, Capabilities::Capability*>  capabilities;
    capabilities.insert( Capabilities::Capability::Buyable, 0 );
    Capabilities::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Capabilities::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::ArtistPtr ptr( mock );

    Meta::ProxyArtist artist( 0, ptr );

    QVERIFY( ! artist.createCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( artist.createCapabilityInterface( Capabilities::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

void
TestProxyCollectionMeta::testHasCapabilityOnSingleComposer()
{
    MyComposerMock *mock = new MyComposerMock();
    QMap<Capabilities::Capability::Type, bool> results;
    results.insert( Capabilities::Capability::Buyable, false );
    results.insert( Capabilities::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::ComposerPtr ptr( mock );

    Meta::ProxyComposer cut( 0, ptr );

    QVERIFY( cut.hasCapabilityInterface( Capabilities::Capability::BookmarkThis ) );
    QVERIFY( !cut.hasCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestProxyCollectionMeta::testCreateCapabilityOnSingleComposer()
{
    MyComposerMock *mock = new MyComposerMock();
    QMap<Capabilities::Capability::Type, Capabilities::Capability*>  capabilities;
    capabilities.insert( Capabilities::Capability::Buyable, 0 );
    Capabilities::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Capabilities::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::ComposerPtr ptr( mock );

    Meta::ProxyComposer cut( 0, ptr );

    QVERIFY( ! cut.createCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( cut.createCapabilityInterface( Capabilities::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

void
TestProxyCollectionMeta::testHasCapabilityOnSingleGenre()
{
    MyGenreMock *mock = new MyGenreMock();
    QMap<Capabilities::Capability::Type, bool> results;
    results.insert( Capabilities::Capability::Buyable, false );
    results.insert( Capabilities::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::GenrePtr ptr( mock );

    Meta::ProxyGenre cut( 0, ptr );

    QVERIFY( cut.hasCapabilityInterface( Capabilities::Capability::BookmarkThis ) );
    QVERIFY( !cut.hasCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestProxyCollectionMeta::testCreateCapabilityOnSingleGenre()
{
    MyGenreMock *mock = new MyGenreMock();
    QMap<Capabilities::Capability::Type, Capabilities::Capability*>  capabilities;
    capabilities.insert( Capabilities::Capability::Buyable, 0 );
    Capabilities::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Capabilities::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::GenrePtr ptr( mock );

    Meta::ProxyGenre cut( 0, ptr );

    QVERIFY( ! cut.createCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( cut.createCapabilityInterface( Capabilities::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

void
TestProxyCollectionMeta::testHasCapabilityOnSingleYear()
{
    MyYearMock *mock = new MyYearMock();
    QMap<Capabilities::Capability::Type, bool> results;
    results.insert( Capabilities::Capability::Buyable, false );
    results.insert( Capabilities::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::YearPtr ptr( mock );

    Meta::ProxyYear cut( 0, ptr );

    QVERIFY( cut.hasCapabilityInterface( Capabilities::Capability::BookmarkThis ) );
    QVERIFY( !cut.hasCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestProxyCollectionMeta::testCreateCapabilityOnSingleYear()
{
    MyYearMock *mock = new MyYearMock();
    QMap<Capabilities::Capability::Type, Capabilities::Capability*>  capabilities;
    capabilities.insert( Capabilities::Capability::Buyable, 0 );
    Capabilities::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Capabilities::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::YearPtr ptr( mock );

    Meta::ProxyYear cut( 0, ptr );

    QVERIFY( ! cut.createCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( cut.createCapabilityInterface( Capabilities::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

class MyEditCapability : public Capabilities::EditCapability
{
public:
    MyEditCapability() : Capabilities::EditCapability()
            , beginCallCount(0)
            , endCallcount(0)
            , abortCallcount(0) {}
    virtual bool isEditable() const { return true; }
    virtual void setAlbum( const QString &newAlbum ) {}
    virtual void setArtist( const QString &newArtist ) {}
    virtual void setComposer( const QString &newComposer ) {};
    virtual void setGenre( const QString &newGenre ) {};
    virtual void setYear( const QString &newYear ) {};
    virtual void setTitle( const QString &newTitle ) {};
    virtual void setComment( const QString &newComment ) {};
    virtual void setTrackNumber( int newTrackNumber ) {};
    virtual void setDiscNumber( int newDiscNumber ) {};
    virtual void setBpm( const qreal newBpm ) {};
    virtual void beginMetaDataUpdate() { beginCallCount++; };
    virtual void endMetaDataUpdate() { endCallcount++; };
    virtual void abortMetaDataUpdate() { abortCallcount++; };

    int beginCallCount;
    int endCallcount;
    int abortCallcount;
};

void
TestProxyCollectionMeta::testEditableCapabilityOnMultipleTracks()
{
    MyTrackMock *mock1 = new MyTrackMock();
    MyTrackMock *mock2 = new MyTrackMock();
    QMap<Capabilities::Capability::Type, bool> result;
    result.insert( Capabilities::Capability::Editable, true );
    MyEditCapability *cap1 = new MyEditCapability();
    MyEditCapability *cap2 = new MyEditCapability();
    mock1->capabilities.insert( Capabilities::Capability::Editable, cap1 );
    mock2->capabilities.insert( Capabilities::Capability::Editable, cap2 );
    mock1->results = result;
    mock2->results = result;

    Meta::TrackPtr ptr1( mock1 );
    Meta::TrackPtr ptr2( mock2 );

    Collections::ProxyCollection *proxyCollection = new Collections::ProxyCollection();

    QSignalSpy spy( proxyCollection, SIGNAL(updated()));
    QVERIFY( spy.isValid() );

    Meta::ProxyTrack cut( proxyCollection, ptr1 );
    cut.add( ptr2 );

    QVERIFY( cut.hasCapabilityInterface( Capabilities::Capability::Editable ) );

    Capabilities::EditCapability *editCap = cut.create<Capabilities::EditCapability>();
    QVERIFY( editCap );
    QVERIFY( editCap->isEditable() );

    QCOMPARE( cap1->beginCallCount, 0 );
    QCOMPARE( cap1->beginCallCount, 0 );
    editCap->beginMetaDataUpdate();
    QCOMPARE( cap1->beginCallCount, 1 );
    QCOMPARE( cap1->beginCallCount, 1 );

    QCOMPARE( cap1->endCallcount, 0 );
    QCOMPARE( cap2->endCallcount, 0 );
    editCap->endMetaDataUpdate();
    QCOMPARE( cap1->endCallcount, 1 );
    QCOMPARE( cap2->endCallcount, 1 );

    //the signal is delayed a bit, but that is ok
    QTest::qWait( 50 );
    //required so that the colleection browser refreshes itself
    QCOMPARE( spy.count(), 1 );

    QCOMPARE( cap1->abortCallcount, 0 );
    QCOMPARE( cap2->abortCallcount, 0 );
    editCap->abortMetaDataUpdate();
    QCOMPARE( cap1->abortCallcount, 1 );
    QCOMPARE( cap2->abortCallcount, 1 );

    QPointer<MyEditCapability> qpointer1( cap1 );
    QPointer<MyEditCapability> qpointer2( cap2 );
    QVERIFY( qpointer1 );
    QVERIFY( qpointer2 );
    delete editCap;
    QVERIFY( !qpointer1 );
    QVERIFY( !qpointer2 );
}

using ::testing::Return;
using ::testing::AnyNumber;

void
TestProxyCollectionMeta::testPrettyUrl()
{
    Meta::MockTrack *mock = new ::testing::NiceMock<Meta::MockTrack>();
    EXPECT_CALL( *mock, prettyUrl() ).Times( AnyNumber() ).WillRepeatedly( Return( "foo" ) );

    Meta::TrackPtr trackPtr( mock );

    Meta::ProxyTrack track( 0, trackPtr );

    QCOMPARE( track.prettyUrl(), QString( "foo" ) );
}
