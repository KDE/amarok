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

#include "TestPlaylistFileSupport.h"
#include "config-amarok-test.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "core/playlists/PlaylistFormat.h"

#include <QTest>
#include <QDir>


QTEST_GUILESS_MAIN( TestPlaylistFileSupport )

TestPlaylistFileSupport::TestPlaylistFileSupport()
{}

void TestPlaylistFileSupport::initTestCase()
{
    const QDir dir( QDir::toNativeSeparators( QStringLiteral( AMAROK_TEST_DIR ) + QStringLiteral("/data/playlists") ) );
    QVERIFY( dir.exists() );
    m_testPlaylistPath = dir.absolutePath();
}

void TestPlaylistFileSupport::testGetFormat()
{
    QCOMPARE( Playlists::getFormat( QUrl::fromLocalFile(m_testPlaylistPath + QStringLiteral("test.asx")) ), Playlists::ASX );
    QCOMPARE( Playlists::getFormat( QUrl::fromLocalFile(m_testPlaylistPath + QStringLiteral("test.m3u")) ), Playlists::M3U );
    QCOMPARE( Playlists::getFormat( QUrl::fromLocalFile(m_testPlaylistPath + QStringLiteral("test.pls")) ), Playlists::PLS );
    QCOMPARE( Playlists::getFormat( QUrl::fromLocalFile(m_testPlaylistPath + QStringLiteral("test.ram")) ), Playlists::RAM );
    QCOMPARE( Playlists::getFormat( QUrl::fromLocalFile(m_testPlaylistPath + QStringLiteral("test.smil")) ), Playlists::SMIL );
    // TODO: Playlists::XML <- what kind of playlist format is that? example?
    QCOMPARE( Playlists::getFormat( QUrl::fromLocalFile(m_testPlaylistPath + QStringLiteral("test.xspf")) ), Playlists::XSPF );
    QCOMPARE( Playlists::getFormat( QUrl::fromLocalFile(m_testPlaylistPath + QStringLiteral("no-playlist.png")) ), Playlists::Unknown );
    QCOMPARE( Playlists::getFormat( QUrl::fromLocalFile(m_testPlaylistPath + QStringLiteral("no-playlist.png")) ), Playlists::NotPlaylist );
}

void TestPlaylistFileSupport::testIsPlaylist()
{
    QCOMPARE( Playlists::isPlaylist( QUrl::fromLocalFile(m_testPlaylistPath + QStringLiteral("test.asx")) ), true );
    QCOMPARE( Playlists::isPlaylist( QUrl::fromLocalFile(m_testPlaylistPath + QStringLiteral("test.m3u")) ), true );
    QCOMPARE( Playlists::isPlaylist( QUrl::fromLocalFile(m_testPlaylistPath + QStringLiteral("test.pls")) ), true );
    QCOMPARE( Playlists::isPlaylist( QUrl::fromLocalFile(m_testPlaylistPath + QStringLiteral("test.ram")) ), true );
    QCOMPARE( Playlists::isPlaylist( QUrl::fromLocalFile(m_testPlaylistPath + QStringLiteral("test.smil")) ), true );
    // TODO: Playlists::XML <- what kind of playlist format is that? example?
    QCOMPARE( Playlists::isPlaylist( QUrl::fromLocalFile(m_testPlaylistPath + QStringLiteral("test.xspf")) ), true );
    QCOMPARE( Playlists::isPlaylist( QUrl::fromLocalFile(m_testPlaylistPath + QStringLiteral("no-playlist.png")) ), false );
}

