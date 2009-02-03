/*###########################################################################
#   A script that shows you the Darkerradio.com Free Music Charts.        #
#   You can:                                                              #
#    * see the charts positions of each song                              #
#    * listen and download each one                                       #
#                                                                         #
#   Copyright                                                             #
#   (C) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>               #
#   (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                          #
#   (C) 2008-2009 Sven Krohlas <sven@getamarok.com>                       #
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
Importer.loadQtBinding( "qt.gui" );     // for QPixmap
Importer.loadQtBinding( "qt.network" );
Importer.loadQtBinding( "qt.uitools" ); // for voting-gui
Importer.loadQtBinding( "qt.xml" );

service_name = "Free Music Charts";
html = "The rules for the Darkerradio Free Music Charts are quite simple: the best 15 songs from the last month and five new ones are the candidates for the next voting. Only open music is allowed to take part, since February 2009 a song can stay in the charts for a time of six months max. You have up to five votes.<br/><br/>You can cast your votes by going to the menu bar: <i>Tools -> Free Music Charts Voting</i>";

votingUrl = new QUrl( "http://www.darkerradio.com/free-music-charts/free-music-charts-voting/" );
xmlUrl    = new QUrl( "http://krohlas.de/fmc.xml" );
http      = new QHttp;
data      = new QIODevice;
doc       = new QDomDocument( "doc" );
elt       = new QDomElement;
elt2      = new QDomElement;
shows     = new QDomNodeList;
script    = new FreeMusicCharts();

Amarok.Window.addToolsSeparator();
Amarok.Window.addToolsMenu( "votingGui", "Free Music Charts Voting", "amarok" );

script.populate.connect( onPopulate );
script.customize.connect( onCustomize );
Amarok.Window.ToolsMenu.votingGui.triggered.connect( onVote );

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
    Amarok.debug ( "got " + shows.length() + " shows!" );

    var showTitles = new Array( shows.length() );

    elt = shows.at( 0 ); // latest show
    html = html + "<br/><br/>Next radio show airs on Tuesday, ";
    html = html + elt.firstChildElement( "nextdate" ).text() + ". ";
    html = html + "The voting ends the day before.";

    item = Amarok.StreamItem;
    item.level = 1;
    item.playableUrl = "";
    item.infoHtml = html;
    item.coverUrl = Amarok.Info.scriptPath() + "/FMCShow.png";

    if( shows.length() == 0) {
      Amarok.Window.Statusbar.longMessage( "<b>Free Music Charts</b><br/><br/>Download of charts seems to have <font color=red><b>failed</b></font>. Please check your internet connection." );
      item.itemName = "Download failed :(";
      item.callbackData = "failed";
      script.insertItem( item )
    }

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

    // ...try to find the track, beautify rank and votes for newcomers
    for ( ; j != 0; j-- ) {
      if ( songs.firstChildElement( "url" ).text() == url ) { // found song
        rank = songs.firstChildElement( "rank" ).text();
        if( rank == "99" ) rank = "New";

        votes = songs.firstChildElement( "votes" ).text();
        if( votes == "-1" ) votes = "none";

        html = html + "<br/><b>" + title;
        html = html + ":</b> Rank: " + rank + ", Votes: " + votes;
      }
      songs = songs.nextSiblingElement( "song" );
    }
  }
  return html;
}


/* Set service icon and emblem */
function onCustomize() {
  Amarok.debug ( "customizing fmc service" );
  var currentDir = Amarok.Info.scriptPath() + "/";
  Amarok.debug ( "loading icon: " + currentDir + "FMCIcon.png" );
  var iconPixmap = new QPixmap( currentDir + "FMCIcon.png" );
  script.setIcon( iconPixmap );

  var emblemPixmap = new QPixmap( currentDir + "FMCEmblem.png" );
  script.setEmblem( emblemPixmap );
}


/* Fill tree view in Amarok */
function onPopulate( level, callbackData, filter ) {
  var i = 0;
  Amarok.debug( "populating fmc level: " + level );

  if ( level == 1 ) { // the shows
    Amarok.debug( "fetching fmc xml..." );
    Amarok.Window.Statusbar.longMessage( "<b>Free Music Charts</b><br/><br/>Fetching charts.<br/>This might take some seconds, depending on the speed of your internet connection..." );
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
    elt = shows.at( callbackData ); // jump to the correct show

    item = Amarok.StreamItem;
    item.level = 0;
    item.callbackData = "";
    item.album = elt.firstChildElement( "title" ).text();

    /* The podcasts */
    item.infoHtml    = html;
    item.artist      = "Free Music Charts";
    item.itemName    = "Podcast (MP3)";
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
      // beautify rank for newcomers
      elt2 = elt.firstChildElement( "rank" );
      if( elt2.text() == "99" ) item.itemName = "New";
      else item.itemName = elt2.text();

      item.itemName = item.itemName + ": " + elt.firstChildElement( "name" ).text();

      // split name into artist/title
      songArtistTitle = elt.firstChildElement( "name" ).text().split( " - " );
      Amarok.debug( songArtistTitle[0] );
      Amarok.debug( songArtistTitle[1] );
      item.artist = songArtistTitle[0];
      //item.itemName = songArtistTitle[1];

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

function onVote() {
  Amarok.debug( "FMC: onVote" );
  QDesktopServices.openUrl( votingUrl );
}

