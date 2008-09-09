/**************************************************************************
*   Amarok 2 lyrics script to fetch lyrics from lyricwiki.org             *
*                                                                         *
*   Copyright                                                             *
*   (C) 2008 Aaron Reichman <reldruh@gmail.com>                           *
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


Importer.loadQtBinding("qt.core");
Importer.loadQtBinding("qt.gui");
Importer.loadQtBinding("qt.network");

function onFinished( dat )
{
    try
    {
        //Amarok.alert("reply.finished was emitted!");
        dat  = Amarok.Lyrics.toUtf8( dat, "ISO 8859-1" );
        dat = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><lyric artist=\"artist name\" title=\"song title\" page_url=\"http://lyricwiki.org\">" + dat + "</lyric>"
        //print( "got result: " + dat );
        Amarok.Lyrics.showLyricsHtml(dat);
    } catch( err )
    {
        print( "got error: " + err );
    }
}

function openconnection(artist, title)
{
    try
    {
        var url ="http://lyricwiki.org/api.php?func=getSong&artist=" + artist +"&song=" + title +"&fmt=html";

        //Amarok.alert( "fetching: " + (new QUrl( url )).toString() );
        new Downloader( url, onFinished );
    } catch( err )
    {
        print( "error!: " + err );
    }
}


function getLyrics(artist, title, url)
{
	//Amarok.alert("test");
	var currentartist = artist;
	var currenttitle = title;
	openconnection(currentartist, currenttitle);
}

Amarok.Lyrics.fetchLyrics.connect( getLyrics );