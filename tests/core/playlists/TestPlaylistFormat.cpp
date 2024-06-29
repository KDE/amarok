/****************************************************************************************
 * Copyright (c) 2012 Jasneet Singh Bhatti <jazneetbhatti@gmail.com>                    *
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

#include "TestPlaylistFormat.h"

#include "core/playlists/PlaylistFormat.h"

#include <QString>

// so that PlaylistFormat can be used in QTest::addColumn()
Q_DECLARE_METATYPE( Playlists::PlaylistFormat )

QTEST_GUILESS_MAIN( TestPlaylistFormat )

TestPlaylistFormat::TestPlaylistFormat()
{
}

void
TestPlaylistFormat::testGetFormat_data()
{
    QTest::addColumn<QString>( "filename" );
    QTest::addColumn<Playlists::PlaylistFormat>( "playlistFormat" );

    // valid formats
    QTest::newRow( "m3u" ) << "playlist.m3u" << Playlists::M3U;
    QTest::newRow( "m3u8" ) << "playlist.m3u8" << Playlists::M3U;
    QTest::newRow( "pls" ) << "playlist.pls" << Playlists::PLS;
    QTest::newRow( "ram" ) << "playlist.ram" << Playlists::RAM;
    QTest::newRow( "smil" ) << "playlist.smil" << Playlists::SMIL;
    QTest::newRow( "asx" ) << "playlist.asx" << Playlists::ASX;
    QTest::newRow( "wax" ) << "playlist.wax" << Playlists::ASX;
    QTest::newRow( "xml" ) << "playlist.xml" << Playlists::XML;
    QTest::newRow( "xspf" ) << "playlist.xspf" << Playlists::XSPF;

    // unknown or invalid formats
    QTest::newRow( "vlc" ) << "playlist.vlc" << Playlists::Unknown;
    QTest::newRow( "invalidformat" ) << "playlist.invalidformat" << Playlists::NotPlaylist;
    QTest::newRow( "dotted invalid" ) << "this.is.an.invalid.format" << Playlists::NotPlaylist;
    QTest::newRow( "trailing dot" ) << "this.is.an.invalid.format.with.trailing.dot." << Playlists::NotPlaylist;
    QTest::newRow( "no dot" ) << "NoDots" << Playlists::NotPlaylist;
    QTest::newRow( "empty string" ) << "" << Playlists::NotPlaylist;
}

void
TestPlaylistFormat::testGetFormat()
{
    QFETCH( QString, filename );
    QFETCH( Playlists::PlaylistFormat, playlistFormat );
    QUrl url( QStringLiteral("amarok:///playlists/") );

    url = url.adjusted(QUrl::RemoveFilename);
    url.setPath(url.path() +  filename );
    QCOMPARE( Playlists::getFormat( url ), playlistFormat );
    // file extensions in capitals must also pass this test
    url = url.adjusted(QUrl::RemoveFilename);
    url.setPath(url.path() +  filename.toUpper() );
    QCOMPARE( Playlists::getFormat( url ), playlistFormat );
}

void
TestPlaylistFormat::testIsPlaylist_data()
{
    QTest::addColumn<QString>( "filename" );
    QTest::addColumn<bool>( "isPlaylist" );

    // valid formats
    QTest::newRow( "m3u" ) << "playlist.m3u" << true;
    QTest::newRow( "m3u8" ) << "playlist.m3u8" << true;
    QTest::newRow( "pls" ) << "playlist.pls" << true;
    QTest::newRow( "ram" ) << "playlist.ram" << true;
    QTest::newRow( "smil" ) << "playlist.smil" << true;
    QTest::newRow( "asx" ) << "playlist.asx" << true;
    QTest::newRow( "wax" ) << "playlist.wax" << true;
    QTest::newRow( "xml" ) << "playlist.xml" << true;
    QTest::newRow( "xspf" ) << "playlist.xspf" << true;

    // unknown or invalid formats
    QTest::newRow( "vlc" ) << "playlist.vlc" << false;
    QTest::newRow( "invalidformat" ) << "playlist.invalidformat" << false;
    QTest::newRow( "dotted invalid" ) << "this.is.an.invalid.format" << false;
    QTest::newRow( "trailing dot" ) << "this.is.an.invalid.format.with.trailing.dot." << false;
    QTest::newRow( "no dot" ) << "NoDots" << false;
    QTest::newRow( "empty string" ) << "" << false;
}

void
TestPlaylistFormat::testIsPlaylist()
{
    QFETCH( QString, filename );
    QFETCH( bool, isPlaylist );
    QUrl url( QStringLiteral("amarok:///playlists/") );

    url = url.adjusted(QUrl::RemoveFilename);
    url.setPath(url.path() +  filename );
    QCOMPARE( Playlists::isPlaylist( url ), isPlaylist );
    // file extensions in capitals must also pass this test
    url = url.adjusted(QUrl::RemoveFilename);
    url.setPath(url.path() +  filename.toUpper() );
    QCOMPARE( Playlists::isPlaylist( url ), isPlaylist );
}
