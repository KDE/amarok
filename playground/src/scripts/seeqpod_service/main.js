/*#########################################################################
#   Amarok script for interfacing with SeeqPod.com.                       #
#                                                                         #
#   Copyright                                                             #
#   (C) 2007, 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>         #
#   (C) 2008 Mark Kretschmann <kretschmann@kde.org>                       #
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
Importer.loadQtBinding( "qt.gui" );

QByteArray.prototype.toString = function() 
{
    ts = new QTextStream( this, QIODevice.ReadOnly );
    return ts.readAll();
}

function configure()
{
    print ("Seeqpod onConfigure");
    uid = "";


    uid = QInputDialog.getText( 0, "Seeqpod Script", "Please enter your uid", QLineEdit.Normal, "", 0 );

    if ( uid != "" ) {
        Amarok.debug ("got " + uid );
        Amarok.Script.writeConfig( "uid", uid );
    }
    
}

function Seeqpod()
{
   // onConfigure();
    print ("Seeqpod Seeqpod");
    
    var currentDir = Amarok.Info.scriptPath();
    print( "got current dir: " + currentDir );


    //get config file ( if it exists, otherwise, ask for uid )
    readConfig();



    var file = new QFile( currentDir + "/SeeqpodService.html" );
    file.open( QIODevice.OpenMode( QIODevice.ReadOnly, QIODevice.Text ) );

    while ( !file.atEnd() ) {
        html += file.readLine().toString();
    }

    Amarok.debug ("creating service...");
    Amarok.debug ("html: " + html );
    ScriptableServiceScript.call( this, "Seeqpod.com", 2, "Search and stream music from all over the web", html, true );
    Amarok.debug ("done creating service!");
}



function readConfig()
{
    print ("Seeqpod readConfig");

    uid = Amarok.Script.readConfig( "uid", "" );
    if ( uid == "" )
	configure();
}


function queryResult( reply )
{

    try
    {
    
        doc.setContent( reply );

        trackElements = doc.elementsByTagName( "track" );

        Amarok.debug ("got " + trackElements.length() + " tracks!");

        var titles = new Array( trackElements.length() );
        var links = new Array( trackElements.length() );
        var artists = new Array( trackElements.length() );
        var albums = new Array( trackElements.length() );


        var i = 0;
        for ( ; i < trackElements.length(); i++ )
        {


            elt = trackElements.at( i );
            elt2 = elt.firstChildElement( "title" );

            titles[i] = elt2.text();

            elt2 = elt.firstChildElement( "location" );
            links[i] = elt2.text();

            elt2 = elt.firstChildElement( "album" );
            albums[i] = elt2.text();

            elt2 = elt.firstChildElement( "creator" );
            artists[i] = elt2.text();


        }


        for( i = 0; i < trackElements.length(); i++ )
        {

            title = titles[i]
            link = links[i];
            album = albums[i];
            artist = artists[i]


            item = Amarok.StreamItem;
            item.level = 0;
            item.callbackData = "";
            item.itemName = title;
            item.artist = artist;
            item.album = album;
            item.playableUrl = link;
            item.infoHtml = "";

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
    print ("Seeqpod onPopulate");
    Amarok.debug( "Seeqpod onPopulate, filter: " + filter );
    offset = 0;

    offsetMarker = filter.lastIndexOf("#");
    
    if ( offsetMarker != -1 ) {
        Amarok.debug( "non 0 marker at " + offsetMarker );
        offset = filter.substring( offsetMarker + 1, filter.length )
                Amarok.debug( "Got offset: " + offset );
        offset = offset.replace( "%20", " " );
        offset = parseInt( offset );
        
        Amarok.debug( "Got offset: " + offset );
        //offset = offset.trim();
        
        filter = filter.substring( 0, offsetMarker );
                
        Amarok.debug( "Got final offset: " + offset );
        Amarok.debug( "Got filter: " + filter );
    }
    

    if ( filter != "" )
    {
        name = filter.replace( "%20", " " );
    }
    else
    {
        name = "Enter Query..."
    }

    if ( level == 1 ) {
        
        if ( offset > 0 )
            name = name + " ( " + offset + " - " + (offset + 100) + " )";

        item = Amarok.StreamItem;
        item.level = 1;
        item.callbackData = "dummy";
        item.itemName = name;
        item.playableUrl = "";
        item.infoHtml = html;
        script.insertItem( item );

        script.donePopulating();

    }
    else if ( level == 0 )
    {
        Amarok.debug( " Populating result level..." );
        Amarok.debug( " url: " +  callback );

        try{

            path = "http://www.seeqpod.com/api/v0.2/<UID>/music/search/<QUERY>/<OFFSET>/100";

            path = path.replace( "<UID>", uid )
            path = path.replace( "<QUERY>", filter )
            path = path.replace( "<OFFSET>", offset );
            qurl = new QUrl( path );

            b = new Downloader( qurl, queryResult );
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
uid = "";

script = new Seeqpod();
script.populate.connect( onPopulate );


