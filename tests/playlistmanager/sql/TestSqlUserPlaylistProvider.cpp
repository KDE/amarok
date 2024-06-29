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

#include "TestSqlUserPlaylistProvider.h"

#include "amarokconfig.h"
#include "EngineController.h"
#include "config-amarok-test.h"
#include "core/meta/Meta.h"
#include "core/support/Components.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "playlistmanager/sql/SqlUserPlaylistProvider.h"

#include <KLocalizedString>

#include <QTest>
#include <QDir>


QTEST_MAIN( TestSqlUserPlaylistProvider )

TestSqlUserPlaylistProvider::TestSqlUserPlaylistProvider()
{
    KLocalizedString::setApplicationDomain("amarok-test");
}

QString
TestSqlUserPlaylistProvider::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QStringLiteral( AMAROK_TEST_DIR ) + QLatin1Char('/') + relPath );
}

void TestSqlUserPlaylistProvider::initTestCase()
{
    AmarokConfig::instance(QStringLiteral("amarokrc"));
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
    QUrl trackUrl;
    trackUrl = QUrl::fromLocalFile(dataPath( QStringLiteral("data/audio/Platz 01.mp3") ));
    tempTrackList.append( CollectionManager::instance()->trackForUrl( trackUrl ) );

    Playlists::PlaylistPtr testPlaylist =
            m_testSqlUserPlaylistProvider->save( tempTrackList, "Amarok Test Playlist" );

    QCOMPARE( testPlaylist->name(), QStringLiteral( "Amarok Test Playlist" ) );
    QCOMPARE( testPlaylist->tracks().size(), 1 );

    Playlists::PlaylistList tempList = m_testSqlUserPlaylistProvider->playlists();
    QCOMPARE( tempList.size(), 1 );
}

void TestSqlUserPlaylistProvider::testRename()
{
    Playlists::PlaylistList tempList = m_testSqlUserPlaylistProvider->playlists();
    m_testSqlUserPlaylistProvider->renamePlaylist( tempList.at( 0 ), "New Test Name" );
    QCOMPARE( tempList.at( 0 )->name(), QStringLiteral( "New Test Name" ) );
}

void TestSqlUserPlaylistProvider::testDelete()
{
    Playlists::PlaylistList tempList = m_testSqlUserPlaylistProvider->playlists();
    m_testSqlUserPlaylistProvider->deletePlaylists( tempList );
    QCOMPARE( m_testSqlUserPlaylistProvider->playlists().size(), 0 );
}
