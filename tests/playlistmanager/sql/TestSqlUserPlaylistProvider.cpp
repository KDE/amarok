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
#include "collection/CollectionManager.h"

#include <KStandardDirs>

#include <QtTest/QTest>
#include <QtCore/QDir>

TestSqlUserPlaylistProvider::TestSqlUserPlaylistProvider( QStringList testArgumentList )
{
    testArgumentList.replace( 2, testArgumentList.at( 2 ) + "SqlUserPlaylistProvider.xml" );
    QTest::qExec( this, testArgumentList );
}


void TestSqlUserPlaylistProvider::testPlaylists()
{
    Meta::PlaylistList tempList = m_testSqlUserPlaylistProvider.playlists();
    QCOMPARE( tempList.size(), 0 );
}

void TestSqlUserPlaylistProvider::testSave()
{
    Meta::TrackList tempTrackList;
    KUrl trackUrl;
    trackUrl = KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/audio/Platz 01.mp3" );
    tempTrackList.append( CollectionManager::instance()->trackForUrl( trackUrl ) );

    Meta::PlaylistPtr testPlaylist = m_testSqlUserPlaylistProvider.save( tempTrackList, "Amarok Test Playlist" );

    QCOMPARE( testPlaylist->name(), QString( "Amarok Test Playlist" ) );
    QCOMPARE( testPlaylist->tracks().size(), 1 );
}

void TestSqlUserPlaylistProvider::testImportAndDeletePlaylists()
{
    QVERIFY( m_testSqlUserPlaylistProvider.import( KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/playlists/test.m3u" ) ) );

    Meta::PlaylistList tempList = m_testSqlUserPlaylistProvider.playlists();
    QCOMPARE( tempList.size(), 1 ); // iow: use it with a clean profile

    m_testSqlUserPlaylistProvider.deletePlaylists( tempList );
    tempList = m_testSqlUserPlaylistProvider.playlists();
    QCOMPARE( tempList.size(), 0 );
}

void TestSqlUserPlaylistProvider::testRename()
{
    QVERIFY( m_testSqlUserPlaylistProvider.import( KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators( "amarok/testdata/playlists/test.m3u" ) ) );

    Meta::PlaylistList tempList = m_testSqlUserPlaylistProvider.playlists();
    QCOMPARE( tempList.size(), 1 );

    m_testSqlUserPlaylistProvider.rename( tempList.at( 0 ), "New Test Name" );
    tempList = m_testSqlUserPlaylistProvider.playlists();
    QCOMPARE( tempList.at( 0 )->name(), QString( "New Test Name" ) );

    m_testSqlUserPlaylistProvider.deletePlaylists( tempList );
}
