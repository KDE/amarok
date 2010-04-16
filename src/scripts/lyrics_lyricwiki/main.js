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

/* GLOBAL VARIABLES */
// template for the xml object that will be populated and passed to Amarok.Lyrics.showLyrics()
XML = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><lyric artist=\"{artist}\" title=\"{title}\">{lyrics}</lyric>";
// if we change variable xml it will not reinitialized on next lyrics request, so we will get lyrics from previous song
// because of that we need temp variable
NEWXML = "";
// maximum numbers that we can follow by #REDIRECT [[Band:Song]]
MAXREDIRECTS = 3;
// url to get lyrics using mediawiki API
APIURL = "http://lyrics.wikia.com/api.php?action=query&prop=revisions&rvprop=content&format=xml&titles=";
// urlified artist and title will be here after initialization
ARTIST = "";
TITLE  = "";
// the error message that is displayed if no lyrics were found or there was an error while trying to fetch them
ERRORMSG = "Lyrics not found. Sorry.";



/* receives a Wiki page (in XML format) that contains url to lyric of the requested song
   this API function can correct our tags
   for example we trying to receive lyrics for Nightwish:Nightwish_-_Sleepwalker (incorrect tags)
   this API functions will redirect us to Nightwish:Sleepwalker (correct tags)
*/
function onHelpReceived( response )
{
    try
    {
        if( response.length == 0 )
            Amarok.Lyrics.showLyricsError( ERRORMSG );
        else
        {
            var doc = new QDomDocument();
            doc.setContent(response);
              
            var urlstr = doc.elementsByTagName( "url" ).at( 0 ).toElement().text();
            var capture;
                 
            if(capture = /.+\/([^?=:]+:[^?=:]+)$/.exec(urlstr))
            {
                  // matched url is like this one: http://lyrics.wikia.com/Nightwish:Sleepwalker
                  // but not like this: http://lyrics.wikia.com/index.php?title=Nightwish:Sleepwalker&action=edit
                      
                  var url = QUrl.fromEncoded( new QByteArray( APIURL + capture[1] ), 1);
                                                                                       // this zero will not allow to execute this function again
                  new Downloader( url, new Function("response", "onLyricsReceived(response, 0)") );
            }
            else
            {
                  Amarok.Lyrics.showLyricsNotFound( ERRORMSG );
            }
        }
    }
    catch( err )
    {
        Amarok.Lyrics.showLyricsError( ERRORMSG );
        Amarok.debug( "script error in function onHelpReceived: " + err );
    }
}

/* receives a Wiki page (in XML format) using wikimedia API and extracts lyrics from it */
function onLyricsReceived( response, redirects )
{
    try
    {
        if( response.length == 0 )
            Amarok.Lyrics.showLyricsError( "Unable to contact server - no website returned" ); // TODO: this should be i18n able
        else
        {
            var doc = new QDomDocument();
            doc.setContent(response);
            
            var capture;
            response = doc.elementsByTagName( "rev" ).at( 0 ).toElement().text();
            
            if(capture = /<(lyrics?>)/i.exec(response))
            { // ok, lyrics found
                // lyrycs can be between <lyrics></lyrics> or <lyric><lyric> tags
                // such variant can be in one response: <lyrics>national lyrics</lyrics> <lyrics>english lyrics</lyrics>
                // example: http://lyrics.wikia.com/api.php?action=query&prop=revisions&titles=Flёur:Колыбельная_для_Солнца&rvprop=content&format=xml
                // we can not use lazy regexp because qt script don't understand it
                // so let's extract lyrics with string functions
                
                var lindex = response.indexOf("<" + capture[1]) + capture[1].length + 1;
                var rindex = response.indexOf("</" + capture[1]);
                NEWXML = NEWXML.replace( "{lyrics}", Amarok.Lyrics.escape( response.substring(lindex, rindex) ) );
                Amarok.Lyrics.showLyrics( NEWXML );
            }
            else if(capture = /#redirect\s+\[\[(.+)\]\]/i.exec(response))
            { // redirect pragma found: #REDIRECT [[Band:Song]]
                redirects++;
                if(redirects == MAXREDIRECTS)
                { // redirection limit exceed
                    Amarok.Lyrics.showLyricsNotFound( ERRORMSG );
                    return;
                }
                
                var url = QUrl.fromEncoded( new QByteArray( APIURL + encodeURIComponent( capture[1] ) ), 1);
                new Downloader( url, new Function("response", "onLyricsReceived(response, " + redirects + ")") );
            }
            else if(redirects < 0)
            { // if we get here after redirect than something go wrong, so checks that redirects < 0
                // maybe lyricwiki can help us
                var urlstr = "http://lyrics.wikia.com/api.php?action=lyrics&func=getSong&fmt=xml&artist=" + ARTIST + "&song=" + TITLE;
                var url = QUrl.fromEncoded( new QByteArray( urlstr ), 1 );
                new Downloader( url, onHelpReceived );
            }
            else
            {
                Amarok.Lyrics.showLyricsNotFound( ERRORMSG );
            }
        }
    }
    catch( err )
    {
        Amarok.Lyrics.showLyricsError( ERRORMSG );
        Amarok.debug( "script error in function onLyricsReceived: " + err );
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
        Amarok.debug ( "script error in function URLify: " + err );
    } 
}

// convert all HTML entities to their applicable characters
function entityDecode(string)
{
    try
    {
        var convertxml = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><body><entity>" + string + "</entity></body>";
        var doc = new QDomDocument();
        if(doc.setContent(convertxml))
        { // xml is valid
            return doc.elementsByTagName( "entity" ).at( 0 ).toElement().text();
        }
        
        return string;
    }
    catch( err )
    {
        Amarok.debug( "script error in function entityDecode: " + err );
    }
}

// entry point
function getLyrics( artist, title, url )
{
    try
    {
        // save artist and title for later display now
        NEWXML = XML.replace( "{artist}", Amarok.Lyrics.escape( artist ) );
        NEWXML = NEWXML.replace( "{title}", Amarok.Lyrics.escape( title ) );
        
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
        
        // URLify artist and title
        ARTIST = artist = URLify( entityDecode(artist) );
        TITLE  = title  = URLify( entityDecode(title) );

        // assemble the (encoded!) URL, build a QUrl out of it and dispatch the download request
        var url = QUrl.fromEncoded( new QByteArray( APIURL + artist + ":" + title ), 1);
        Amarok.debug( "request URL: " + url.toString() );
                                                                          // there was no redirections yet
        new Downloader( url, new Function("response", "onLyricsReceived(response, -1)") );
    }
    catch( err )
    {
        Amarok.debug( "error: " + err );
    }
}


Amarok.Lyrics.fetchLyrics.connect( getLyrics );
