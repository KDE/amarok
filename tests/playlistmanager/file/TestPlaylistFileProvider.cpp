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

#include "TestPlaylistFileProvider.h"
#include "core-implementations/collections/support/CollectionManager.h"
#include "config-amarok-test.h"
#include "playlistmanager/file/PlaylistFileProvider.h"

#include <KConfigGroup>

#include <QtTest/QTest>
#include <QtCore/QDir>
#include <QtCore/QFile>

#include <qtest_kde.h>

QTEST_KDEMAIN( TestPlaylistFileProvider, GUI )

TestPlaylistFileProvider::TestPlaylistFileProvider()
{}

void TestPlaylistFileProvider::initTestCase()
{
}

void TestPlaylistFileProvider::init()
{
    removeConfigPlaylistEntries();
    removeTestPlaylist();
}

void TestPlaylistFileProvider::cleanupTestCase()
{
    removeTestPlaylist();
    removeConfigPlaylistEntries();
    delete m_testPlaylistFileProvider;
}

void TestPlaylistFileProvider::testPlaylists()
{
    Playlists::PlaylistList tempList = m_testPlaylistFileProvider->playlists();
    QCOMPARE( tempList.size(), 0 );
}

void TestPlaylistFileProvider::testSave()
{
    Meta::TrackList tempTrackList;
    const KUrl trackUrl = dataPath( "data/audio/Platz 01.mp3" );
    tempTrackList.append( CollectionManager::instance()->trackForUrl( trackUrl ) );

    Playlists::PlaylistPtr testPlaylist = m_testPlaylistFileProvider->save( tempTrackList, "Amarok Test Playlist.m3u" );

    QCOMPARE( testPlaylist->name(), QString( "Amarok Test Playlist.m3u" ) );
    QCOMPARE( testPlaylist->tracks().size(), 1 );
    QVERIFY( QFile::exists( Amarok::saveLocation( "playlists" ) + "Amarok Test Playlist.m3u" ) );
}

void TestPlaylistFileProvider::testImportAndDeletePlaylists()
{
    QVERIFY( m_testPlaylistFileProvider->import( dataPath( "data/playlists/test.m3u" ) ) );

    Playlists::PlaylistList tempList = m_testPlaylistFileProvider->playlists();
    QCOMPARE( tempList.size(), 1 ); // iow: use it with a clean profile

    // FIXME: deleting playlist causes a confirm dialog to appear
    // m_testPlaylistFileProvider->deletePlaylists( tempList );
    // tempList = m_testPlaylistFileProvider->playlists();
    // QCOMPARE( tempList.size(), 0 );
}

void TestPlaylistFileProvider::testRename()
{
    QVERIFY( m_testPlaylistFileProvider->import( dataPath( "data/playlists/test.m3u" ) ) );

    Playlists::PlaylistList tempList = m_testPlaylistFileProvider->playlists();
    QCOMPARE( tempList.size(), 1 );

    m_testPlaylistFileProvider->rename( tempList.at( 0 ), "New Test Name" );
    tempList = m_testPlaylistFileProvider->playlists();
    QCOMPARE( tempList.at( 0 )->name(), QString( "New Test Name" ) );

    // FIXME: deleting playlist causes a confirm dialog to appear
    // m_testPlaylistFileProvider->deletePlaylists( tempList );
}

QString TestPlaylistFileProvider::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + '/' + relPath );
}

void TestPlaylistFileProvider::removeTestPlaylist()
{
    const QString m3u = Amarok::saveLocation( "playlists" ) + "Amarok Test Playlist.m3u";
    if( QFile::exists( m3u ) )
        QFile::remove( m3u );
}

void TestPlaylistFileProvider::removeConfigPlaylistEntries()
{
    m_testPlaylistFileProvider = new Playlists::PlaylistFileProvider();
    KConfigGroup config = Amarok::config( "Loaded Playlist Files" );
    config.deleteGroup();
}
