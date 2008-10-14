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

QByteArray.prototype.toString = function() 
{
    ts = new QTextStream( this, QIODevice.ReadOnly );
    return ts.readAll();
}

function Librivox()
{

    var currentDir = Amarok.Info.scriptPath();
    currentDir = currentDir.slice(0, -7)

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


function onConfigure()
{
    Amarok.alert( "sorry", "This script does not require any configuration." );
}


function bookFetchResult( reply )
{

    try
    {
    
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

            titles[i] = elt2.text();

            elt2 = elt.firstChildElement( "url" );
            links[i] = elt2.text();


        }


        for( i = 0; i < bookElements.length(); i++ )
        {

            title = titles[i]
            link = links[i];

            item = Amarok.StreamItem;
            item.level = 1;
            item.callbackData = link;
            item.itemName = title;
            item.playableUrl = "";
            item.infoHtml = html;

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

        htmlPage = result;

        //remove all <em> and </em> as they screw up simple parsing if present ( basicaly be cause on some pages they are there and on some they are not
        //in a way that is difficult to take into account in a regexp )
        htmlPage = htmlPage.replace( "<em>", "" );
        htmlPage = htmlPage.replace( "</em>", "" );

        //Amarok.debug( " Got reply from librivox: " +  html );

        //get the book description. Unfortunately this will not work currently, as tracks do not have a description, and there is no easy way to apply this to the book level.
        //In the long term, I think we should add a "get info" callback instead of having to pass html info for each object at creation 
        //descriptionRx = new RegExp( "<blockquote>(.*)<\\/blockquote>" );
        //description = htmlPage.match( descriptionRx );

        //Apparently we cannot bot do multiple matches and multiple capture groups as well in qt-script, so use the same regexp twice, one for getting each book, and once for getting
        //book, and once for getting the two parts of the book element that we are interested in, the title and the url.
        rx = new RegExp( "<li>([^\\n]*)<br\\s\\/>\\s*\\n[^\\n]*\\n?[^\\n]*\\n[^\\n]*\\n[^\\n]*href=\\\"([\\.a-zA-Z0-9_:\\/]*\\.ogg)\\\">ogg\\svorbis", "g" );
        list = htmlPage.match( rx );



        rx2 = new RegExp( "<li>([^\\n]*)<br\\s\\/>\\s*\\n[^\\n]*\\n?[^\\n]*\\n[^\\n]*\\n[^\\n]*href=\\\"([\\.a-zA-Z0-9_:\\/]*\\.ogg)\\\">ogg\\svorbis" );
        for ( i = 0; i < list.length; i++ )

        {
            list2 = list[i].match( rx2 );
            title = list2[1];
            url = list2[2];
            item = Amarok.StreamItem;
            item.level = 0;
            item.callbackData = "";
            item.itemName = title;
            item.playableUrl = url;
            item.infoHtml = html;

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
        name = filter.replace( "%20", " " );
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
        item.infoHtml = html;
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


http = new QHttp;
data = new QBuffer;
doc = new QDomDocument("doc");
elt = new QDomElement;
elt2 = new QDomElement;
bookElements = new QDomNodeList;
html = "";

Amarok.configured.connect( onConfigure );
script = new Librivox();
script.populate.connect( onPopulate );
