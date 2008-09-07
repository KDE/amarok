/**************************************************************************
*   Amarok 2 lyrics script to fetch lyrics from lyrc.com.ar               *
*                                                                         *
*   Copyright                                                             *
*   (C) 2008 Leo Franchi <lfranchi@kde.org>                               *
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
    //print( "parsing..." );

    var lyricsReady = lyrics;

    lyrics = lyrics.replace(  /<[fF][oO][nN][tT][^>]*>/g, "" );
    var doc = new QDomDocument( );

    var root = doc.createElement( "lyrics" );

    var titleStr = /(<b>)([^<]*)/.exec( lyrics )[ 2 ];
    print( "got title: " + titleStr );
    root.setAttribute( "title", titleStr );
    var artistStr = /(<u>)([^<]*)/.exec( lyrics )[ 2 ];
    //print( "got artist: " + artistStr );
    root.setAttribute( "artist", artistStr );

    try {
        lyrics = /(<\/u><\/font>)(.*)/.exec( lyrics )[ 2 ];
        // html -> plaintext:
        lyrics = lyrics.replace( /<[Bb][Rr][^>]*>/g, "\n" );
        lyrics = lyrics.replace( "\n\r\n\r", "" ).replace( "\n", "" );
        //lyrics = lyrics.replace( "\n\n", "\n" ).replace( "\r", "" );
        lyrics = lyrics.replace( /<.*>/g, "" ); // erase everything after the lyric
        var lyricsStr = lyrics.replace( /\n\n[\n]+/g, "\n" );
        //print( "got cleaned lyrics: " + lyrics );

        xml = xml.replace( "{artist}", artistStr );
        xml = xml.replace( "{title}", titleStr );
        xml = xml.replace( "{lyrics}", lyricsStr );
        var text = doc.createTextNode( "lyricsText" );
        text.setData( lyricsStr );

        //xml = doc.toString();
        //print( "xml: " + xml );
    } catch (err) {
        print( "error!: " + err );
    }

    Amarok.Lyrics.showLyrics( xml );
} 

function parseSuggestions( lyrics )
{
    print( "parsing suggestions!" );
    try
    {
        lyrics = lyrics.slice( lyrics.indexOf( "Suggestions : " ), lyrics.indexOf( "<br><br>" ) );

        lyrics = lyrics.replace( "<font color='white'>", "" );
        lyrics = lyrics.replace( "</font>", "" );
        lyrics = lyrics.replace( "<br /><br />", "" );

        //print( "got cleaned suggestions: " + lyrics );
        
        var suggestions = lyrics.split( "<br>" );

        suggestions_xml = suggestions_xml.replace( "{provider_url}", "" ); // empty for now
        var body_xml = ""
        for( i = 0; i < suggestions.length; i++ )
        {
            if( suggestions[ i ] == "" )
                continue;
            //print( "checking suggestion: " + suggestions[ i ] );
            if( ! /(<a href=")([^"]*)/.exec( suggestions[ i ] ) )
                continue;
            url =  /(<a href=")([^"]*)/.exec( suggestions[ i ] )[ 2 ]
            if( ! /<a href=.*>([^<]*)<\/a>/.exec( suggestions[ i ] ) )
                continue;
            var artist_title = artist_title = /<a href=.*>([^<]*)<\/a>/.exec( suggestions[ i ] )[ 1 ]

            var artist = artist_title.split( " - " )[ 0 ];
            var title = artist_title.split( " - " )[ 1 ];

            body_xml += suggestions_body.replace( "{artist}", artist ).replace( "{title}", title ).replace( "{url}", url );
            //print( "done checking suggestion: " + suggestions[ i ] );

        }

        suggestions_xml = suggestions_xml.replace( "{suggestions}", body_xml );
    } catch( err )
    {
        print( "got err in parsing suggestions: " )
        print( err );
    }
    //print( "got suggestions xml: " + suggestions_xml );
    Amarok.Lyrics.showLyrics( suggestions_xml );
}

function lyricsFetchResult( reply )
{
    print( "got result from lyrics fetch:" + reply ); 
    try
    {
        var lyrics = Amarok.Lyrics.toUtf8( reply.readAll(), "ISO 8859-1" );
    } catch( err )
    {
        print( "error converting lyrics: " + err );
    }
    //print( "result: " + lyrics );

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

    try {
        lyricsPos = lyrics.search( /<[fF][oO][nN][tT][ ]*[sS][iI][zZ][eE][ ]*='2'[ ]*/ );
        //print( "found lyrics at pos " + lyricsPos );
        //print( "found suggestions pos: " + lyrics.indexOf( "Suggestions" ) );
        if( lyricsPos > 0 )
        {
            parseLyrics( lyrics.slice( lyricsPos ) );
        } else if( lyrics.indexOf( "Suggestions" ) > 0 )
        {
            parseSuggestions( lyrics );
        }
    } catch( err ) {
        print( "got err: " + err );
    }
}


function fetchLyrics( artist, title, url )
{
    xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><lyric artist=\"{artist}\" title=\"{title}\">{lyrics}</lyric>"
    suggestions_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><suggestions page_url=\"{provider_url}\" >{suggestions}</suggestions>"
    suggestions_body="<suggestion artist=\"{artist}\" title=\"{title}\" url=\"{url}\" />"

    var connection = new QNetworkAccessManager();
    try{
        if( url == "" )
        {
            var path = "http://lyrc.com.ar/en/tema1en.php";
            encodedTitle = Amarok.Lyrics.fromUtf8( title, "ISO 8859-1" );
            encodedTitleKey = Amarok.Lyrics.fromUtf8( "songname", "ISO 8859-1" );
            encodedArtist = Amarok.Lyrics.fromUtf8( artist, "ISO 8859-1" )
            encodedArtistKey = Amarok.Lyrics.fromUtf8( "artist", "ISO 8859-1" );
            url = new QUrl( path );
            url.addEncodedQueryItem( encodedArtistKey, encodedArtist );
            url.addEncodedQueryItem( encodedTitleKey, encodedTitle );
            //print( "fetching from: " + url.toString() );
        } else
        {   // we are told to fetch a specific url
            var path = "http://lyrc.com.ar/en/" + url;
            url = new QUrl( path );
            //print( "fetching from given url: " + url.toString() );
        }
    }
    catch( err )
    {
        print( err );
    }
    // TODO for now, ignoring proxy settings
    //page_url = QUrl.toPercentEncoding( page_url )

    connection.finished.connect( lyricsFetchResult );
    connection.get( new QNetworkRequest( url ) );
}

Amarok.Lyrics.fetchLyrics.connect( fetchLyrics );