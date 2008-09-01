/**************************************************************************
*   Amarok 2 lyrics script to fetch lyrics from lyrc.com.ar               *
*                                                                         *
*   Copyright                                                             *
*   (C) 2008 Leo Franchi <lfranchi@kde.org>                               *
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

// HACK this is not very pretty, but i can't get the QDomDocument stuff to work
// so this is how it'll be for now
// HACK poor man's version of string substitutions
//xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><lyrics artist=\"{artist}\" title=\"{title}\" page_url=\"{page}\">{lyrics}</lyrics></xml>"
xml = "<lyric artist=\"{artist}\" title=\"{title}\">{lyrics}</lyric>"


QByteArray.prototype.toString = function()
{
  ts = new QTextStream( this, QIODevice.ReadOnly );
  return ts.readAll();
}

function parseLyrics( lyrics )
{
    print( "parsing!!!" );
    lyricsReady = lyrics;

    lyrics = lyrics.replace(  /<[fF][oO][nN][tT][^>]*>/g, "" );
    doc = new QDomDocument( );

    root = doc.createElement( "lyrics" );

    root.setAttribute( "page_url", page_url );
    titleStr = /(<b>)([^<]*)/.exec( lyrics )[ 2 ];
    print( "got title: " + titleStr );
    root.setAttribute( "title", titleStr );
    artistStr = /(<u>)([^<]*)/.exec( lyrics )[ 2 ];
    print( "got artist: " + artistStr );
    root.setAttribute( "artist", artistStr );

    try {
        lyrics = /(<\/u><\/font>)(.*)/.exec( lyrics )[ 2 ];
        // html -> plaintext:
        lyrics = lyrics.replace( /<[Bb][Rr][^>]*>/g, "\n" );
        lyrics = lyrics.replace( "\n\n", "" ).replace( "\r", "" );
        lyrics = lyrics.replace( "\n\n", "\n" ).replace( "\r", "" );
        lyrics = lyrics.replace( /<.*>/g, "" ); // erase everything after the lyric
        lyricsStr = lyrics.replace( /\n\n[\n]+/g, "\n" );
        //print( "got cleaned lyrics: " + lyrics );

        xml = xml.replace( "{artist}", artistStr );
        xml = xml.replace( "{title}", titleStr );
        xml = xml.replace( "{page}", page_url );
        xml = xml.replace( "{lyrics}", lyricsStr );
        text = doc.createTextNode( "lyricsText" );
        text.setData( lyricsStr );

        //xml = doc.toString();
        print( "xml: " + xml );
    } catch (err) {
        print( "error!: " + err );
    }
    
    //Amarok.alert( xml );
    Amarok.Lyrics.showLyrics( xml );
} 

function lyricsFetchResult( reply )
{
    //Amarok.alert( "lyrics slot called!" );
    print( "got result from lyrics fetch:" + reply );
    try {
//        print( "got size of result from lyrics fetch:" + reply.size() );
        lyrics = reply.readAll().toString();
    } catch( err )
    {
        print( "got error: " + err );
    }
    
    print( "result: " + lyrics );

    // no need, just complicates regexp
    lyrics.replace( "\n", "" );
    lyrics.replace( "\r", "" );

    // Remove images, links, scripts, styles, fonts and tables
    lyrics = lyrics.replace( /<[iI][mM][gG][^>]*>/g, "");
    lyrics = lyrics.replace( /<[aA][^>]*>[^<]*<\/[aA]>/g, "" );
    lyrics = lyrics.replace( /<[sS][cC][rR][iI][pP][tT][^>]*>[^<]*(<!--[^>]*>)*[^<]*<\/[sS][cC][rR][iI][pP][tT]>/g, "" );
    lyrics = lyrics.replace( /<[sS][tT][yY][lL][eE][^>]*>[^<]*(<!--[^>]*>)*[^<]*<\/[sS][tT][yY][lL][eE]>>/g, "");
    // remove leftover
    lyrics = lyrics.replace( /<table align="left"><tr><td>.*<\/td><\/tr><\/table>/g, "" );
    //print( "result: " + lyrics );
    
    lyricsPos = lyrics.search( /<[fF][oO][nN][tT][ ]*[sS][iI][zZ][eE][ ]*='2'[ ]*/ );
    if( lyricsPos > 0 )
    {
        print( "found lyrics at pos " + lyricsPos );
        parseLyrics( lyrics.slice( lyricsPos ) );
    } else if( lyrics.indexOf( "Suggestions" ) > 0 )
    {
        //Amarok.alert( "Suggestions not implemented yet" );
        // TODO parse suggestions
        print( "suggestions not implemented yet" );
    }

}


function fetchLyrics( artist, title )
{
    connection = new QNetworkAccessManager();
    var host = "lyrc.com.ar";
    var path;

    path = "/en/tema1en.php?artist=" + artist + "&songname=" + title;
    page_url = "http://" + host + path;

    // TODO for now, ignoring proxy settings
    //page_url = QUrl.toPercentEncoding( page_url )

    connection.finished.connect( lyricsFetchResult );

    //Amarok.alert( "GOT LYRICS REQUEST: " + page_url.toString() );

    connection.get( new QNetworkRequest( new QUrl( page_url ) ) );
}

Amarok.Lyrics.fetchLyrics.connect( fetchLyrics );
//fetchLyrics( "The Beatles", "Hey Jude" );