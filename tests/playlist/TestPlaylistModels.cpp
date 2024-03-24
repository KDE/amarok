/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.com>                                *
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

#include "TestPlaylistModels.h"

#include "amarokconfig.h"
#include "core/support/Components.h"
#include "EngineController.h"

#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModelStack.h"
#include "playlist/PlaylistModel.h"
#include "playlist/UndoCommands.h"

#include "mocks/MetaMock.h"
#include "mocks/MockTrack.h"

#include <KLocalizedString>

#include <QtDebug>
#include <QTest>


using namespace Playlist;

QTEST_GUILESS_MAIN( TestPlaylistModels )


TestPlaylistModels::TestPlaylistModels()
    : QObject()
{
    KLocalizedString::setApplicationDomain("amarok-test");
}

void TestPlaylistModels::initTestCase()
{
    AmarokConfig::instance("amarokrc");

    //apparently the engine controller is needed somewhere, or we will get a crash...
    EngineController *controller = new EngineController();
    Amarok::Components::setEngineController( controller );

    // Initialize playlistAction before we set the playlist, lest our playlist be overwritten with Art Of Nations
    The::playlistActions();
    The::playlistController()->removeRow( 0 );

    //we want to add a few tracks to the playlist so we can test sorting, filtering and so on. So first create a bunch of dummy tracks we can use.

    Meta::TrackList tracks;

    QVariantMap map1;
    map1.insert( Meta::Field::TITLE,  QString( "Cool as honey" ) );
    MetaMock * metaMock = new MetaMock( map1 );
    metaMock->m_artist = new MockArtist( "Bonzai Bees" );
    metaMock->m_album = new MockAlbum( "The Hive", metaMock->m_artist );
    tracks << Meta::TrackPtr( metaMock );

    QVariantMap map2;
    map2.insert( Meta::Field::TITLE,  QString( "xTreme buzzing sound" ) );
    metaMock = new MetaMock( map2 );
    metaMock->m_artist = new MockArtist( "Bonzai Bees" );
    metaMock->m_album = new MockAlbum( "The Hive", metaMock->m_artist );
    tracks << Meta::TrackPtr( metaMock );

    QVariantMap map3;
    map3.insert( Meta::Field::TITLE,  QString( "Alphabet soup" ) );
    metaMock = new MetaMock( map3 );
    metaMock->m_artist = new MockArtist( "Grumpy Grizzlies" );
    metaMock->m_album = new MockAlbum( "The Hive", metaMock->m_artist );
    tracks << Meta::TrackPtr( metaMock );

    QVariantMap map4;
    map4.insert( Meta::Field::TITLE,  QString( "Zlick" ) );
    metaMock = new MetaMock( map4 );
    metaMock->m_artist = new MockArtist( "Grumpy Grizzlies" );
    metaMock->m_album = new MockAlbum( "Nice Long Nap", metaMock->m_artist );
    tracks << Meta::TrackPtr( metaMock );

    QVariantMap map5;
    map5.insert( Meta::Field::TITLE,  QString( "23 hours is not enough" ) );
    metaMock = new MetaMock( map5 );
    metaMock->m_artist = new MockArtist( "Grumpy Grizzlies" );
    metaMock->m_album = new MockAlbum( "Nice Long Nap", metaMock->m_artist );
    tracks << Meta::TrackPtr( metaMock );

    QVariantMap map6;
    map6.insert( Meta::Field::TITLE,  QString( "1 song to rule them all" ) );
    metaMock = new MetaMock( map6 );
    metaMock->m_artist = new MockArtist( "Bonzai Bees" );
    metaMock->m_album = new MockAlbum( "Pretty Flowers", metaMock->m_artist );
    tracks << Meta::TrackPtr( metaMock );

    QVariantMap map7;
    map7.insert( Meta::Field::TITLE,  QString( "zz ambience sound" ) );
    metaMock = new MetaMock( map7 );
    // note: no artist, no album!
    tracks << Meta::TrackPtr( metaMock );

    The::playlistController()->insertTracks( 0, tracks );

    QCOMPARE( The::playlist()->trackAt( 3 )->prettyName(), QString( "Zlick" ) );
}

void TestPlaylistModels::cleanup()
{
    SortScheme scheme = SortScheme();
    ModelStack::instance()->sortProxy()->updateSortMap( scheme );
    ModelStack::instance()->filterProxy()->clearSearchTerm();
}

void TestPlaylistModels::testSorting()
{
    //simple sort by title
    //******************************//

    SortScheme scheme = SortScheme();
    scheme.addLevel( SortLevel( Playlist::Title, Qt::AscendingOrder ) );
    ModelStack::instance()->sortProxy()->updateSortMap( scheme );

    AbstractModel * topModel = The::playlist();

    QCOMPARE( topModel->trackAt( 0 )->prettyName(), QString( "1 song to rule them all" ) );
    QCOMPARE( topModel->trackAt( 1 )->prettyName(), QString( "23 hours is not enough" ) );
    QCOMPARE( topModel->trackAt( 2 )->prettyName(), QString( "Alphabet soup" ) );
    QCOMPARE( topModel->trackAt( 3 )->prettyName(), QString( "Cool as honey" ) );
    QCOMPARE( topModel->trackAt( 4 )->prettyName(), QString( "xTreme buzzing sound" ) );
    QCOMPARE( topModel->trackAt( 5 )->prettyName(), QString( "Zlick" ) );


    //Sort by Artist - Album - Title
    //******************************//

    SortScheme scheme2 = SortScheme();

    scheme2.addLevel( SortLevel( Playlist::Artist, Qt::AscendingOrder ) );
    scheme2.addLevel( SortLevel( Playlist::Album, Qt::AscendingOrder ) );
    scheme2.addLevel( SortLevel( Playlist::Title, Qt::AscendingOrder ) );

    QCOMPARE( scheme2.length(), 3 );

    ModelStack::instance()->sortProxy()->updateSortMap( scheme2 );
    topModel->qaim()->revert();

    // the one without artist or album comes first
    QCOMPARE( topModel->trackAt( 0 )->prettyName(), QString( "zz ambience sound" ) );
    QCOMPARE( topModel->trackAt( 1 )->prettyName(), QString( "1 song to rule them all" ) );
    QCOMPARE( topModel->trackAt( 2 )->prettyName(), QString( "Cool as honey" ) );
    QCOMPARE( topModel->trackAt( 3 )->prettyName(), QString( "xTreme buzzing sound" ) );
    QCOMPARE( topModel->trackAt( 4 )->prettyName(), QString( "23 hours is not enough" ) );
    QCOMPARE( topModel->trackAt( 5 )->prettyName(), QString( "Zlick" ) );
    QCOMPARE( topModel->trackAt( 6 )->prettyName(), QString( "Alphabet soup" ) );

    //reverse some stuff
    //******************************//

    SortScheme scheme3 = SortScheme();

    scheme3.addLevel( SortLevel( Playlist::Artist, Qt::AscendingOrder ) );
    scheme3.addLevel( SortLevel( Playlist::Album, Qt::DescendingOrder ) );
    scheme3.addLevel( SortLevel( Playlist::Title, Qt::AscendingOrder ) );

    ModelStack::instance()->sortProxy()->updateSortMap( scheme3 );
    topModel->qaim()->revert();

    QCOMPARE( topModel->trackAt( 0 )->prettyName(), QString( "zz ambience sound" ) );
    QCOMPARE( topModel->trackAt( 1 )->prettyName(), QString( "Cool as honey" ) );
    QCOMPARE( topModel->trackAt( 2 )->prettyName(), QString( "xTreme buzzing sound" ) );
    QCOMPARE( topModel->trackAt( 3 )->prettyName(), QString( "1 song to rule them all" ) );
    QCOMPARE( topModel->trackAt( 4 )->prettyName(), QString( "Alphabet soup" ) );
    QCOMPARE( topModel->trackAt( 5 )->prettyName(), QString( "23 hours is not enough" ) );
    QCOMPARE( topModel->trackAt( 6 )->prettyName(), QString( "Zlick" ) );

    //Sort by album when tracks have same album name and different artists
    //******************************//

    SortScheme scheme4 = SortScheme();

    scheme4.addLevel( SortLevel( Playlist::Album, Qt::AscendingOrder ) );
    scheme4.addLevel( SortLevel( Playlist::Title, Qt::AscendingOrder ) );

    ModelStack::instance()->sortProxy()->updateSortMap( scheme4 );
    topModel->qaim()->revert();

    QCOMPARE( topModel->trackAt( 0 )->prettyName(), QString( "zz ambience sound" ) );
    QCOMPARE( topModel->trackAt( 1 )->prettyName(), QString( "23 hours is not enough" ) );
    QCOMPARE( topModel->trackAt( 2 )->prettyName(), QString( "Zlick" ) );
    QCOMPARE( topModel->trackAt( 3 )->prettyName(), QString( "1 song to rule them all" ) );
    QCOMPARE( topModel->trackAt( 4 )->prettyName(), QString( "Cool as honey" ) );
    QCOMPARE( topModel->trackAt( 5 )->prettyName(), QString( "xTreme buzzing sound" ) );
    QCOMPARE( topModel->trackAt( 6 )->prettyName(), QString( "Alphabet soup" ) );
}

void TestPlaylistModels::testFiltering()
{
    ModelStack::instance()->filterProxy()->showOnlyMatches( true );
    ModelStack::instance()->filterProxy()->find( "ou" );
    ModelStack::instance()->filterProxy()->filterUpdated();

    AbstractModel * topModel = The::playlist();

    QCOMPARE( topModel->qaim()->rowCount(), 4 );
    QCOMPARE( topModel->trackAt( 0 )->prettyName(), QString( "xTreme buzzing sound" ) );
    QCOMPARE( topModel->trackAt( 1 )->prettyName(), QString( "Alphabet soup" ) );
    QCOMPARE( topModel->trackAt( 2 )->prettyName(), QString( "23 hours is not enough" ) );
    QCOMPARE( topModel->trackAt( 3 )->prettyName(), QString( "zz ambience sound" ) );
    //TODO: More advanced filtering tests go here

}

void TestPlaylistModels::testSearching()
{
}

void TestPlaylistModels::testShuffling()
{
    Meta::TrackList oldTrackList = The::playlist()->tracks();

    The::playlistActions()->shuffle();

    QVERIFY( oldTrackList != The::playlist()->tracks() );

    The::playlistController()->undo();

    QCOMPARE( oldTrackList, The::playlist()->tracks() );
}
