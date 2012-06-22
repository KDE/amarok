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

#include <qtest_kde.h>

#include <QString>

// so that PlaylistFormat can be used in QTest::addColumn()
Q_DECLARE_METATYPE( Playlists::PlaylistFormat )

QTEST_KDEMAIN_CORE( TestPlaylistFormat )

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
    KUrl url( "amarok:///playlists/" );

    url.setFileName( filename );
    QCOMPARE( Playlists::getFormat( url ), playlistFormat );
    // file extensions in capitals must also pass this test
    url.setFileName( filename.toUpper() );
    QCOMPARE( Playlists::getFormat( url ), playlistFormat );
}

void
TestPlaylistFormat::testIsPlaylist()
{
    KUrl url( "amarok:///playlists/" );

    // These formats are supported
    QString filename = "playlist.m3u";
    url.setFileName( filename );
    QCOMPARE( Playlists::isPlaylist( url ), true );

    filename = "playlist.m3u8";
    url.setFileName( filename );
    QCOMPARE( Playlists::isPlaylist( url ), true );

    filename = "playlist.pls";
    url.setFileName( filename );
    QCOMPARE( Playlists::isPlaylist( url ), true );

    filename = "playlist.ram";
    url.setFileName( filename );
    QCOMPARE( Playlists::isPlaylist( url ), true );

    filename = "playlist.smil";
    url.setFileName( filename );
    QCOMPARE( Playlists::isPlaylist( url ), true );

    filename = "playlist.asx";
    url.setFileName( filename );
    QCOMPARE( Playlists::isPlaylist( url ), true );

    filename = "playlist.wax";
    url.setFileName( filename );
    QCOMPARE( Playlists::isPlaylist( url ), true );

    filename = "playlist.xml";
    url.setFileName( filename );
    QCOMPARE( Playlists::isPlaylist( url ), true );

    filename = "playlist.xspf";
    url.setFileName( filename );
    QCOMPARE( Playlists::isPlaylist( url ), true );

    // File extensions in capitals must also pass this test
    filename = "playlist.M3U";
    url.setFileName( filename );
    QCOMPARE( Playlists::isPlaylist( url ), true );

    // These shouldn't be supported
    filename = "playlist.vlc";
    url.setFileName( filename );
    QCOMPARE( Playlists::isPlaylist( url ), false );

    filename = "playlist.invalidformat";
    url.setFileName( filename );
    QCOMPARE( Playlists::isPlaylist( url ), false );

    filename = "this.is.an.invalid.format";
    url.setFileName( filename );
    QCOMPARE( Playlists::isPlaylist( url ), false );

    filename = "this.is.an.invalid.format.with.trailing.dot.";
    url.setFileName( filename );
    QCOMPARE( Playlists::isPlaylist( url ), false );

    filename = "NoDots";
    url.setFileName( filename );
    QCOMPARE( Playlists::isPlaylist( url ), false );

    filename = "";
    url.setFileName( filename );
    QCOMPARE( Playlists::isPlaylist( url ), false );
}
