/**************************************************************************
*   Amarok 2 lyrics script to fetch lyrics from lyricwiki.org             *
*                                                                         *
*   Copyright                                                             *
*   (C) 2008 Aaron Reichman <reldruh@gmail.com>                           *
*   (C) 2008 Leo Franchi <lfranchi@kde.org>                               *
*   (C) 2008 Mark Kretschmann <kretschmann@kde.org>                       *
*   (C) 2008 Peter ZHOU <peterzhoulei@gmail.org>                          *
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

xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><lyric artist=\"{artist}\" title=\"{title}\">{lyrics}</lyric>";
newxml = "";

function onFinished2( response )
{
    try
    {
        if( response.length == 0 )
            Amarok.Lyrics.showLyricsError( "Unable to contact server - no website returned" ); // TODO: this should be i18n able
        else
        {
            doc2 = new QDomDocument();
            doc2.setContent( response );
            textboxtext = doc2.elementsByTagName( "textarea" ).at( 0 ).toElement().text();
            lyr = /<lyrics>(.*)<\/lyrics>/.exec(textboxtext)[1];
            //Amarok.debug( "matched: " + lyr );
            newxml = newxml.replace( "{lyrics}", Amarok.Lyrics.escape( lyr ) );
            Amarok.Lyrics.showLyrics( newxml );
        }
    }
    catch( err )
    {
        Amarok.Lyrics.showLyricsError( "Could not retrieve lyrics: " + err );
        Amarok.debug( "error: " + err );
    }
}

function onFinished( dat )
{
    try
    {
        if( dat.length == 0 )
            Amarok.Lyrics.showLyricsError( "Unable to contact server" ); // TODO: this should be i18n able
        else
        {
            doc = new QDomDocument();
            doc.setContent( dat );
            newxml = xml.replace( "{artist}", Amarok.Lyrics.escape( doc.elementsByTagName( "artist" ).at( 0 ).toElement().text() ) );
            newxml = newxml.replace( "{title}", Amarok.Lyrics.escape( doc.elementsByTagName( "song" ).at( 0 ).toElement().text() ) );
            Amarok.debug( "returned real lyricwiki URL: " + doc.elementsByTagName( "url" ).at( 0 ).toElement().text());
            var url = decodeURI(doc.elementsByTagName( "url" ).at( 0 ).toElement().text());
            url = url.replace( /lyricwiki\.org\//, "lyricwiki.org/index.php?action=edit&title=" );
            var url2 = new QUrl(url);
            Amarok.debug( "request-2 URL: " + url2.toString() );
            new Downloader( url2, onFinished2 );
        }
    }
    catch( err )
    {
        Amarok.Lyrics.showLyricsError( "Could not retrieve lyrics: " + err );
        Amarok.debug( "error: " + err );
    }
}

function getLyrics( artist, title, url )
{
    try
    {
        var url = new QUrl( "http://lyricwiki.org/api.php" );
        url.addQueryItem( "func", "getSong" );
        url.addQueryItem( "artist", artist );
        url.addQueryItem( "song", title );
        url.addQueryItem( "fmt", "xml" );
        Amarok.debug( "request URL: " + url.toString() );

        new Downloader( url, onFinished );
    }
    catch( err )
    {
        Amarok.debug( "error: " + err );
    }
}


Amarok.Lyrics.fetchLyrics.connect( getLyrics );

