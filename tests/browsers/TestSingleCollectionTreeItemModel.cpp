/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "TestSingleCollectionTreeItemModel.h"

#include "amarokconfig.h"
#include "core/meta/Meta.h"
#include "browsers/CollectionTreeItemModelBase.h"
#include "browsers/SingleCollectionTreeItemModel.h"

#include "CollectionTestImpl.h"
#include "mocks/MockTrack.h"
#include "mocks/MockAlbum.h"
#include "mocks/MockArtist.h"

#include <KLocalizedString>

#include <QApplication>
#include <QModelIndex>
#include <QSet>
#include <QSignalSpy>
#include <QStringList>

#include <gmock/gmock.h>

using ::testing::Return;
using ::testing::AnyNumber;
using ::testing::_;

QTEST_MAIN( TestSingleCollectionTreeItemModel )

TestSingleCollectionTreeItemModel::TestSingleCollectionTreeItemModel()
{
    KLocalizedString::setApplicationDomain("amarok-test");
}

void addMockTrack( Collections::CollectionTestImpl *coll, const QString &trackName, const QString &artistName, const QString &albumName )
{
    Meta::MockTrack *track = new Meta::MockTrack();
    ::testing::Mock::AllowLeak( track );
    Meta::TrackPtr trackPtr( track );
    EXPECT_CALL( *track, name() ).Times( AnyNumber() ).WillRepeatedly( Return( trackName ) );
    EXPECT_CALL( *track, prettyName() ).Times( AnyNumber() ).WillRepeatedly( Return( trackName ) );
    EXPECT_CALL( *track, uidUrl() ).Times( AnyNumber() ).WillRepeatedly( Return( trackName + '_' + artistName + '_' + albumName ) );
    EXPECT_CALL( *track, playableUrl() ).Times( AnyNumber() ).WillRepeatedly( Return( QUrl( '/' + track->uidUrl() ) ) );
    EXPECT_CALL( *track, composer() ).Times( AnyNumber() ).WillRepeatedly( Return( Meta::ComposerPtr() ) );
    EXPECT_CALL( *track, genre() ).Times( AnyNumber() ).WillRepeatedly( Return( Meta::GenrePtr() ) );
    EXPECT_CALL( *track, year() ).Times( AnyNumber() ).WillRepeatedly( Return( Meta::YearPtr() ) );
    coll->mc->addTrack( trackPtr );

    Meta::AlbumPtr albumPtr = coll->mc->albumMap().value( albumName, QString() /* no album artist */ );
    Meta::MockAlbum *album;
    Meta::TrackList albumTracks;
    if( albumPtr )
    {
        album = dynamic_cast<Meta::MockAlbum*>( albumPtr.data() );
        if( !album )
        {
            QFAIL( "expected a Meta::MockAlbum" );
            return;
        }
        albumTracks = albumPtr->tracks();
    }
    else
    {
        album = new Meta::MockAlbum();
        ::testing::Mock::AllowLeak( album );
        albumPtr = Meta::AlbumPtr( album );
        EXPECT_CALL( *album, name() ).Times( AnyNumber() ).WillRepeatedly( Return( albumName ) );
        EXPECT_CALL( *album, prettyName() ).Times( AnyNumber() ).WillRepeatedly( Return( albumName ) );
        EXPECT_CALL( *album, hasAlbumArtist() ).Times( AnyNumber() ).WillRepeatedly( Return( false ) );
        EXPECT_CALL( *album, isCompilation() ).Times( AnyNumber() ).WillRepeatedly( Return( false ) ); //inconsistent
        coll->mc->addAlbum( albumPtr );
    }
    albumTracks << trackPtr;
    EXPECT_CALL( *album, tracks() ).Times( AnyNumber() ).WillRepeatedly( Return( albumTracks ) );

    EXPECT_CALL( *track, album() ).Times( AnyNumber() ).WillRepeatedly( Return( albumPtr ) );

    Meta::ArtistPtr artistPtr = coll->mc->artistMap().value( artistName );
    Meta::MockArtist *artist;
    Meta::TrackList artistTracks;
    if( artistPtr )
    {
        artist = dynamic_cast<Meta::MockArtist*>( artistPtr.data() );
        if( !artist )
        {
            QFAIL( "expected a Meta::MockArtist" );
            return;
        }
        artistTracks = artistPtr->tracks();
    }
    else
    {
        artist = new Meta::MockArtist();
        ::testing::Mock::AllowLeak( artist );
        artistPtr = Meta::ArtistPtr( artist );
        EXPECT_CALL( *artist, name() ).Times( AnyNumber() ).WillRepeatedly( Return( artistName ) );
        EXPECT_CALL( *artist, prettyName() ).Times( AnyNumber() ).WillRepeatedly( Return( artistName ) );
        coll->mc->addArtist( artistPtr );
    }
    artistTracks << trackPtr;
    EXPECT_CALL( *artist, tracks() ).Times( AnyNumber() ).WillRepeatedly( Return( artistTracks ) );
    EXPECT_CALL( *track, artist() ).Times( AnyNumber() ).WillRepeatedly( Return( artistPtr ) );
    EXPECT_CALL( *album, albumArtist() ).Times( AnyNumber() ).WillRepeatedly( Return( artistPtr ) );
}

void
TestSingleCollectionTreeItemModel::initTestCase()
{
    qRegisterMetaType<Meta::TrackList>();
    qRegisterMetaType<Meta::AlbumList>();
    qRegisterMetaType<Meta::ArtistList>();
    qRegisterMetaType<Meta::DataList>();
    qRegisterMetaType<Meta::GenreList>();
    qRegisterMetaType<Meta::ComposerList>();
    qRegisterMetaType<Meta::YearList>();
    qRegisterMetaType<Meta::LabelList>();

    AmarokConfig::instance("amarokrc");
}

#define loadChildren( itemModel, idx ) \
{ \
    if( itemModel->canFetchMore( idx ) ) { \
        QSignalSpy spy( itemModel, &SingleCollectionTreeItemModel::allQueriesFinished ); \
        itemModel->fetchMore( idx ); \
        QVERIFY( spy.wait( 5000 ) ); \
    } \
}

void
TestSingleCollectionTreeItemModel::testAddNewArtist()
{
    Collections::CollectionTestImpl *coll = new Collections::CollectionTestImpl( "test" );
    addMockTrack( coll, "track1", "artist1", "album1" );

    QList<CategoryId::CatMenuId> levels;
    levels<< CategoryId::Artist << CategoryId::Album;

    SingleCollectionTreeItemModel *model = new SingleCollectionTreeItemModel( coll, levels );

    loadChildren( model, QModelIndex() );

    QCOMPARE( model->rowCount( QModelIndex() ), 1 );

    {
        QModelIndex artist1Index = model->index( 0, 0, QModelIndex() );
        QCOMPARE( model->data( artist1Index, Qt::DisplayRole ).toString(), QString( "artist1" ) );
    }

    addMockTrack( coll, "track2", "artist2", "album2" );

    model->slotFilter();

    loadChildren( model, QModelIndex() );

    QCOMPARE( model->rowCount( QModelIndex() ), 2 );

    QSet<QString> artists;

    QModelIndex idx1 = model->index( 0, 0, QModelIndex() );
    artists << model->data( idx1, Qt::DisplayRole ).toString();

    QModelIndex idx2 = model->index( 1, 0, QModelIndex() );
    artists << model->data( idx2, Qt::DisplayRole ).toString();

    {
        QSet<QString> expected;
        expected << "artist1" << "artist2";
        QCOMPARE( artists, expected );
    }

    delete model;
    delete coll;
}

void
TestSingleCollectionTreeItemModel::testRemoveArtist()
{
    Collections::CollectionTestImpl *coll = new Collections::CollectionTestImpl( "test" );
    addMockTrack( coll, "track1", "artist1", "album1" );
    addMockTrack( coll, "track2", "artist2", "album2" );

    QList<CategoryId::CatMenuId> levels;
    levels<< CategoryId::Artist << CategoryId::Album;

    SingleCollectionTreeItemModel *model = new SingleCollectionTreeItemModel( coll, levels );

    loadChildren( model, QModelIndex() );

    QCOMPARE( model->rowCount( QModelIndex() ), 2 );

    {
        QSet<QString> artists;

        QModelIndex idx1 = model->index( 0, 0, QModelIndex() );
        artists << model->data( idx1, Qt::DisplayRole ).toString();

        QModelIndex idx2 = model->index( 1, 0, QModelIndex() );
        artists << model->data( idx2, Qt::DisplayRole ).toString();

        QSet<QString> expected;
        expected << "artist1" << "artist2";
        QCOMPARE( artists, expected );
    }

    ArtistMap map = coll->mc->artistMap();
    map.remove( "artist2" );  //album and track are still part of the collection
    coll->mc->setArtistMap( map );

    model->slotFilter();

    loadChildren( model, QModelIndex() );

    QCOMPARE( model->rowCount( QModelIndex() ), 1 );

    {
        QModelIndex artist1Index = model->index( 0, 0, QModelIndex() );
        QCOMPARE( model->data( artist1Index, Qt::DisplayRole ).toString(), QString( "artist1" ) );
    }

    delete model;
    delete coll;
}

void
TestSingleCollectionTreeItemModel::testAddTrack()
{
    Collections::CollectionTestImpl *coll = new Collections::CollectionTestImpl( "test" );
    addMockTrack( coll, "track1", "artist1", "album1" );
    addMockTrack( coll, "track2", "artist2", "album2" );

    QList<CategoryId::CatMenuId> levels;
    levels<< CategoryId::Artist << CategoryId::Album;

    SingleCollectionTreeItemModel *model = new SingleCollectionTreeItemModel( coll, levels );

    loadChildren( model, QModelIndex() );

    QCOMPARE( model->rowCount( QModelIndex() ), 2 );

    {
        QSet<QString> artists;

        QModelIndex idx1 = model->index( 0, 0, QModelIndex() );
        artists << model->data( idx1, Qt::DisplayRole ).toString();

        QModelIndex idx2 = model->index( 1, 0, QModelIndex() );
        artists << model->data( idx2, Qt::DisplayRole ).toString();

        QSet<QString> expected;
        expected << "artist1" << "artist2";
        QCOMPARE( artists, expected );
    }

    for( int i = 0; i < 2; i++ )
    {
        QModelIndex parent = model->index( i, 0, QModelIndex() );
        loadChildren( model, parent );
        QCOMPARE( model->rowCount( parent ), 1 );

        QModelIndex subParent = model->index( 0, 0, parent );
        loadChildren( model, subParent );
        QCOMPARE( model->rowCount( subParent ), 1 );
    }

    addMockTrack( coll, "track3", "artist1", "album1" );

    model->slotFilter();

    QTest::qWait( 30 );

    loadChildren( model, QModelIndex() );

    QCOMPARE( model->rowCount( QModelIndex() ), 2 );

    for( int i = 0; i < 2; i++ )
    {
        QModelIndex parent = model->index( i, 0, QModelIndex() );
        loadChildren( model, parent );
        QCOMPARE( model->rowCount( parent ), 1 );

        QString name = model->data( parent, Qt::DisplayRole ).toString();
        int count = (name == "artist1" ? 2 : 1 );

        QModelIndex subParent = model->index( 0, 0, parent );
        loadChildren( model, subParent );
        QCOMPARE( model->rowCount( subParent ), count );
    }

    delete model;
    delete coll;
}

void
TestSingleCollectionTreeItemModel::testRemoveTrack()
{

}

void
TestSingleCollectionTreeItemModel::testAddTrackWithFilter()
{
    Collections::CollectionTestImpl *coll = new Collections::CollectionTestImpl( "test" );
    addMockTrack( coll, "track1", "artist1", "album1" );

    QList<CategoryId::CatMenuId> levels;
    levels << CategoryId::Artist << CategoryId::Album;

    SingleCollectionTreeItemModel *model = new SingleCollectionTreeItemModel( coll, levels );

    loadChildren( model, QModelIndex() );
    QCOMPARE( model->rowCount( QModelIndex() ), 1 );

    {
        QModelIndex artist1Index = model->index( 0, 0, QModelIndex() );
        QCOMPARE( model->data( artist1Index, Qt::DisplayRole ).toString(), QString( "artist1" ) );
    }

    addMockTrack( coll, "track2", "artist2", "album2" );
    model->setCurrentFilter( "track2" );
    model->slotFilter();
    loadChildren( model, QModelIndex() );
    QCOMPARE( model->rowCount( QModelIndex() ), 1 );

    model->setCurrentFilter( QString() );
    model->slotFilter();
    loadChildren( model, QModelIndex() );
    QCOMPARE( model->rowCount( QModelIndex() ), 2 );

    QModelIndex idx1 = model->index( 0, 0, QModelIndex() );
    QCOMPARE( model->data( idx1, Qt::DisplayRole ).toString(), QString( "artist2" ) );

    delete model;
    delete coll;
}
