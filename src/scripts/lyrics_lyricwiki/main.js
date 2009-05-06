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
            newxml = xml.replace( "{artist}", doc.elementsByTagName( "artist" ).at( 0 ).toElement().text() );
            newxml = newxml.replace( "{title}", doc.elementsByTagName( "song" ).at( 0 ).toElement().text() );
            newxml = newxml.replace( "{lyrics}", doc.elementsByTagName( "lyrics" ).at( 0 ).toElement().text() );
            Amarok.Lyrics.showLyrics( Amarok.Lyrics.fromUtf8( newxml, "UTF-8" ) );
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

