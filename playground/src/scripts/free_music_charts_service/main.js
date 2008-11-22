/*###########################################################################
#   A script that shows you the Darkerradio.com Free Music Charts.        #
#   You can:                                                              #
#    * see the charts positions of each song                              #
#    * listen and download each one                                       #
#                                                                         #
#   Copyright                                                             #
#   (C) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>               #
#   (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                          #
#   (C) 2008 Sven Krohlas <sven@getamarok.com>                            #
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
########################################################################### */

Importer.loadQtBinding( "qt.core" );
Importer.loadQtBinding( "qt.xml" );
Importer.loadQtBinding( "qt.network" );

service_name = "Free Music Charts";
html = "The rules for the Darkerradio Free Music Charts are quite simple: the best 15 songs from the last month and five new ones are the candidates for the next voting. You have up to five votes.<br/><br/>You can cast your votes at <a href=\"http://www.darkerradio.com/pollsarchive/\">www.darkerradio.com/pollsarchive/</a>, support for voting from within Amarok might be added later.<br/><br/>Podcasts of the radio shows (in German) are located at <a href=\"http://www.darkerradio.com/category/podcast/\">www.darkerradio.com/category/podcast/</a>.";

// temporary location for testing purposes, will very likely
// be moved to darkerradio.com
xmlUrl = new QUrl( "http://krohlas.de/fmc.xml" );
http   = new QHttp;
data   = new QIODevice;
doc    = new QDomDocument("doc");
elt    = new QDomElement;
elt2   = new QDomElement;
shows  = new QDomNodeList;


/* Initialization of service */
function FreeMusicCharts() {
  Amarok.debug( "creating fmc service..." );
  ScriptableServiceScript.call( this, "Free Music Charts", 2, "Free Music Charts from Darkerradio.com", html, false );
  Amarok.debug( "done creating fmc service!" );
}


/* Get info for shows */
function fmcShowsXmlParser( reply ) {
  Amarok.debug( "start fmc shows xml parsing..." );
  try {
    doc.setContent( reply );
    shows = doc.elementsByTagName( "show" );
    Amarok.debug ("got " + shows.length() + " shows!");

    var showTitles = new Array( shows.length() );

    item = Amarok.StreamItem;
    item.level = 1;
    item.playableUrl = "";
    item.infoHtml = html;

    var i = 0;
    for ( ; i < shows.length(); i++ ) {
      elt = shows.at( i );
      elt2 = elt.firstChildElement( "title" );
      item.itemName = elt2.text();
      // this is needed to identify the item when we need to expand
      // it in onPopulate( level, -->callbackData<--, filter )
      item.callbackData = i;
      script.insertItem( item );
    }
  }
  catch( err ) {
    Amarok.debug( err );
  }
  script.donePopulating();
}


/* Get info for given track, identified by its playable url */
function fmcTracksXmlParser( url ) {
  Amarok.debug( "start fmc tracks xml parsing..." );
  tempShow = new QDomNodeList;
  songs    = new QDomElement;
  var html = '', rank, votes, title;
  var i = 0, j = 0;

  // for each show...
  for ( ; i < shows.length(); i++ ) {
    Amarok.debug( "found a show..." );
    tempShow = shows.at( i );
    j = tempShow.firstChildElement( "songcount" ).text();
    title = tempShow.firstChildElement( "title" ).text();
    tempShow = tempShow.firstChildElement( "songs" );
    songs    = tempShow.firstChildElement( "song" );

    // ...try to find the track
    for ( ; j != 0; j-- ) {
      if ( songs.firstChildElement( "url" ).text() == url ) { // found song
        rank = songs.firstChildElement( "rank" ).text();
        if( rank == "99") rank = "New";

        votes = songs.firstChildElement( "votes" ).text();
        if( votes == "-1") votes = "none";

        html = html + "<br/><b>" + title;
        html = html + ":</b> Rank: " + rank + ", Votes: " + votes;
      }
      songs = songs.nextSiblingElement( "song" );
    }
  }
  return html;
}


/* Fill tree view in Amarok */
function onPopulate( level, callbackData, filter ) {
  var i = 0;
  Amarok.debug( "populating fmc level: " + level );

  if ( level == 1 ) { // the shows
    Amarok.debug( "fetching fmc xml..." );
    Amarok.Window.Statusbar.longMessage( "Free Music Charts: Fetching charts. This might take some seconds, depending on the speed of your internet connection..." );
    try {
      qurl = new QUrl( xmlUrl );
      a = new Downloader( qurl, fmcShowsXmlParser );
    }
    catch( err ) {
      Amarok.debug( err );
    }

  Amarok.debug( "done fetching fmc xml..." );
  // No script.donePopulating(); here, as the downloader returns
  // immediately, even before the parser is being run.
  // Instead call it at then end of fmcShowsXmlParser(...).
  }

  else if ( level == 0 ) { // the tracks from each show
    Amarok.debug( "populating fmc track level..." );
    item = Amarok.StreamItem;
    item.level = 0;
    item.callbackData = "";

    elt = shows.at( callbackData ); // jump to the correct show

    /* The podcasts */
    item.itemName    = "Podcast (MP3)";
    item.artist      = "Free Music Charts";
    item.infoHtml    = html;
    item.playableUrl = elt.firstChildElement( "podcastmp3" ).text();
    if (item.playableUrl != "not yet available" )
      script.insertItem( item );

    item.itemName    = "Podcast (OGG)";
    item.playableUrl = elt.firstChildElement( "podcastogg" ).text();
    if (item.playableUrl != "not yet available" )
      script.insertItem( item );

    /* The songs */
    var i = elt.firstChildElement( "songcount" ).text(); // get songcount
    Amarok.debug( i );
    elt = elt.firstChildElement( "songs" ); // ascend to songs
    elt = elt.firstChildElement( "song" );  // ascent to first song

    var songArtistTitle = new Array( 2 );

    for ( ; i != 0; i-- ) {
      // beautify rank and votes for newcomers
      elt2 = elt.firstChildElement( "rank" );
      if( elt2.text() == "99") item.itemName = "New";
      else item.itemName = elt2.text();

      item.itemName = item.itemName + ": " + elt.firstChildElement( "name" ).text();

      // split name into artist/title
      songArtistTitle = elt.firstChildElement( "name" ).text().split(" - ");
      Amarok.debug( songArtistTitle[0] );
      Amarok.debug( songArtistTitle[1] );
      item.artist = songArtistTitle[0];

      elt2 = elt.firstChildElement( "url" );
      item.playableUrl = elt2.text();
      item.infoHtml = "<center><b>Chart positions of<br/>";
      item.infoHtml = item.infoHtml + elt.firstChildElement( "name" ).text();
      item.infoHtml = item.infoHtml + "</b></center><br/>";
      item.infoHtml = item.infoHtml + fmcTracksXmlParser( elt2.text() );
      elt = elt.nextSiblingElement( "song" );
      script.insertItem( item );
    }

    Amarok.debug( "done populating fmc track level..." );
    script.donePopulating();
  }
}

script = new FreeMusicCharts();
script.populate.connect( onPopulate );