/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@asbest-online.de>               *
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

#include "amarokconfig.h"
#include "config-amarok-test.h"
#include "core/support/Amarok.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "playlistmanager/file/PlaylistFileProvider.h"

#include <QTest>
#include <QDir>
#include <QFile>

#include <KConfigGroup>
#include <KLocalizedString>

QTEST_MAIN( TestPlaylistFileProvider )

TestPlaylistFileProvider::TestPlaylistFileProvider()
{
    KLocalizedString::setApplicationDomain("amarok-test");
}

void TestPlaylistFileProvider::initTestCase()
{
    AmarokConfig::instance("amarokrc");
    m_testPlaylistFileProvider = new Playlists::PlaylistFileProvider();
    QVERIFY( m_testPlaylistFileProvider );

    removeConfigPlaylistEntries();
    removeTestPlaylist();
    m_testPlaylistFileProvider->deletePlaylists( m_testPlaylistFileProvider->playlists() );
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

void TestPlaylistFileProvider::testSave_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("result");

    QTest::newRow("no extension, should default to xspf") << "Amarok Test Playlist" << "Amarok Test Playlist.xspf";
    QTest::newRow("directory separators '\\' and '/' in file name are being replaced by '-'") << "amarok/playlist" << "amarok-playlist.xspf";
    QTest::newRow("directory separators '\\' and '/' in file name are being replaced by '-'") << "amarok\\playlist" << "amarok-playlist.xspf";
    QTest::newRow("xspf") << "Amarok Test Playlist.xspf" << "Amarok Test Playlist.xspf";
    QTest::newRow("m3u") << "Amarok Test Playlist.m3u" << "Amarok Test Playlist.m3u";
    QTest::newRow("pls") << "Amarok Test Playlist.pls" << "Amarok Test Playlist.pls";
    QTest::newRow("playlist name with dot in it (BR 290318)") << "Amarok Test Playlist. With dots." << "Amarok Test Playlist. With dots..xspf";
}

void TestPlaylistFileProvider::testSave()
{
    Meta::TrackList tempTrackList;
    const QUrl trackUrl = QUrl::fromLocalFile(dataPath( "data/audio/Platz 01.mp3" ));
    tempTrackList.append( CollectionManager::instance()->trackForUrl( trackUrl ) );
    QCOMPARE( tempTrackList.size(), 1 );

    QFETCH(QString, name);
    QFETCH(QString, result);

    Playlists::PlaylistPtr testPlaylist = m_testPlaylistFileProvider->save( tempTrackList, name );
    QVERIFY( testPlaylist );

    QVERIFY( QFile::exists( Amarok::saveLocation( "playlists" ) + result ) );
    QCOMPARE( testPlaylist->name(), QString( result ) );
    QCOMPARE( testPlaylist->tracks().size(), 1 );
    QFile::remove( Amarok::saveLocation( "playlists" ) + result );
}

void TestPlaylistFileProvider::testImportAndDeletePlaylists()
{
    m_testPlaylistFileProvider->deletePlaylists( m_testPlaylistFileProvider->playlists() );
    Playlists::PlaylistList tempList = m_testPlaylistFileProvider->playlists();
    QCOMPARE( tempList.size(), 0 );

    QVERIFY( QFile::exists( dataPath( "data/playlists/test.m3u" ) ) );
    QFile::copy( dataPath( "data/playlists/test.m3u" ), QDir::tempPath() + "/test.m3u" );
    QVERIFY( QFile::exists( QDir::tempPath() + "/test.m3u" ) );

    QVERIFY( m_testPlaylistFileProvider->import( QUrl::fromLocalFile( QDir::tempPath() + "/test.m3u") ) );
    tempList = m_testPlaylistFileProvider->playlists();
    QCOMPARE( tempList.size(), 1 );

    m_testPlaylistFileProvider->deletePlaylists( tempList );
    tempList = m_testPlaylistFileProvider->playlists();
    QCOMPARE( tempList.size(), 0 );
}

void TestPlaylistFileProvider::testRename()
{
    m_testPlaylistFileProvider->deletePlaylists( m_testPlaylistFileProvider->playlists() );
    Playlists::PlaylistList tempList = m_testPlaylistFileProvider->playlists();
    QCOMPARE( tempList.size(), 0 );

    QVERIFY( QFile::exists( dataPath( "data/playlists/test.m3u" ) ) );
    QFile::copy( dataPath( "data/playlists/test.m3u" ), QDir::tempPath() + "/test.m3u" );
    QVERIFY( QFile::exists( QDir::tempPath() + "/test.m3u" ) );

    QVERIFY( m_testPlaylistFileProvider->import( QUrl::fromLocalFile( QDir::tempPath() + "/test.m3u") ) );
    tempList = m_testPlaylistFileProvider->playlists();
    QCOMPARE( tempList.size(), 1 );

    m_testPlaylistFileProvider->renamePlaylist( tempList.at( 0 ), "New Test Name" );
    tempList = m_testPlaylistFileProvider->playlists();
    QCOMPARE( tempList.at( 0 )->name(), QString( "New Test Name.m3u" ) );

    m_testPlaylistFileProvider->deletePlaylists( tempList );
    tempList = m_testPlaylistFileProvider->playlists();
    QCOMPARE( tempList.size(), 0 );
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
    KConfigGroup config = Amarok::config( "Loaded Playlist Files" );
    config.deleteGroup();
}
