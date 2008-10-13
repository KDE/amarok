/**************************************************************************
*   Amarok 2 lyrics script to fetch lyrics from lyricwiki.org             *
*                                                                         *
*   Copyright                                                             *
*   (C) 2008 Aaron Reichman <reldruh@gmail.com>                           *
*   (C) 2008 Leo Franchi <lfranchi@kde.org>                               *
*   (C) 2008 Mark Kretschmann <kretschmann@kde.org>
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
Importer.loadQtBinding( "qt.gui" );
Importer.loadQtBinding( "qt.network" );


function onFinished( dat )
{
    try
    {
        //Amarok.debug( "got result: " + dat );
        dat = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><lyric artist=\"artist name\" title=\"song title\" page_url=\"http://lyricwiki.org\">" + dat + "</lyric>"
        Amarok.Lyrics.showLyricsHtml( dat );
    }
    catch( err )
    {
        Amarok.debug( "error: " + err );
    }
}

function getLyrics( artist, title, url )
{
    try
    {
        var encodedArtistKey = Amarok.Lyrics.fromUtf8( "artist", "ISO 8859-1" );
        var encodedArtist = Amarok.Lyrics.fromUtf8( artist, "ISO 8859-1" );
        var encodedTitleKey = Amarok.Lyrics.fromUtf8( "song", "ISO 8859-1" );
        var encodedTitle = Amarok.Lyrics.fromUtf8( title, "ISO 8859-1" );
       
        var url = new QUrl( "http://lyricwiki.org/api.php" );
        url.addQueryItem( "func", "getSong" );
        url.addEncodedQueryItem( encodedArtistKey, encodedArtist ); 
        url.addEncodedQueryItem( encodedTitleKey, encodedTitle );
        Amarok.debug( "Using request URL: " + url.toString() );

        new Downloader( url, onFinished );
    }
    catch( err )
    {
        Amarok.debug( "error: " + err );
    }
}


Amarok.Lyrics.fetchLyrics.connect( getLyrics );

