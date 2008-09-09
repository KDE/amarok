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

function Librivox()
{

    print ("creating service...");
    ScriptableServiceScript.call( this, "Librivox.org", 3, "Search for books from Librivox", "Librivox service script", true );
    print ("done creating service!");
}


function onConfigure()
{
    Amarok.alert( "sorry", "This script does not require any configuration." );
}


function bookFetchResult( reply ) {

    try{
    
        print( " Got reply from librivox: " + reply );

        doc.setContent( reply );

        bookElements = doc.elementsByTagName( "book" );

        print ("got " + bookElements.length() + " books!");

        var titles = new Array( bookElements.length() );
        var links = new Array( bookElements.length() );


        var i = 0;
        for ( ; i < bookElements.length(); i++ ) {



            print ( i );

            elt = bookElements.at( i );
            print ( "got element" );

            elt2 = elt.firstChildElement( "title" );
            print ( "got title element" );

            titles[i] = elt2.text();
            print( " Got title: " +  elt2.text() );

            elt2 = elt.firstChildElement( "url" );
            links[i] = elt2.text();

            print ( "bottom of loop" );


        }

        print( "processed " + ( i +1 ) + " books" );

        for( i = 0; i < bookElements.length(); i++ ) {

            title = titles[i]
            link = links[i];

            item = Amarok.StreamItem;
            item.level = 1;
            item.callbackData = link;
            item.itemName = title;
            item.playableUrl = "";
            item.infoHtml = "";

            script.insertItem( item );

        }

        print( "all done here!");

    }
    catch( err )
    {
        print( err );
    }

    script.donePopulating();



}

function episodeFetchResult( result ) {

    try{

        //HAAAAAAAAAAAAAAAACK!!  How the hell do you get the string out of this result otherwise?

        html = Amarok.Lyrics.toUtf8( result, "ISO 8859-1" );

        //remove all <em> and </em> as they screw up simple parsing if present
        html = html.replace( "<em>", "" );
        html = html.replace( "</em>", "" );

        print( " Got reply from librivox: " +  html );

        //rx = new RegExp("<li>(.*?)<br\\s\\/>\\n.*\\n.*\\n.*href=\\\"(.*?\\.ogg)\\\">ogg\\svorbis/)");
        //rx = new RegExp("href=\\\"([\\.a-zA-Z0-9_:\\/]*\\.ogg)\\\">ogg\\svorbis");
        rx = new RegExp("<li>([^\\n]*)<br\\s\\/>\\s*\\n[^\\n]*\\n?[^\\n]*\\n[^\\n]*\\n[^\\n]*href=\\\"([\\.a-zA-Z0-9_:\\/]*\\.ogg)\\\">ogg\\svorbis", "g");
        //list = rx.exec( html );
        list = html.match( rx );

        print ( "--------------------------------------------" );




        rx2 = new RegExp("<li>([^\\n]*)<br\\s\\/>\\s*\\n[^\\n]*\\n?[^\\n]*\\n[^\\n]*\\n[^\\n]*href=\\\"([\\.a-zA-Z0-9_:\\/]*\\.ogg)\\\">ogg\\svorbis" );
        for ( i = 0; i < list.length; i++ ) {
            list2 = list[i].match( rx2 );
            title = list2[1];
            url = list2[2];

            //print ( title + " " + url );

            item = Amarok.StreamItem;
            item.level = 0;
            item.callbackData = "";
            item.itemName = title;
            item.playableUrl = url;
            item.infoHtml = "";

            script.insertItem( item );
        }


    }
    catch( err )
    {
        print( err );
    }

    script.donePopulating();

}

function onPopulate( level, callback, filter )
{
    print ("Librivox 4");
    offset = 0;

    if ( filter != "" )
    {
        name = filter.replace( "%20", " " );
    }
    else
    {
        name = "Enter Query..."
    }

    print ("Librivox 5");

    if ( level == 2 ) {
        print( " Populating main level..." );
        html = "The results of your query for: " + filter;
        if ( offset > 0 )
            name = name + " ( " + offset + " - " + (offset + 100) + " )";

        print ("Librivox 6");
        item = Amarok.StreamItem;
        item.level = 2;
        item.callbackData = "dummy";
        item.itemName = name;
        item.playableUrl = "";
        item.infoHtml = html;

        print ("Librivox 7");

        script.insertItem( item );

        print ("Librivox 8");

        script.donePopulating();

    } else if ( level == 1 )
    {
        print( " Populating book level..." );

        try{

            path = "http://librivox.org/newcatalog/search_xml.php?simple=" + filter;
            qurl = new QUrl( path );
            a = new Downloader( qurl, bookFetchResult );
        }
        catch( err )
        {
            print( err );
        }

    }
    else if ( level == 0 )
    {
        print( " Populating episode level..." );
        print( " url: " +  callback );

        try{

            path = callback;
            qurl = new QUrl( path );
            b = new Downloader( qurl, episodeFetchResult );
        }
        catch( err )
        {
            print( err );
        }

    }
}




print ("Librivox 0");
http = new QHttp;
data = new QBuffer;
doc = new QDomDocument("doc");
elt = new QDomElement;
elt2 = new QDomElement;
bookElements = new QDomNodeList;

print ("Librivox 1");
Amarok.configured.connect( onConfigure );
print ("Librivox 2");
script = new Librivox();
print ("Librivox 2.1");
script.populate.connect( onPopulate );
print ("Librivox 2.2");