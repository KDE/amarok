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

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestPlaylistFormat )

TestPlaylistFormat::TestPlaylistFormat()
{
}

void
TestPlaylistFormat::testGetFormat()
{
    KUrl url( "amarok:///playlists/" );

    // Supported formats
    QString filename = "playlist.m3u";
    url.setFileName( filename );
    QCOMPARE( Playlists::getFormat( url ), Playlists::M3U );

    filename = "playlist.m3u8";
    url.setFileName( filename );
    QCOMPARE( Playlists::getFormat( url ), Playlists::M3U );

    filename = "playlist.pls";
    url.setFileName( filename );
    QCOMPARE( Playlists::getFormat( url ), Playlists::PLS );

    filename = "playlist.ram";
    url.setFileName( filename );
    QCOMPARE( Playlists::getFormat( url ), Playlists::RAM );

    filename = "playlist.smil";
    url.setFileName( filename );
    QCOMPARE( Playlists::getFormat( url ), Playlists::SMIL );

    filename = "playlist.asx";
    url.setFileName( filename );
    QCOMPARE( Playlists::getFormat( url ), Playlists::ASX );

    filename = "playlist.wax";
    url.setFileName( filename );
    QCOMPARE( Playlists::getFormat( url ), Playlists::ASX );

    filename = "playlist.xml";
    url.setFileName( filename );
    QCOMPARE( Playlists::getFormat( url ), Playlists::XML );

    filename = "playlist.xspf";
    url.setFileName( filename );
    QCOMPARE( Playlists::getFormat( url ), Playlists::XSPF );

    // File extensions in capitals must also pass this test
    filename = "playlist.M3U";
    url.setFileName( filename );
    QCOMPARE( Playlists::getFormat( url ), Playlists::M3U );

    // Unknown or invalid formats
    filename = "playlist.vlc";
    url.setFileName( filename );
    QCOMPARE( Playlists::getFormat( url ), Playlists::Unknown );

    filename = "playlist.invalidformat";
    url.setFileName( filename );
    QCOMPARE( Playlists::getFormat( url ), Playlists::NotPlaylist );

    filename = "this.is.an.invalid.format";
    url.setFileName( filename );
    QCOMPARE( Playlists::getFormat( url ), Playlists::NotPlaylist );

    filename = "this.is.an.invalid.format.with.trailing.dot.";
    url.setFileName( filename );
    QCOMPARE( Playlists::getFormat( url ), Playlists::NotPlaylist );

    filename = "NoDots";
    url.setFileName( filename );
    QCOMPARE( Playlists::getFormat( url ), Playlists::NotPlaylist );

    filename = "";
    url.setFileName( filename );
    QCOMPARE( Playlists::getFormat( url ), Playlists::NotPlaylist );
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

#include "TestPlaylistFormat.moc"
