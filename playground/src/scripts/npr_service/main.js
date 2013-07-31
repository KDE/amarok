/*###########################################################################
#   A script that lets you browse content made available by the NPR         #
#   API. It also retrieves context specific content                         #
#                                                                           #
#   Copyright                                                               #
#   (C) 2008 Casey Link <unnamedrambler@gmail.com>                          #
#   (C) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                 #
#   (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                            #
#   (C) 2008 Sven Krohlas <sven@asbest-online.de>                           #
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
programsUrl = new QUrl( "http://api.npr.org/list?id=3004" );
biosUrl = new QUrl( "http://api.npr.org/list?id=3007" );
columnsUrl = new QUrl( "http://api.npr.org/list?id=3003" );
seriesUrl = new QUrl( "http://api.npr.org/list?id=3006" );
doc    = new QDomDocument("doc");
elt    = new QDomElement;
elt2   = new QDomElement;
topics  = new QDomNodeList;
artists = new QDomNodeList;
initials = new QDomNodeList;
storyelements = new QDomNodeList;

storiesNeedingUrls = new Array();

/* Initialization of service */
function NPRService() {
  Amarok.debug( "creating npr service..." );
  ScriptableServiceScript.call( this, "NPR", 3, "Content made available via the NPR API. Over 250,000 articles, stories, and reviews.", html, false );
  Amarok.debug( "done creating npr service!" );
}

/* Get info for topics */
function topicsDownloadResult( reply ) {
  Amarok.debug( "start npr topics xml parsing..." );
  try {
    doc.setContent( reply );
    topics = doc.elementsByTagName( "item" );
    Amarok.debug ("got " + topics.length() + " topics!");

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
      // this is needed to identify the item when we need to expand
      // it in onPopulate( level, -->callbackData<--, filter )
      item.callbackData = topicid; // the <item id=""> field
      script.insertItem( item );
    }
    script.donePopulating()
  } catch( err ) {
	Amarok.debug( err );
    }
}
/*get info for artists*/
function artistsDownloadResult( reply ) {
  Amarok.debug( "start npr artists xml parsing..." );
  try {
    doc.setContent( reply );
    initials = doc.elementsByTagName( "subcategory" );
    //Amarok.debug ("got " + topics.length() + " artists!");

    var item = Amarok.StreamItem;
    item.level = 1;
    item.playableUrl = "";
    var j = 0;
    for( ; j < initials.length(); j++ ) {
    artists = initials.at(j).toElement().elementsByTagName( "item" );
    
    var i = 0;
    for ( ; i < artists.length(); i++ ) {
      elt = artists.at( i );
      elt2 = elt.firstChildElement( "title" );
      item.itemName = elt2.text();
      item.infoHtml = "";

      var artistid = "";
      artistid = elt.toElement().attribute("id");
      Amarok.debug("got id " + artistid);
      // this is needed to identify the item when we need to expand
      // it in onPopulate( level, -->callbackData<--, filter )
      item.callbackData = artistid; // the <item id=""> field
      script.insertItem( item );
    }
    }
    script.donePopulating()
  } catch( err ) {
	Amarok.debug( err );
    }
}
/*get info for stories*/
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

	elt2 = elt.firstChildElement( "storyDate" );
	var datetime = elt2.text();
	var date = datetime.substring(0, datetime.indexOf(":") - 3);
	if( date.length > 0 )
	  title += " - " + date;
/*	var date = new QDateTime;
	title += " - " + date.fromString(elt2.text(), "ddd, dd MMM yyyy hh:mm:ss -0400").toString("ddd, dd MMM yyyy");*/
      

	elt2 = elt.firstChildElement( "audio" );
	elt2 = elt2.firstChildElement( "format" );
	elt2 = elt2.firstChildElement( "mp3" );
	var m3uurl = elt2.text();

	elt2 = elt.firstChildElement( "byline" );
	elt2 = elt2.firstChildElement( "name" );
	var writer = elt2.text();
	//Amarok.debug ("url: " + item.playableUrl);
	
	Amarok.debug("Adding " + title + " by " + writer + " @ "  + m3uurl);
	var data = title + "\\" + m3uurl + "\\" + writer + "\\" + teaser;
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
      item.infoHtml = data[3];
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
  if( level == 2 ) {
    item = Amarok.StreamItem;
    item.level = 2;
    item.playableUrl = "";

    item.callbackData = "stories_topic";
    item.itemName = "Topics";
    item.infoHtml = "Collection of NPR stories that represent a given topic or subject matter. (eg. Health Care, Interviews)";
    script.insertItem( item );

    item.callbackData = "artist_stories";
    item.itemName = "Music Artists";
    item.infoHtml = "Collection of stories that are about music artists. Artists are sorted by letter. (eg. Bob Dylan, Death Cab For Cutie)";
    script.insertItem( item );

    item.callbackData = "genre_stories";
    item.itemName = "Music Genres";
    item.infoHtml = "Collection of NPR stories that represent a given musical genre. (eg. Rock/Pop/Folk, Jazz)";
    script.insertItem( item );

    item.callbackData = "program_stories";
    item.itemName = "Programs";
    item.infoHtml = "Collection of NPR stories that aired on an NPR program. (eg. All Things Considered, Tell Me More)";
    script.insertItem( item );

    item.callbackData = "bios_stories";
    item.itemName = "Bios";
    item.infoHtml = "Collection of NPR stories as reported by an NPR personality. Personalities are sorted by letter. (eg. Nina Totenburg, Steve Inskeep)";
    script.insertItem( item );

    item.callbackData = "columns_stories";
    item.itemName = "Columns";
    item.infoHtml = "Collection of stories containing opinions and perspectives of an NPR personality. (eg. Watching Washington, Song of the Day)";
    script.insertItem( item );

    item.callbackData = "series_stories";
    item.itemName = "Series";
    item.infoHtml = "An ongoing collection of NPR stories on a topic. (eg. Climate Connections, Summer Books)";
    script.insertItem( item );

    script.donePopulating();
  } else if ( level == 1 ) { // the shows
    Amarok.debug( "fetching npr xml..." );
    Amarok.Window.Statusbar.longMessage( "NPR: Fetching and parsing stories. This might take a little bit, depending on the speed of your internet connection..." );
    try {
      if( callbackData == "stories_topic" )
	a = new Downloader( topicsUrl, topicsDownloadResult );
      else if( callbackData == "artist_stories" )
	a = new Downloader( artistsUrl, artistsDownloadResult );
      else if( callbackData == "genre_stories" )
	a = new Downloader( genresUrl, topicsDownloadResult );
      else if( callbackData == "program_stories" )
	a = new Downloader( programsUrl, topicsDownloadResult );
      else if( callbackData == "columns_stories" )
	a = new Downloader( columnsUrl, topicsDownloadResult );
      else if( callbackData == "series_stories" )
	a = new Downloader( seriesUrl, topicsDownloadResult );
      else if( callbackData == "bios_stories" )
	a = new Downloader( biosUrl, artistsDownloadResult );


    }
    catch( err ) {
      Amarok.debug( err );
    }

  } else if ( level == 0 ) { // the stories from each topic
    try {
      url = new QUrl( baseUrl + callbackData + "&action=Or&fields=title,teaser,storyDate,audio&output=NPRML&numResults=20" );
      Amarok.debug("Fetching stories for topic: " + callbackData);
      Amarok.debug(url);
      a = new Downloader( url, storiesDownloadResult );
    } catch( err ) {
      Amarok.debug( err );
    }
  }
}

script = new NPRService();
script.populate.connect( onPopulate );
