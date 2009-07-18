/*#########################################################################
#   Amarok script for interfacing with Librivox.org.                      #
#                                                                         #
#   Copyright                                                             #
#   (C) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>               #
#   (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                          #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         #
#########################################################################*/

Importer.loadQtBinding( "qt.core" );
Importer.loadQtBinding( "qt.xml" );
Importer.loadQtBinding( "qt.network" );
Importer.loadQtBinding( "qt.gui" ); //for QPixmap

QByteArray.prototype.toString = function() 
{
    ts = new QTextStream( this, QIODevice.ReadOnly );
    return ts.readAll();
}

function Librivox()
{

    var currentDir = Amarok.Info.scriptPath() + "/";

    var file = new QFile( currentDir + "LibrivoxService.html" );
    file.open( QIODevice.OpenMode( QIODevice.ReadOnly, QIODevice.Text ) );

    while ( !file.atEnd() ) {
        html += file.readLine().toString();
    }

    html = html.replace( "_IMAGE_DIR_", currentDir );

    Amarok.debug ("creating service...");
    //Amarok.debug ("html: " + html );
    ScriptableServiceScript.call( this, "Librivox.org", 3, "Search for books from Librivox", html, true );

    Amarok.debug ("done creating service!");
}

function onCustomize()
{
    Amarok.debug ("customizing Librivox service");
    var currentDir = Amarok.Info.scriptPath() + "/";
    Amarok.debug ( "loading icon: " + currentDir + "LibrivoxIcon.png" );
    var iconPixmap = new QPixmap( currentDir + "LibrivoxIcon.png" );
    script.setIcon( iconPixmap );

    var emblemPixmap = new QPixmap( currentDir + "LibrivoxEmblem.png" );
    script.setEmblem( emblemPixmap );
    script.setScalableEmblem( currentDir + "LibrivoxScalableEmblem.svgz" );
}

function bookFetchResult( reply )
{

    try
    {

        var cover = Amarok.Info.scriptPath() + "/audio_book128.png";
    
        doc.setContent( reply );

        bookElements = doc.elementsByTagName( "book" );

        Amarok.debug ("got " + bookElements.length() + " books!");

        var titles = new Array( bookElements.length() );
        var links = new Array( bookElements.length() );


        var i = 0;
        for ( ; i < bookElements.length(); i++ )
        {

            elt = bookElements.at( i );
            elt2 = elt.firstChildElement( "title" );

            var title = elt2.text();

            var rx = new RegExp( ".*\\(in\\s\\\"(.*)\\\"\\).*" );
            var list = title.match( rx );

            if ( list != null ) {
                Amarok.debug( "got a match: " + list[0] );
                title = list[1];
            }

            titles[i] = title;

            elt2 = elt.firstChildElement( "rssurl" );
            links[i] = elt2.text();

        }

        for( i = 0; i < bookElements.length(); i++ )
        {

            title = titles[i];
            link = links[i];

            item = Amarok.StreamItem;
            item.level = 1;
            item.callbackData = link;
            item.itemName = title;
            item.playableUrl = "";
            item.infoHtml = "";

            item.coverUrl = cover;

            script.insertItem( item );
        }

    }
    catch( err )
    {
        Amarok.debug( err );
    }

    script.donePopulating();

}

function episodeFetchResult( result )
{

    try
    {

        var cover = Amarok.Info.scriptPath() + "/audio_book128.png";
        doc.setContent( result );

        //get book and author title as these might not match the filter that was used...

        elt = doc.firstChildElement("rss");
        elt = elt.firstChildElement("channel");
        
        elt2 = elt.firstChildElement( "title" );
        var bookTitle = elt2.text();


        //give propper book titles for chpters in a compilation
        Amarok.debug( "Book title: " + bookTitle );
        var rx = new RegExp( ".*\\(in\\s\\\"(.*)\\\"\\).*" );
        var list = bookTitle.match( rx );

        if ( list != null ) {
            Amarok.debug( "got a match: " + list[0] );
            bookTitle = list[1];
        }

        var author = "Librivox.com";

        //process chapters
        chapterElements = doc.elementsByTagName( "item" );

        Amarok.debug( "got " + chapterElements.length() + " items..." );
        
        for ( var i = 0; i < chapterElements.length(); i++ )
        {
            
            elt = chapterElements.at( i );
            
            elt2 = elt.firstChildElement( "link" );
            var url = elt2.text();

            elt2 = elt.firstChildElement( "title" );
            var title = elt2.text();

            elt2 = elt.firstChildElement( "itunes:duration" );
            var duration = elt2.text();

            url = url.replace( "_64kb.mp3", ".ogg" );

            //lets see if we have a propper title, if not, create soething that does not look like crap
            var rx = new RegExp( "(\\d*)_" );
            var list = title.match( rx );

            if ( list != null )
                title = "Chapter " + list[1];


            item = Amarok.StreamItem;
            item.level = 0;
            item.callbackData = "";
            item.itemName = title;
            item.playableUrl = url;
            item.album = bookTitle;
            item.artist = author;
            item.coverUrl = cover;
            item.infoHtml = title;

            script.insertItem( item );

        }


    }
    catch( err )
    {
        Amarok.debug( err );
    }

    script.donePopulating();

}

function onPopulate( level, callback, filter )
{
    offset = 0;

    if ( filter != "" )
    {
        name = filter.replace( /%20/g, " " );
    }
    else
    {
        name = "Enter Query..."
    }

    if ( level == 2 ) {

        if ( offset > 0 )
            name = name + " ( " + offset + " - " + (offset + 100) + " )";

        item = Amarok.StreamItem;
        item.level = 2;
        item.callbackData = "dummy";
        item.itemName = name;
        item.playableUrl = "";
        item.infoHtml = "";
        script.insertItem( item );

        script.donePopulating();

    }
    else if ( level == 1 )
    {
        Amarok.debug( " Populating book level..." );

        try
        {

            path = "http://librivox.org/newcatalog/search_xml.php?simple=" + filter;
            qurl = new QUrl( path );
            a = new Downloader( qurl, bookFetchResult );
        }
        catch( err )
        {
            Amarok.debug( err );
        }

    }
    else if ( level == 0 )
    {
        Amarok.debug( " Populating episode level..." );
        Amarok.debug( " url: " +  callback );

        try{

            path = callback;
            qurl = new QUrl( path );
            b = new Downloader( qurl, episodeFetchResult );
        }
        catch( err )
        {
            Amarok.debug( err );
        }

    }
}

function onFetchInfo( level, callback ) {

    if ( level == 1 ) {
        qurl = new QUrl( callback );
        b = new Downloader( qurl, parseInfo );
    }

}

function parseInfo( result ) {

    Amarok.debug( result );

    rx = new RegExp( "<description>(.*)<\\/description>" );
    list = result.match( rx );
    info = list[1];

    info = info.replace( "<![CDATA[", "" );
    info = info.replace( "]]>", "" );

    script.setCurrentInfo( info );

}


http = new QHttp;
data = new QBuffer;
doc = new QDomDocument("doc");
elt = new QDomElement;
elt2 = new QDomElement;
bookElements = new QDomNodeList;
html = "";

script = new Librivox();
script.populate.connect( onPopulate );
script.customize.connect( onCustomize );
script.fetchInfo.connect( onFetchInfo );
