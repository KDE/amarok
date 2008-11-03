/*###########################################################################
#   A script that lets you browse content made available by the NPR         #
#   API. It also retrieves context specific content                         #
#                                                                           #
#   Copyright                                                               #
#   (C) 2008 Casey Link <unnamedrambler@gmail.com>                          #
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

service_name = "NPR";
html = "NPR";


apikey = "MDAyMjI0OTQyMDEyMjU3MjgzNDM3ZDBjZg001";
baseUrl = "http://api.npr.org/query?apikey="+apikey+"&id=";
artistsUrl = new QUrl( "http://api.npr.org/list?id=3009" );
topicsUrl = new QUrl( "http://api.npr.org/list?id=3002" );
genresUrl = new QUrl( "http://api.npr.org/list?id=3018" );
doc    = new QDomDocument("doc");
elt    = new QDomElement;
elt2   = new QDomElement;
topics  = new QDomNodeList;
storyelements = new QDomNodeList;

storiesNeedingUrls = new Array();

/* Configuration */
function onConfigure() {
  Amarok.alert( "sorry", "This script does not require any configuration." );
}

/* Initialization of service */
function NPRService() {
  Amarok.debug( "creating npr service..." );
  ScriptableServiceScript.call( this, "NPR", 2, "Content made available via the NPR API. Over 250,000 articles, stories, and reviews.", html, false );
  Amarok.debug( "done creating npr service!" );
}

/* Get info for shows */
function topicsDownloadResult( reply ) {
  Amarok.debug( "start npr topics xml parsing..." );
  try {
    doc.setContent( reply );
    topics = doc.elementsByTagName( "item" );
    Amarok.debug ("got " + topics.length() + " topics!");
 
    var topicNames = new Array( topics.length() );

    var item = Amarok.StreamItem;
    item.level = 1;
    item.playableUrl = "";

    var i = 0;
    var ids = "";
    for ( ; i < topics.length(); i++ ) {
      elt = topics.at( i );
      elt2 = elt.firstChildElement( "title" );
      item.itemName = elt2.text();

      elt2 = elt.firstChildElement( "additionalInfo" );
      item.infoHtml = elt2.text();
      var topicid = "";
      topicid = elt.toElement().attribute("id");
      Amarok.debug("got id " + topicid);
      ids = ids + "," + topicid 
      // this is needed to identify the item when we need to expand
      // it in onPopulate( level, -->callbackData<--, filter )
      item.callbackData = topicid; // the <item id=""> field
      script.insertItem( item );
    }
    ids = ids.substring(1);
    script.donePopulating()
  } catch( err ) {
	Amarok.debug( err );
    }
}
function storiesDownloadResult( reply ) {
    Amarok.debug ("got stories..." );
    doc.setContent( reply );
    storyelements = doc.elementsByTagName( "story" );
    Amarok.debug ("got " + storyelements.length() + " stories!");
    try {
      var j = 0;
      for ( ; j < storyelements.length(); j++ ) {
	elt = storyelements.at( j );

	var id = elt.toElement().attribute("id", "" );

	elt2 = elt.firstChildElement( "title" );
	var title = elt2.text();

	elt2 = elt.firstChildElement( "teaser" );
	var teaser = elt2.text();

	elt2 = elt.firstChildElement( "audio" );
	elt2 = elt2.firstChildElement( "format" );
	elt2 = elt2.firstChildElement( "mp3" );
	var m3uurl = elt2.text();

	elt2 = elt.firstChildElement( "byline" );
	elt2 = elt2.firstChildElement( "name" );
	var writer = elt2.text();
	//Amarok.debug ("url: " + item.playableUrl);
	
	Amarok.debug("Adding " + title + " by " + writer + " @ "  + m3uurl);
	var data = title + "\\" + m3uurl + "\\" +writer;
	storiesNeedingUrls.push(data);
    }
    Amarok.debug( " fetching playable urls " + storiesNeedingUrls.length);
    if(storiesNeedingUrls.length > 0 ) {
      var url = new QUrl( storiesNeedingUrls[storiesNeedingUrls.length-1].split("\\")[1] )
      a = new Downloader( url, playableUrlDownloadResult );
    }
    } catch( err ) {
      Amarok.debug( err );
    }
  
}

function playableUrlDownloadResult( reply ) {
  Amarok.debug( "Got playable url result" );
  Amarok.debug(reply);
  try {
    html = reply;
    urlrRx = new RegExp( "(http://.*\.mp3)" );
    Amarok.debug("made regex");
    var matches = html.match( urlrRx );
    var data = storiesNeedingUrls.pop().split("\\");
    if( matches ) {
      var actualurl = matches[1];
      var item = Amarok.StreamItem;
      item.level = 0;
      item.callbackData = "";
      item.itemName = data[0];
      item.playableUrl = actualurl;
      item.artist = "NPR";
      item.album = data[2];
      script.insertItem( item );
    }
    if(storiesNeedingUrls.length > 0 ) {
	var newurl = new QUrl( storiesNeedingUrls[storiesNeedingUrls.length-1].split("\\")[1] );
	Amarok.debug("fetching " + newurl  + " with " + storiesNeedingUrls.length + " to go!!!");
	a = new Downloader( newurl, playableUrlDownloadResult );
    }
    else // we're done getting urls
      doneFetchingUrls();

  } catch( err ) {
	Amarok.debug( err );
  }
}
function doneFetchingUrls()
{
  storiesNeedingUrls = new Array();
  script.donePopulating()
}
/* Fill tree view in Amarok */
function onPopulate( level, callbackData, filter ) {
  var i = 0;
  Amarok.debug( "populating npr level: " + level );

  if ( level == 1 ) { // the shows
    Amarok.debug( "fetching npr xml..." );
    Amarok.Window.Statusbar.longMessage( "NPR: Fetching and parsing stories. This might take a little bit, depending on the speed of your internet connection..." );
    try {
      a = new Downloader( topicsUrl, topicsDownloadResult );
    }
    catch( err ) {
      Amarok.debug( err );
    }

  } else if ( level == 0 ) { // the stories from each topic
    try {
      url = new QUrl( baseUrl + callbackData + "&action=Or&fields=title,teaser,audio&output=NPRML&numResults=20" );
      Amarok.debug("Fetching stories for topic: " + callbackData);
      Amarok.debug(url);
      a = new Downloader( url, storiesDownloadResult );
    } catch( err ) {
      Amarok.debug( err );
    }
  }
}

Amarok.configured.connect( onConfigure );

script = new NPRService();
script.populate.connect( onPopulate );
