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

#include "TestAggregateMeta.h"

#include "core/capabilities/OrganiseCapability.h"
#include "core/meta/Meta.h"
#include "core/meta/TrackEditor.h"
#include "core/support/Debug.h"
#include "core-impl/collections/aggregate/AggregateCollection.h"
#include "core-impl/collections/aggregate/AggregateMeta.h"

#include "mocks/MetaMock.h"
#include "mocks/MockTrack.h"

#include <QMap>
#include <QSignalSpy>

#include <gmock/gmock.h>

QTEST_GUILESS_MAIN( TestAggregateMeta )

TestAggregateMeta::TestAggregateMeta()
{
    int argc = 1;
    char **argv = (char **) malloc(sizeof(char *));
    argv[0] = strdup( QCoreApplication::applicationName().toLocal8Bit().data() );
    ::testing::InitGoogleMock( &argc, argv );
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

    Meta::TrackEditorPtr editor()
    {
        return trackEditors.isEmpty() ? Meta::TrackEditorPtr() : trackEditors.takeFirst();
    }

    mutable QMap<Capabilities::Capability::Type, bool> results;
    mutable QMap<Capabilities::Capability::Type, Capabilities::Capability*> capabilities;
    QList<Meta::TrackEditorPtr> trackEditors;
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
TestAggregateMeta::testHasCapabilityOnSingleTrack()
{
    MyTrackMock *mock = new MyTrackMock();
    QMap<Capabilities::Capability::Type, bool> results;
    results.insert( Capabilities::Capability::Buyable, false );
    results.insert( Capabilities::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::TrackPtr ptr( mock );

    Meta::AggregateTrack cut( 0, ptr );

    QVERIFY( cut.hasCapabilityInterface( Capabilities::Capability::BookmarkThis ) );
    QVERIFY( !cut.hasCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestAggregateMeta::testCreateCapabilityOnSingleTrack()
{
    MyTrackMock *mock = new MyTrackMock();
    QMap<Capabilities::Capability::Type, Capabilities::Capability*>  capabilities;
    capabilities.insert( Capabilities::Capability::Buyable, 0 );
    Capabilities::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Capabilities::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::TrackPtr ptr( mock );

    Meta::AggregateTrack cut( 0, ptr );

    QVERIFY( ! cut.createCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( cut.createCapabilityInterface( Capabilities::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

void
TestAggregateMeta::testHasCapabilityOnSingleAlbum()
{
    MyAlbumMock *mock = new MyAlbumMock();
    QMap<Capabilities::Capability::Type, bool> results;
    results.insert( Capabilities::Capability::Buyable, false );
    results.insert( Capabilities::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::AlbumPtr ptr( mock );

    Meta::AggregateAlbum album( 0, ptr );

    QVERIFY( album.hasCapabilityInterface( Capabilities::Capability::BookmarkThis ) );
    QVERIFY( !album.hasCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestAggregateMeta::testCreateCapabilityOnSingleAlbum()
{
    MyAlbumMock *mock = new MyAlbumMock();
    QMap<Capabilities::Capability::Type, Capabilities::Capability*>  capabilities;
    capabilities.insert( Capabilities::Capability::Buyable, 0 );
    Capabilities::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Capabilities::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::AlbumPtr ptr( mock );

    Meta::AggregateAlbum album( 0, ptr );

    QVERIFY( ! album.createCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( album.createCapabilityInterface( Capabilities::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

void
TestAggregateMeta::testHasCapabilityOnSingleArtist()
{
    MyArtistMock *mock = new MyArtistMock();
    QMap<Capabilities::Capability::Type, bool> results;
    results.insert( Capabilities::Capability::Buyable, false );
    results.insert( Capabilities::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::ArtistPtr ptr( mock );

    Meta::AggregateArtist artist( 0, ptr );

    QVERIFY( artist.hasCapabilityInterface( Capabilities::Capability::BookmarkThis ) );
    QVERIFY( !artist.hasCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestAggregateMeta::testCreateCapabilityOnSingleArtist()
{
    MyArtistMock *mock = new MyArtistMock();
    QMap<Capabilities::Capability::Type, Capabilities::Capability*>  capabilities;
    capabilities.insert( Capabilities::Capability::Buyable, 0 );
    Capabilities::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Capabilities::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::ArtistPtr ptr( mock );

    Meta::AggregateArtist artist( 0, ptr );

    QVERIFY( ! artist.createCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( artist.createCapabilityInterface( Capabilities::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

void
TestAggregateMeta::testHasCapabilityOnSingleComposer()
{
    MyComposerMock *mock = new MyComposerMock();
    QMap<Capabilities::Capability::Type, bool> results;
    results.insert( Capabilities::Capability::Buyable, false );
    results.insert( Capabilities::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::ComposerPtr ptr( mock );

    Meta::AggregateComposer cut( 0, ptr );

    QVERIFY( cut.hasCapabilityInterface( Capabilities::Capability::BookmarkThis ) );
    QVERIFY( !cut.hasCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestAggregateMeta::testCreateCapabilityOnSingleComposer()
{
    MyComposerMock *mock = new MyComposerMock();
    QMap<Capabilities::Capability::Type, Capabilities::Capability*>  capabilities;
    capabilities.insert( Capabilities::Capability::Buyable, 0 );
    Capabilities::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Capabilities::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::ComposerPtr ptr( mock );

    Meta::AggregateComposer cut( 0, ptr );

    QVERIFY( ! cut.createCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( cut.createCapabilityInterface( Capabilities::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

void
TestAggregateMeta::testHasCapabilityOnSingleGenre()
{
    MyGenreMock *mock = new MyGenreMock();
    QMap<Capabilities::Capability::Type, bool> results;
    results.insert( Capabilities::Capability::Buyable, false );
    results.insert( Capabilities::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::GenrePtr ptr( mock );

    Meta::AggregateGenre cut( 0, ptr );

    QVERIFY( cut.hasCapabilityInterface( Capabilities::Capability::BookmarkThis ) );
    QVERIFY( !cut.hasCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestAggregateMeta::testCreateCapabilityOnSingleGenre()
{
    MyGenreMock *mock = new MyGenreMock();
    QMap<Capabilities::Capability::Type, Capabilities::Capability*>  capabilities;
    capabilities.insert( Capabilities::Capability::Buyable, 0 );
    Capabilities::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Capabilities::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::GenrePtr ptr( mock );

    Meta::AggregateGenre cut( 0, ptr );

    QVERIFY( ! cut.createCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( cut.createCapabilityInterface( Capabilities::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

void
TestAggregateMeta::testHasCapabilityOnSingleYear()
{
    MyYearMock *mock = new MyYearMock();
    QMap<Capabilities::Capability::Type, bool> results;
    results.insert( Capabilities::Capability::Buyable, false );
    results.insert( Capabilities::Capability::BookmarkThis, true );
    mock->results = results;

    Meta::YearPtr ptr( mock );

    Meta::AggreagateYear cut( 0, ptr );

    QVERIFY( cut.hasCapabilityInterface( Capabilities::Capability::BookmarkThis ) );
    QVERIFY( !cut.hasCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( mock->results.count(), 0 );
}

void
TestAggregateMeta::testCreateCapabilityOnSingleYear()
{
    MyYearMock *mock = new MyYearMock();
    QMap<Capabilities::Capability::Type, Capabilities::Capability*>  capabilities;
    capabilities.insert( Capabilities::Capability::Buyable, 0 );
    Capabilities::Capability *cap = new MyOrganiseCapability();
    capabilities.insert( Capabilities::Capability::Organisable, cap );

    mock->capabilities = capabilities;

    Meta::YearPtr ptr( mock );

    Meta::AggreagateYear cut( 0, ptr );

    QVERIFY( ! cut.createCapabilityInterface( Capabilities::Capability::Buyable ) );
    QCOMPARE( cut.createCapabilityInterface( Capabilities::Capability::Organisable ), cap );
    QCOMPARE( mock->capabilities.count(), 0 );
    delete cap;
}

class MyTrackEditor : public Meta::TrackEditor
{
public:
    MyTrackEditor() : Meta::TrackEditor()
            , beginCallCount(0)
            , endCallcount(0) {}
    virtual void setAlbum( const QString &newAlbum ) { Q_UNUSED( newAlbum ) }
    virtual void setAlbumArtist( const QString &newAlbumArtist ) { Q_UNUSED( newAlbumArtist ) }
    virtual void setArtist( const QString &newArtist ) { Q_UNUSED( newArtist ) }
    virtual void setComposer( const QString &newComposer ) { Q_UNUSED( newComposer ) };
    virtual void setGenre( const QString &newGenre ) { Q_UNUSED( newGenre ) };
    virtual void setYear( int newYear ) { Q_UNUSED( newYear ) };
    virtual void setTitle( const QString &newTitle ) { Q_UNUSED( newTitle ) };
    virtual void setComment( const QString &newComment ) { Q_UNUSED( newComment ) };
    virtual void setTrackNumber( int newTrackNumber ) { Q_UNUSED( newTrackNumber ) };
    virtual void setDiscNumber( int newDiscNumber ) { Q_UNUSED( newDiscNumber ) };
    virtual void setBpm( const qreal newBpm ) { Q_UNUSED( newBpm ) };
    virtual void beginUpdate() { beginCallCount++; };
    virtual void endUpdate() { endCallcount++; };

    int beginCallCount;
    int endCallcount;
};

void
TestAggregateMeta::testEditableCapabilityOnMultipleTracks()
{
    MyTrackMock *mock1 = new MyTrackMock();
    MyTrackMock *mock2 = new MyTrackMock();
    AmarokSharedPointer<MyTrackEditor> cap1 ( new MyTrackEditor() );
    AmarokSharedPointer<MyTrackEditor> cap2 ( new MyTrackEditor() );
    mock1->trackEditors << Meta::TrackEditorPtr( cap1.data() );
    mock2->trackEditors << Meta::TrackEditorPtr( cap2.data() );

    Meta::TrackPtr ptr1( mock1 );
    Meta::TrackPtr ptr2( mock2 );

    Collections::AggregateCollection *collection = new Collections::AggregateCollection();

    QSignalSpy spy( collection, &Collections::AggregateCollection::updated);
    QVERIFY( spy.isValid() );

    Meta::AggregateTrack cut( collection, ptr1 );
    cut.add( ptr2 );

    Meta::TrackEditorPtr editCap = cut.editor();
    QVERIFY( editCap );

    QCOMPARE( cap1->beginCallCount, 0 );
    QCOMPARE( cap2->beginCallCount, 0 );
    editCap->beginUpdate();
    QCOMPARE( cap1->beginCallCount, 1 );
    QCOMPARE( cap2->beginCallCount, 1 );

    QCOMPARE( cap1->endCallcount, 0 );
    QCOMPARE( cap2->endCallcount, 0 );
    editCap->endUpdate();
    QCOMPARE( cap1->endCallcount, 1 );
    QCOMPARE( cap2->endCallcount, 1 );

    //the signal is delayed a bit, but that is ok
    QTest::qWait( 50 );
    //required so that the collection browser refreshes itself
    QCOMPARE( spy.count(), 1 );
}

using ::testing::Return;
using ::testing::AnyNumber;

void
TestAggregateMeta::testPrettyUrl()
{
    Meta::MockTrack *mock = new ::testing::NiceMock<Meta::MockTrack>();
    EXPECT_CALL( *mock, prettyUrl() ).Times( AnyNumber() ).WillRepeatedly( Return( "foo" ) );

    Meta::TrackPtr trackPtr( mock );

    Meta::AggregateTrack track( 0, trackPtr );

    QCOMPARE( track.prettyUrl(), QString( "foo" ) );
}
