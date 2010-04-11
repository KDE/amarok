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

// template for the xml object that will be populated and passed to Amarok.Lyrics.showLyrics()
xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><lyric artist=\"{artist}\" title=\"{title}\">{lyrics}</lyric>";
// if we change variable xml it will not reinitialized on next lyrics request, so we will get lyrics from previous song
// because of that we need temp variable
newxml = "";
// maximum numbers that we can follow by #REDIRECT [[Band:Song]]
maxredirects = 3;
// url to get lyrics using mediawiki API
apiurl = "http://lyrics.wikia.com/api.php?action=query&prop=revisions&rvprop=content&format=xml&titles=";
// the error message that is displayed if no lyrics were found or there was an error while trying to fetch them
errormsg = "Lyrics not found. Sorry.";

/* receives a Wiki page (in XML format) using wikimedia API and extracts lyrics from it */
function onFinished( response, redirects )
{
    try
    {
        if( response.length == 0 )
            Amarok.Lyrics.showLyricsError( "Unable to contact server - no website returned" ); // TODO: this should be i18n able
        else
        {
            //Amarok.debug( "response: " + response );
            
            // to allow extract lyrics with xml pasrer
            response = response.replace('&lt;lyrics&gt;', '<lyrics>');
            response = response.replace('&lt;/lyrics&gt;', '</lyrics>');
            
            var doc = new QDomDocument();
            doc.setContent(response);
            
            if(response = doc.elementsByTagName( "lyrics" ).at( 0 ).toElement().text())
            { // ok, lyrics found
                  newxml = newxml.replace( "{lyrics}", Amarok.Lyrics.escape( response ) );
                  Amarok.Lyrics.showLyrics( newxml );
            }
            else if(response = doc.elementsByTagName( "rev" ).at( 0 ).toElement().text())
            {
                  var preg;
                  if(preg = /#redirect\s+\[\[(.+)\]\]/i.exec(response))
                  { // redirect pragma found: #REDIRECT [[Band:Song]]
                        redirects++;
                        if(redirects == maxredirects)
                        { // redirection limit exceed
                              Amarok.Lyrics.showLyricsNotFound( errormsg );
                              return;
                        }
                        var url = QUrl.fromEncoded( new QByteArray( apiurl + encodeURIComponent(preg[1]) ), 1);
                        new Downloader( url, new Function("response", "onFinished(response, " + redirects + ")") );
                  }
                  else
                  {
                        Amarok.Lyrics.showLyricsNotFound( errormsg );
                  }
            }
            else
            {
                  Amarok.Lyrics.showLyricsNotFound( errormsg );
            }
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

// convert all HTML entities to their applicable characters
function entityDecode(string)
{
      var convertxml = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><body><entity>" + string + "</entity></body>";
      var doc = new QDomDocument();
      if(doc.setContent(convertxml))
      { // xml is valid
            return doc.elementsByTagName( "entity" ).at( 0 ).toElement().text();
      }
      
      return string;
}

// entry point
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
        
        // URLify artist and title
        artist = URLify( entityDecode(artist) );
        title = URLify( entityDecode(title) );

        // assemble the (encoded!) URL, build a QUrl out of it and dispatch the download request
        var url = QUrl.fromEncoded( new QByteArray( apiurl + artist + ":" + title ), 1);
        Amarok.debug( "request URL: " + url.toString() );
                                                                          //there was no redirections yet
        new Downloader( url, new Function("response", "onFinished(response, -1)") );
    }
    catch( err )
    {
        Amarok.debug( "error: " + err );
    }
}


Amarok.Lyrics.fetchLyrics.connect( getLyrics );
