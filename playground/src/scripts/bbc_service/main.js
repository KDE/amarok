/*###########################################################################
#   A script that lets you browse and listen to free content                #
#   From the BBC                                                            #
#                                                                           #
#   Copyright                                                               #
#   (C) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                 #
#   (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                            #
#   (C) 2008 Sven Krohlas <sven@getamarok.com>                              #
#                                                                           #
#   This program is free software; you can redistribute it and/or modify    #
#   it under the terms of the GNU General Public License as published by    #
#   the Free Software Foundation; either version 2 of the License, or       #
#   (at your option) any later version.                                     #
#                                                                           #
#   This program is distributed in the hope that it will be useful,         #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of          #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           #
#   GNU General Public License for more details.                            #
#                                                                           #
#   You should have received a copy of the GNU General Public License       #
#   along with this program; if not, write to the                           #
#   Free Software Foundation, Inc.,                                         #
#   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           #
###########################################################################*/

Importer.loadQtBinding( "qt.core" );
Importer.loadQtBinding( "qt.xml" );
Importer.loadQtBinding( "qt.network" );

service_name = "BBC";
html = "BBC";
globalFilter = "";


xmlUrl = new QUrl( "http://open.bbc.co.uk/rad/uriplay/availablecontent" );
http   = new QHttp;
data   = new QIODevice;
doc    = new QDomDocument("doc");
elt    = new QDomElement;
elt2   = new QDomElement;
shows  = new QDomNodeList;

episodes = new Object();
urls = new Object();

xmlFetched = 0;


function trimKey( key ) {

    newKey = key.replace("http://open.bbc.co.uk/rad/uriplay/content/", "" );
    /*index = newKey.indexOf( "/" );
    newKey = newKey.substring(index);
    newKey = newKey.replace( "/", "" );
    newKey = newKey.replace( "-", "_" );*/

    return newKey;
}

String.prototype.trim = function() {
    a = this.replace(/^\s+/, '');
    return a.replace(/\s+$/, '');
}


/* Initialization of service */
function BBCService() {
  Amarok.debug( "creating bbc service..." );
  ScriptableServiceScript.call( this, "BBC", 2, "Freely available content from BBC", html, true );
  Amarok.debug( "done creating bbc service!" );
}


/* Get info for shows */
function xmlDownloadResult( reply ) {
  Amarok.debug( "start bbc shows xml parsing..." );
  try {
    doc.setContent( reply );

    shows = doc.elementsByTagName( "po:Brand" );
    Amarok.debug ("got " + shows.length() + " shows!");


    //Amarok.debug ("building episode map..." );


    //build a map of all episodes keyed by their unique url
    var episodeNodes = doc.elementsByTagName( "po:Episode" );

    var j = 0;
    for ( ; j < episodeNodes.length(); j++ ) {


      elt = episodeNodes.at( j );

      var key = trimKey( elt.toElement().attribute("rdf:about", "failed" ) );
      var data = "";;



      elt2 = elt.firstChildElement( "dc:title" );
      data += elt2.text();
      data += "\\";

      elt2 = elt.firstChildElement( "dc:description" );
      data += elt2.text();
      data += "\\";


      elt2 = elt.lastChildElement( "rdfs:seeAlso" );
      data += elt2.attribute( "rdf:resource", "" );

      //Amarok.debug ("url: " + item.playableUrl);

      //episodes[key] = item2;
      episodes[key] = data;
      //Amarok.debug ("item " + data + " inserted with key: " + key);

    }

    //build a map of download urls as these are in some cass spread out all over the place
    var playNodes = doc.elementsByTagName( "play:Location" );

    var k = 0;
    for ( ; k < playNodes.length(); k++ ) {

      elt = playNodes.at( k );

      var ulrKey = trimKey( elt.toElement().attribute("rdf:about", "failed" ) );

      elt2 = elt.lastChildElement( "play:uri" );
      var url = elt2.attribute( "rdf:resource", "" );

      //Amarok.debug ("url: " + item.playableUrl);

      //episodes[key] = item2;
      urls[ulrKey] = url;
      //Amarok.debug ("url " + url + " inserted with key: " + ulrKey);

    }

    //Amarok.debug ("got " + episodeNodes.length() + " episodes!");

  }
  catch( err ) {
    Amarok.debug( err );
  }
  xmlFetched == 1;
  populateShows( globalFilter );
}


function populateShows( filter ) {

  //Amarok.debug ("in populateShows");
  try {

    var showTitles = new Array( shows.length() );

    var item = Amarok.StreamItem;
    item.level = 1;
    item.playableUrl = "";

    var i = 0;
    for ( ; i < shows.length(); i++ ) {

        elt = shows.at( i );
        elt2 = elt.firstChildElement( "dc:title" );
        item.itemName = elt2.text();

        var lowerCaseTitle = item.itemName.toLowerCase();
        var lowerCaseFilter = filter.toLowerCase();

        //Amarok.debug ("searching for filter " + lowerCaseFilter + " in title " + lowerCaseTitle);


        if ( lowerCaseFilter != "" && lowerCaseTitle.indexOf(lowerCaseFilter) == -1 ) {
            continue;
        }


	elt2 = elt.firstChildElement( "dc:description" );
	item.infoHtml = elt2.text();

	// this is needed to identify the item when we need to expand
	// it in onPopulate( level, -->callbackData<--, filter )
	item.callbackData = i;
	script.insertItem( item );

        
    }

 } catch( err ) {
    Amarok.debug( err );
  }

  script.donePopulating()

}


/* Fill tree view in Amarok */
function onPopulate( level, callbackData, filter ) {
  var i = 0;
  Amarok.debug( "populating bbc level: " + level );

  filter = filter.replace( "%20", " " );
  filter = filter.trim();
  globalFilter = filter;

  if ( level == 1 ) { // the shows

    try {

        if ( shows.length() == 0 ) {
            Amarok.debug( "fetching bbc xml..." );
            Amarok.Window.Statusbar.longMessage( "<b>BBC</b><br/><br/>Fetching and parsing shows.<br/>This might take some seconds, depending on the speed of your internet connection..." );
            qurl = new QUrl( xmlUrl );
            a = new Downloader( qurl, xmlDownloadResult );
        } else {
            populateShows( filter );
        }
    } catch( err ) {
      Amarok.debug( err );
    }

  }

  else if ( level == 0 ) { // the tracks from each show

    var show = new QDomElement;
    //Amarok.debug ("got " + shows.length() + " shows! ( only need one right now... )");

    try {

        show = shows.at( callbackData );

        elt = show.firstChildElement( "dc:title" );
       // Amarok.debug ("populating show: " + elt.text() );

/*for(att in episodes){
 Amarok.debug ("att: " + att + ", " + episodes[att].itemName)
}*/


        //for ( elt = show.firstChildElement( "po:Episode" ); /*!elt.isNull()*/ 1; elt = show.nextSiblingElement( "po:Episode" )) {
            
        elt = show.firstChildElement( "po:episode" );

        while ( elt.isNull() == false ) {

            //Amarok.debug ("here!!: " + elt.isNull() );

            var key = trimKey( elt.attribute( "rdf:resource", "failed" ) );
 
            if ( key == "failed" ) {
                //dig one deeper
		elt2 = elt.firstChildElement( "po:Episode" );
		key = trimKey( elt2.attribute( "rdf:about", "failed" ) );

            }

            //Amarok.debug ("using key: " + key );
            //Amarok.debug ("inserting item: " + episodes[key] );


            var item2 = Amarok.StreamItem;
            item2.level = 0;
            item2.callbackData = "";

            var item_array=episodes[key].split("\\");


            item2.itemName = item_array[0];
            item2.infoHtml = item_array[1];

            item2.playableUrl = urls[key + "main/main/main/"];

            item2.artist = "BBC";


            script.insertItem( item2 );

            elt = elt.nextSiblingElement( "po:episode" )

        }

    } catch( err ) {
        Amarok.debug( err );
    }


    script.donePopulating();
    //Amarok.debug( "done populating bbc track level..." );
  }
}

script = new BBCService();
script.populate.connect( onPopulate );
