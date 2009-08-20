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

#include <KStandardDirs>

#include "collection/CollectionManager.h"

#include "TestPlaylistFileProvider.h"

TestPlaylistFileProvider::TestPlaylistFileProvider( QStringList testArgumentList )
{
    testArgumentList.replace( 2, testArgumentList.at( 2 ) + "PlaylistFileProvider.xml" );
    QTest::qExec( this, testArgumentList );
}


void TestPlaylistFileProvider::testPlaylists()
{
    Meta::PlaylistList tempList = m_testPlaylistFileProvider.playlists();
    QCOMPARE( tempList.size(), 0 );
}

void TestPlaylistFileProvider::testSave()
{
    Meta::TrackList tempTrackList;
    KUrl trackUrl;
    trackUrl = KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/audio/Platz 01.mp3" );
    tempTrackList.append( CollectionManager::instance()->trackForUrl( trackUrl ) );

    if( QFile::exists( Amarok::saveLocation( "playlists" ) + "Amarok Test Playlist.m3u" ) )
        QFile::remove( Amarok::saveLocation( "playlists" ) + "Amarok Test Playlist.m3u" );

    Meta::PlaylistPtr testPlaylist = m_testPlaylistFileProvider.save( tempTrackList, "Amarok Test Playlist.m3u" );

    QCOMPARE( testPlaylist->name(), QString( "Amarok Test Playlist.m3u" ) );
    QCOMPARE( testPlaylist->tracks().size(), 1 );
    QVERIFY( QFile::exists( Amarok::saveLocation( "playlists" ) + "Amarok Test Playlist.m3u" ) );
}

void TestPlaylistFileProvider::testImportAndDeletePlaylists()
{
    QVERIFY( m_testPlaylistFileProvider.import( KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/playlists/test.m3u" ) ) );

    Meta::PlaylistList tempList = m_testPlaylistFileProvider.playlists();
    QCOMPARE( tempList.size(), 1 ); // iow: use it with a clean profile

    m_testPlaylistFileProvider.deletePlaylists( tempList );
    tempList = m_testPlaylistFileProvider.playlists();
    QCOMPARE( tempList.size(), 0 );
}

void TestPlaylistFileProvider::testRename()
{
    QVERIFY( m_testPlaylistFileProvider.import( KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/playlists/test.m3u" ) ) );

    Meta::PlaylistList tempList = m_testPlaylistFileProvider.playlists();
    QCOMPARE( tempList.size(), 1 );

    m_testPlaylistFileProvider.rename( tempList.at( 0 ), "New Test Name" );
    tempList = m_testPlaylistFileProvider.playlists();
    QCOMPARE( tempList.at( 0 )->name(), QString( "New Test Name" ) );

    m_testPlaylistFileProvider.deletePlaylists( tempList );
}
