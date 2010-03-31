/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@getamarok.com>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "TestSqlUserPlaylistProvider.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "config-amarok-test.h"
#include "playlistmanager/sql/SqlUserPlaylistProvider.h"
#include "core/support/Components.h"
#include "EngineController.h"

#include <QtTest/QTest>
#include <QtCore/QDir>

#include <qtest_kde.h>

QTEST_KDEMAIN( TestSqlUserPlaylistProvider, GUI )

TestSqlUserPlaylistProvider::TestSqlUserPlaylistProvider()
{}

QString
TestSqlUserPlaylistProvider::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + '/' + relPath );
}

void TestSqlUserPlaylistProvider::initTestCase()
{
    //apparently the engine controller is needed somewhere, or we will get a crash...
    EngineController *controller = new EngineController();
    Amarok::Components::setEngineController( controller );

    m_testSqlUserPlaylistProvider = new Playlists::SqlUserPlaylistProvider( true );
    m_testSqlUserPlaylistProvider->deletePlaylists( m_testSqlUserPlaylistProvider->playlists() );
}

void TestSqlUserPlaylistProvider::cleanupTestCase()
{
    delete m_testSqlUserPlaylistProvider;
}

void TestSqlUserPlaylistProvider::testPlaylists()
{
    Playlists::PlaylistList tempList = m_testSqlUserPlaylistProvider->playlists();
    QCOMPARE( tempList.size(), 0 );
}

void TestSqlUserPlaylistProvider::testSave()
{
    Meta::TrackList tempTrackList;
    KUrl trackUrl;
    trackUrl = dataPath( "data/audio/Platz 01.mp3" );
    tempTrackList.append( CollectionManager::instance()->trackForUrl( trackUrl ) );

    Playlists::PlaylistPtr testPlaylist = m_testSqlUserPlaylistProvider->save( tempTrackList, "Amarok Test Playlist" );

    QCOMPARE( testPlaylist->name(), QString( "Amarok Test Playlist" ) );
    QCOMPARE( testPlaylist->tracks().size(), 1 );
}

void TestSqlUserPlaylistProvider::testImportAndDeletePlaylists()
{
    QVERIFY( m_testSqlUserPlaylistProvider->import( dataPath( "data/playlists/test.xspf" ) ) );

    Playlists::PlaylistList tempList = m_testSqlUserPlaylistProvider->playlists();
    QCOMPARE( tempList.size(), 2 );

    QVERIFY( m_testSqlUserPlaylistProvider->import( dataPath( "data/playlists/test.pls" ) ) );
    QCOMPARE( m_testSqlUserPlaylistProvider->playlists().size(), 3 );

    m_testSqlUserPlaylistProvider->deletePlaylists( tempList );
    QCOMPARE( m_testSqlUserPlaylistProvider->playlists().size(), 1 );
}

void TestSqlUserPlaylistProvider::testRename()
{
    QVERIFY( m_testSqlUserPlaylistProvider->import( dataPath( "data/playlists/test.xspf" ) ) );

    Playlists::PlaylistList tempList = m_testSqlUserPlaylistProvider->playlists();
    QCOMPARE( tempList.size(), 2 );

    m_testSqlUserPlaylistProvider->rename( tempList.at( 0 ), "New Test Name" );
    tempList = m_testSqlUserPlaylistProvider->playlists();
    QCOMPARE( tempList.at( 0 )->name(), QString( "New Test Name" ) );

    m_testSqlUserPlaylistProvider->deletePlaylists( tempList );
    QCOMPARE( m_testSqlUserPlaylistProvider->playlists().size(), 0 );
}
