/**************************************************************************
*   Amarok 2 lyrics script to fetch lyrics from lyrics.wikia.com          *
*   (formerly lyricwiki.org)                                              *
*                                                                         *
*   Copyright                                                             *
*   (C) 2008 Aaron Reichman <reldruh@gmail.com>                           *
*   (C) 2008 Leo Franchi <lfranchi@kde.org>                               *
*   (C) 2008 Mark Kretschmann <kretschmann@kde.org>                       *
*   (C) 2008 Peter ZHOU <peterzhoulei@gmail.org>                          *
*   (C) 2009 Jakob Kummerow <jakob.kummerow@gmail.com>                    *
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
Importer.loadQtBinding( "qt.xml" );

// template for the xml object needed to convert special characters in artist/title
convertxml = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><body><artist>{artist}</artist><song>{title}</song></body>";
newconvertxml = "";
// template for the xml object that will be populated and passed to Amarok.Lyrics.showLyrics()
xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><lyric artist=\"{artist}\" title=\"{title}\">{lyrics}</lyric>";
newxml = "";
// information about the last artist and title that we tried to get lyrics for
triedArtist = "";
triedSong = "";
retryNumber = 0;
// the error message that is displayed if no lyrics were found or there was an error while trying to fetch them
errormsg = "Lyrics not found. Sorry.";

/* receives an XML response from the API call and constructs a new request out of it */
function onFinishedAPI( response )
{
    try
    {
        if( response.length == 0 )
            Amarok.Lyrics.showLyricsError( errormsg );
        else
        {
            // construct a QDomDocument out of the response and extract the <url>...</url> part
            doc = new QDomDocument();
            doc.setContent( response );
            Amarok.debug( "returned wiki URL: " + doc.elementsByTagName( "url" ).at( 0 ).toElement().text());
            var url = doc.elementsByTagName( "url" ).at( 0 ).toElement().text();
            var url2 = QUrl.fromEncoded( new QByteArray( url ), 1 );
            Amarok.debug( "request no. 3 URL: " + url2.toString() );
            // access the URL, let the response be handled by onFinished
            new Downloader( url2, onFinished );
        }
    }
    catch( err )
    {
        Amarok.Lyrics.showLyricsError( errormsg );
        Amarok.debug( "script error in stage 2: " + err );
    }
}

/* receives a Wiki page (in HTML format) and extracts lyrics from it */
function onFinished( response )
{
    try
    {
        if( response.length == 0 )
            Amarok.Lyrics.showLyricsError( "Unable to contact server - no website returned" ); // TODO: this should be i18n able
        else
        {
            //Amarok.debug( "response: " + response );
            response = response.replace(/[\n\r]/g, ""); // remove all line breaks
            // if lyrics for this song don't exist, try something else
            if ( response.indexOf( "Click here to start this page!" ) != -1 ) {
                if ( retryNumber == 0 ) {
                    // try again using the re-born API :)
                    var urlstring = "http://lyrics.wikia.com/api.php?action=lyrics&func=getSong&fmt=xml&artist=" + triedArtist + "&song=" + triedSong;
                    retryNumber = 1;
                    var url = QUrl.fromEncoded( new QByteArray( urlstring ), 1 );
                    Amarok.debug( "request no. 2 URL: " + url.toString() );
                    // since the result will be XML stuff, we need onFinishedAPI to handle it
                    new Downloader( url, onFinishedAPI );
                    return;
                }
                // despite second attempt no lyrics were found. print an error message. 
                Amarok.Lyrics.showLyricsError( errormsg );
                Amarok.debug( "No lyrics found for artist=" + triedArtist + ", song=" + triedSong );
                return;
            }
            // parse the relevant part of the html source of the returned page
            relevant = /<div[^<>]*['"]lyricbox['"][^<>]*>(.*)<\/div>/.exec(response)[1];
            // take care of a few special cases
            relevant = relevant.replace(/<br\s*\/?>/g, "\n") + "\n\n"; // convert <br> to \n
            relevant = relevant.replace( /&mdash;/g, "—" ); // not supported by QDomDocument
            // construct a QDomDocument to convert special characters in the lyrics text. 
            doc2 = new QDomDocument();
            doc2.setContent( "<?xml version=\"1.0\" encoding=\"UTF-8\"?><lyrics>" + relevant + "</lyrics>" );
            var lyr = doc2.elementsByTagName( "lyrics" ).at( 0 ).toElement().text();
            // finally display the lyrics
            newxml = newxml.replace( "{lyrics}", Amarok.Lyrics.escape( lyr ) );
            Amarok.Lyrics.showLyrics( newxml );
        }
    }
    catch( err )
    {
        Amarok.Lyrics.showLyricsError( errormsg );
        Amarok.debug( "script error in stage 1: " + err );
    }
}

// build a URL component out of a string containing an artist or a song title
function URLify( string ) {
    try {
        // replace (erroneously used) accent ` with a proper apostrophe '
        string = string.replace( "`", "'" );
        // split into words, then treat each word separately
        var words = string.split( " " );
        for ( var i = 0; i < words.length; i++ ) {
            var upper = 1; // normally, convert first character only to uppercase, but:
            // if we have a Roman numeral (well, at least either of "ii", "iii"), convert all "i"s
            if ( words[i].charAt(0).toUpperCase() == "I" ) {
                // count "i" letters 
                while ( words[i].length > upper && words[i].charAt(upper).toUpperCase() == "I" ) {
                    upper++;
                }
            }
            // if the word starts with an apostrophe or parenthesis, the next character has to be uppercase
            if ( words[i].charAt(0) == "'" || words[i].charAt(0) == "(" ) {
                upper++;
            }
            // finally, perform the capitalization
            if ( upper < words[i].length ) {
                words[i] = words[i].substring( 0, upper ).toUpperCase() + words[i].substring( upper );
            } else {
                words[i] = words[i].toUpperCase();
            }
            // now take care of more special cases
            // names like "McSomething"
            if ( words[i].substring( 0, 2 ) == "Mc" ) {
                words[i] = "Mc" + words[i][2].toUpperCase() + words[i].substring( 3 );
            }
            // URI-encode the word
            words[i] = encodeURIComponent( words[i] );
        } 
        // join the words back together and return the result
        var result = words.join( "_" );
        return result;
    } catch ( err ) {
        Amarok.debug ( "lyrics-URLify-error: " + err );
    } 
} 

function getLyrics( artist, title, url )
{
    try
    {
        // save artist and title for later display now
        newxml = xml.replace( "{artist}", Amarok.Lyrics.escape( artist ) );
        newxml = newxml.replace( "{title}", Amarok.Lyrics.escape( title ) );
        // strip "featuring <someone else>" from the artist
        var strip = artist.toLowerCase().indexOf( " ft. ");
        if ( strip != -1 ) {
            artist = artist.substring( 0, strip );
        }
        strip = artist.toLowerCase().indexOf( " feat. " );
        if ( strip != -1 ) {
            artist = artist.substring( 0, strip );
        }
        strip = artist.toLowerCase().indexOf( " featuring " );
        if ( strip != -1 ) {
            artist = artist.substring( 0, strip );
        }
        // for the web request, convert special characters with the help of a temporary DomDocument
        newconvertxml = convertxml.replace( "{artist}", artist );
        newconvertxml = newconvertxml.replace( "{title}", title );
        var doc = new QDomDocument();
        doc.setContent( newconvertxml );
        // URLify artist and title and save them for later retries
        var artist2 = URLify( doc.elementsByTagName( "artist" ).at( 0 ).toElement().text() );
        var title2 = URLify( doc.elementsByTagName( "song" ).at( 0 ).toElement().text() );
        triedArtist = artist2;
        triedSong = title2;
        retryNumber = 0;
        // assemble the (encoded!) URL, build a QUrl out of it and dispatch the download request
        var urlstring = "http://lyrics.wikia.com/lyrics/" + artist2 + ":" + title2;
        var url = QUrl.fromEncoded( new QByteArray( urlstring ), 1);
        Amarok.debug( "request URL: " + url.toString() );

        new Downloader( url, onFinished );
    }
    catch( err )
    {
        Amarok.debug( "error: " + err );
    }
}


Amarok.Lyrics.fetchLyrics.connect( getLyrics );

