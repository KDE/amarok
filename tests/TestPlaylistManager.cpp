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

#include "../src/playlistmanager/PlaylistManager.h"

#include <KStandardDirs>

#include <QtTest>


class TestPlaylistManager : public QObject
{
    Q_OBJECT
    QString testPlaylistPath;

public:
    TestPlaylistManager( int argc, char ** argv, char *fileNamePtr )
    {
        strcpy ( fileNamePtr, "PlaylistManager.log\0" ); // HACK: max 100 chars!
        QTest::qExec ( this, argc, argv );
    }

private slots:
    void initTestCase()
    {
        testPlaylistPath = KStandardDirs::installPath( "data" ) + "/amarok/testdata/playlists/";
    }

    void testGetFormat()
    {
        QCOMPARE( PlaylistManager::instance()->getFormat( testPlaylistPath + "test.asx" ), PlaylistManager::ASX );
        QCOMPARE( PlaylistManager::instance()->getFormat( testPlaylistPath + "test.m3u" ), PlaylistManager::M3U );
        QCOMPARE( PlaylistManager::instance()->getFormat( testPlaylistPath + "test.pls" ), PlaylistManager::PLS );
        QCOMPARE( PlaylistManager::instance()->getFormat( testPlaylistPath + "test.ram" ), PlaylistManager::RAM );
        QCOMPARE( PlaylistManager::instance()->getFormat( testPlaylistPath + "test.smil" ), PlaylistManager::SMIL );
        // TODO: PlaylistManager::XML <- what kind of playlist format is that? example?
        QCOMPARE( PlaylistManager::instance()->getFormat( testPlaylistPath + "test.xspf" ), PlaylistManager::XSPF );
        QCOMPARE( PlaylistManager::instance()->getFormat( testPlaylistPath + "no-playlist.png" ), PlaylistManager::Unknown );
        QCOMPARE( PlaylistManager::instance()->getFormat( testPlaylistPath + "no-playlist.png" ), PlaylistManager::NotPlaylist );
    }

    void testIsPlaylist()
    {
        QCOMPARE( PlaylistManager::instance()->isPlaylist( testPlaylistPath + "test.asx" ), true );
        QCOMPARE( PlaylistManager::instance()->isPlaylist( testPlaylistPath + "test.m3u" ), true );
        QCOMPARE( PlaylistManager::instance()->isPlaylist( testPlaylistPath + "test.pls" ), true );
        QCOMPARE( PlaylistManager::instance()->isPlaylist( testPlaylistPath + "test.ram" ), true );
        QCOMPARE( PlaylistManager::instance()->isPlaylist( testPlaylistPath + "test.smil" ), true );
        // TODO: PlaylistManager::XML <- what kind of playlist format is that? example?
        QCOMPARE( PlaylistManager::instance()->isPlaylist( testPlaylistPath + "test.xspf" ), true );
        QCOMPARE( PlaylistManager::instance()->isPlaylist( testPlaylistPath + "no-playlist.png" ), false );
    }
};

#include "../tests/TestPlaylistManager.moc"
