/**************************************************************************
*   Amarok 2 lyrics script to fetch Chinese lyrics from mp3.sogou.com     *
*                                                                         *
*   Copyright                                                             *
*   (C) 2008 Peter ZHOU  <peterzhoulei@gmail.com>                         *
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
**************************************************************************/

Importer.loadQtBinding( "qt.core" );
Importer.loadQtBinding( "qt.network" );
Importer.loadQtBinding( "qt.xml" );

function parseLyrics( lyrics )
{
    print( "parsing..." );

    lyrics = lyrics.replace( /<tr><td rowspan="2" style="line-height:21px"><font class=mr>/, "" );

    TrackInfo = Amarok.Engine.currentTrack();
    // html -> plaintext:
    lyrics = lyrics.replace( /<[Bb][Rr][^>]*>/g, "\n" );
    lyrics = lyrics.replace( /<.*>/g, "" ); // erase everything after the lyric
    lyricsStr = lyrics.replace( /\n\n[\n]+/g, "\n" );

    xml = xml.replace( "{artist}", TrackInfo.artist );
    xml = xml.replace( "{title}", TrackInfo.title );
    xml = xml.replace( "{lyrics}", lyricsStr );
    //print( xml );

    Amarok.Lyrics.showLyrics( xml );
} 

function lyricsFetchResult( reply )
{
    lyrics = Amarok.Lyrics.toUtf8( reply.readAll(), "GB2312" );

    lyrics = lyrics.replace( /<[iI][mM][gG][^>]*>/g, "");
    lyrics = lyrics.replace( /<[aA][^>]*>[^<]*<\/[aA]>/g, "" );
    lyrics = lyrics.replace( /<[sS][cC][rR][iI][pP][tT][^>]*>[^<]*(<!--[^>]*>)*[^<]*<\/[sS][cC][rR][iI][pP][tT]>/g, "" );
    
    print( "searching..." );
    lyricsPos = lyrics.search( /<tr><td rowspan="2" style="line-height:21px"><font class=mr>*/ );

    if( lyricsPos > 0 )
    {
        print( "found lyrics at pos " + lyricsPos );
        parseLyrics( lyrics.slice( lyricsPos ) );
    }
}

function fetchLyrics( artist, title )
{
    xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><lyric artist=\"{artist}\" title=\"{title}\">{lyrics}</lyric>"
    connection = new QNetworkAccessManager();
    var path = "http://mp3.sogou.com/gecisearch.so?query";
    encodedTitle = Amarok.Lyrics.fromUtf8( "query=" + title + "+" + artist, "GB2312" );
    connection.finished.connect( lyricsFetchResult );
    url = new QUrl( path );
    url.setEncodedQuery( encodedTitle );
    print( url.toString() );
    connection.get( new QNetworkRequest( url ) );
}

Amarok.Lyrics.fetchLyrics.connect( fetchLyrics );