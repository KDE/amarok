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
#include "Debug.h"
#include "meta/Capability.h"
#include "meta/Meta.h"
#include "meta/capabilities/EditCapability.h"
#include "meta/capabilities/OrganiseCapability.h"

#include "mocks/MetaMock.h"

#include <QMap>
#include <QSignalSpy>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestProxyCollectionMeta )

//required for Debug.h
QMutex Debug::mutex;

TestProxyCollectionMeta::TestProxyCollectionMeta()
{
}

class MyTrackMock : public MetaMock
{
public:
    MyTrackMock() : MetaMock( QVariantMap() ) {}

    bool hasCapabilityInterface( Meta::Capability::Type type ) const
    {
        bool result = results.value( type );
        results.remove( type );
        return result;
    }

    Meta::Capability* createCapabilityInterface(Meta::Capability::Type type)
    {
        Meta::Capability* cap = capabilities.value( type );
        capabilities.remove( type );
        return cap;
    }


    mutable QMap<Meta::Capability::Type, bool> results;
    mutable QMap<Meta::Capability::Type, Meta::Capability*> capabilities;
};

class MyAlbumMock : public MockAlbum
{
public:
    MyAlbumMock() : MockAlbum( "testAlbum" ) {}

    bool hasCapabilityInterface( Meta::Capability::Type type ) const
    {
        bool result = results.value( type );
        results.remove( type );
        return result;
    }

    Meta::Capability* createCapabilityInterface(Meta::Capability::Type type)
    {
        Meta::Capability* cap = capabilities.value( type );
        capabilities.remove( type );
        return cap;
    }


    mutable QMap<Meta::Capability::Type, bool> results;
    mutable QMap<Meta::Capability::Type, Meta::Capability*> capabilities;
};

class MyArtistMock : public MockArtist
{
public:
    MyArtistMock() : MockArtist( "testArtist" ) {}

    bool hasCapabilityInterface( Meta::Capability::Type type ) const
    {
        bool result = results.value( type );
        results.remove( type );
        return result;
    }

    Meta::Capability* createCapabilityInterface(Meta::Capability::Type type)
    {
        Meta::Capability* cap = capabilities.value( type );
        capabilities.remove( type );
        return cap;
    }


    mutable QMap<Meta::Capability::Type, bool> results;
    mutable QMap<Meta::Capability::Type, Meta::Capability*> capabilities;
};

class MyGenreMock : public MockGenre
{
public:
    MyGenreMock() : MockGenre( "testGenre" ) {}

    bool hasCapabilityInterface( Meta::Capability::Type type ) const
    {
        bool result = results.value( type );
        results.remove( type );
        return result;
    }

    Meta::Capability* createCapabilityInterface(Meta::Capability::Type type)
    {
        Meta::Capability* cap = capabilities.value( type );
        capabilities.remove( type );
        return cap;
    }


    mutable QMap<Meta::Capability::Type, bool> results;
    mutable QMap<Meta::Capability::Type, Meta::Capability*> capabilities;
};

class MyComposerMock : public MockComposer
{
public:
    MyComposerMock() : MockComposer( "testComposer" ) {}

    bool hasCapabilityInterface( Meta::Capability::Type type ) const
    {
        bool result = results.value( type );
        results.remove( type );
        return result;
    }

    Meta::Capability* createCapabilityInterface(Meta::Capability::Type type)
    {
        Meta::Capability* cap = capabilities.value( type );
        capabilities.remove( type );
        return cap;
    }


    mutable QMap<Meta::Capability::Type, bool> results;
    mutable QMap<Meta::Capability::Type, Meta::Capability*> capabilities;
};

class MyYearMock : public MockYear
{
public:
    MyYearMock() : MockYear( "testYear" ) {}

    bool hasCapabilityInterface( Meta::Capability::Type type ) const
    {
        bool result = results.value( type );
        results.remove( type );
        return result;
    }

    Meta::Capability* createCapabilityInterface(Meta::Capability::Type type)
    {
        Meta::Capability* cap = capabilities.value( type );
        capabilities.remove( type );
        return cap;
    }


    mutable QMap<Meta::Capability::Type, bool> results;
    mutable QMap<Meta::Capability::Type, Meta::Capability*> capabilities;
};

class MyOrganiseCapability : public Meta::OrganiseCapability
{
public:
    void deleteTrack() {}
};

void
TestProxyCollectionMeta::testHasCapabilityOnSingleTrack()
{
    MyTrackMock *mock = new MyTrackMock();
    QMap<Meta::Capability::Type, bool> results;
    results.insert( Meta::Capability::Buyable, false );
    results.insert( Meta::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::TrackPtr ptr( mock );

    ProxyCollection::Track cut( 0, ptr );

    QVERIFY( cut.hasCapabilityInterface( Meta::Capability::BookmarkThis ) );
    QVERIFY( !cut.hasCapabilityInterface( Meta::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestProxyCollectionMeta::testCreateCapabilityOnSingleTrack()
{
    MyTrackMock *mock = new MyTrackMock();
    QMap<Meta::Capability::Type, Meta::Capability*>  capabilities;
    capabilities.insert( Meta::Capability::Buyable, 0 );
    Meta::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Meta::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::TrackPtr ptr( mock );

    ProxyCollection::Track cut( 0, ptr );

    QVERIFY( ! cut.createCapabilityInterface( Meta::Capability::Buyable ) );
    QCOMPARE( cut.createCapabilityInterface( Meta::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

void
TestProxyCollectionMeta::testHasCapabilityOnSingleAlbum()
{
    MyAlbumMock *mock = new MyAlbumMock();
    QMap<Meta::Capability::Type, bool> results;
    results.insert( Meta::Capability::Buyable, false );
    results.insert( Meta::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::AlbumPtr ptr( mock );

    ProxyCollection::Album album( 0, ptr );

    QVERIFY( album.hasCapabilityInterface( Meta::Capability::BookmarkThis ) );
    QVERIFY( !album.hasCapabilityInterface( Meta::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestProxyCollectionMeta::testCreateCapabilityOnSingleAlbum()
{
    MyAlbumMock *mock = new MyAlbumMock();
    QMap<Meta::Capability::Type, Meta::Capability*>  capabilities;
    capabilities.insert( Meta::Capability::Buyable, 0 );
    Meta::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Meta::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::AlbumPtr ptr( mock );

    ProxyCollection::Album album( 0, ptr );

    QVERIFY( ! album.createCapabilityInterface( Meta::Capability::Buyable ) );
    QCOMPARE( album.createCapabilityInterface( Meta::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

void
TestProxyCollectionMeta::testHasCapabilityOnSingleArtist()
{
    MyArtistMock *mock = new MyArtistMock();
    QMap<Meta::Capability::Type, bool> results;
    results.insert( Meta::Capability::Buyable, false );
    results.insert( Meta::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::ArtistPtr ptr( mock );

    ProxyCollection::Artist artist( 0, ptr );

    QVERIFY( artist.hasCapabilityInterface( Meta::Capability::BookmarkThis ) );
    QVERIFY( !artist.hasCapabilityInterface( Meta::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestProxyCollectionMeta::testCreateCapabilityOnSingleArtist()
{
    MyArtistMock *mock = new MyArtistMock();
    QMap<Meta::Capability::Type, Meta::Capability*>  capabilities;
    capabilities.insert( Meta::Capability::Buyable, 0 );
    Meta::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Meta::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::ArtistPtr ptr( mock );

    ProxyCollection::Artist artist( 0, ptr );

    QVERIFY( ! artist.createCapabilityInterface( Meta::Capability::Buyable ) );
    QCOMPARE( artist.createCapabilityInterface( Meta::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

void
TestProxyCollectionMeta::testHasCapabilityOnSingleComposer()
{
    MyComposerMock *mock = new MyComposerMock();
    QMap<Meta::Capability::Type, bool> results;
    results.insert( Meta::Capability::Buyable, false );
    results.insert( Meta::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::ComposerPtr ptr( mock );

    ProxyCollection::Composer cut( 0, ptr );

    QVERIFY( cut.hasCapabilityInterface( Meta::Capability::BookmarkThis ) );
    QVERIFY( !cut.hasCapabilityInterface( Meta::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestProxyCollectionMeta::testCreateCapabilityOnSingleComposer()
{
    MyComposerMock *mock = new MyComposerMock();
    QMap<Meta::Capability::Type, Meta::Capability*>  capabilities;
    capabilities.insert( Meta::Capability::Buyable, 0 );
    Meta::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Meta::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::ComposerPtr ptr( mock );

    ProxyCollection::Composer cut( 0, ptr );

    QVERIFY( ! cut.createCapabilityInterface( Meta::Capability::Buyable ) );
    QCOMPARE( cut.createCapabilityInterface( Meta::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

void
TestProxyCollectionMeta::testHasCapabilityOnSingleGenre()
{
    MyGenreMock *mock = new MyGenreMock();
    QMap<Meta::Capability::Type, bool> results;
    results.insert( Meta::Capability::Buyable, false );
    results.insert( Meta::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::GenrePtr ptr( mock );

    ProxyCollection::Genre cut( 0, ptr );

    QVERIFY( cut.hasCapabilityInterface( Meta::Capability::BookmarkThis ) );
    QVERIFY( !cut.hasCapabilityInterface( Meta::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestProxyCollectionMeta::testCreateCapabilityOnSingleGenre()
{
    MyGenreMock *mock = new MyGenreMock();
    QMap<Meta::Capability::Type, Meta::Capability*>  capabilities;
    capabilities.insert( Meta::Capability::Buyable, 0 );
    Meta::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Meta::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::GenrePtr ptr( mock );

    ProxyCollection::Genre cut( 0, ptr );

    QVERIFY( ! cut.createCapabilityInterface( Meta::Capability::Buyable ) );
    QCOMPARE( cut.createCapabilityInterface( Meta::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

void
TestProxyCollectionMeta::testHasCapabilityOnSingleYear()
{
    MyYearMock *mock = new MyYearMock();
    QMap<Meta::Capability::Type, bool> results;
    results.insert( Meta::Capability::Buyable, false );
    results.insert( Meta::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::YearPtr ptr( mock );

    ProxyCollection::Year cut( 0, ptr );

    QVERIFY( cut.hasCapabilityInterface( Meta::Capability::BookmarkThis ) );
    QVERIFY( !cut.hasCapabilityInterface( Meta::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestProxyCollectionMeta::testCreateCapabilityOnSingleYear()
{
    MyYearMock *mock = new MyYearMock();
    QMap<Meta::Capability::Type, Meta::Capability*>  capabilities;
    capabilities.insert( Meta::Capability::Buyable, 0 );
    Meta::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Meta::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::YearPtr ptr( mock );

    ProxyCollection::Year cut( 0, ptr );

    QVERIFY( ! cut.createCapabilityInterface( Meta::Capability::Buyable ) );
    QCOMPARE( cut.createCapabilityInterface( Meta::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

class MyEditCapability : public Meta::EditCapability
{
public:
    MyEditCapability() : Meta::EditCapability()
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
    virtual void setBpm( const float newBpm ) {};
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
    QMap<Meta::Capability::Type, bool> result;
    result.insert( Meta::Capability::Editable, true );
    MyEditCapability *cap1 = new MyEditCapability();
    MyEditCapability *cap2 = new MyEditCapability();
    mock1->capabilities.insert( Meta::Capability::Editable, cap1 );
    mock2->capabilities.insert( Meta::Capability::Editable, cap2 );
    mock1->results = result;
    mock2->results = result;

    Meta::TrackPtr ptr1( mock1 );
    Meta::TrackPtr ptr2( mock2 );

    ProxyCollection::Collection *proxyCollection = new ProxyCollection::Collection();

    QSignalSpy spy( proxyCollection, SIGNAL(updated()));
    QVERIFY( spy.isValid() );

    ProxyCollection::Track cut( proxyCollection, ptr1 );
    cut.add( ptr2 );

    QVERIFY( cut.hasCapabilityInterface( Meta::Capability::Editable ) );

    Meta::EditCapability *editCap = cut.create<Meta::EditCapability>();
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
