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

/* TODO:
* sometimes Umlauts in Song names are broken
* icon (not yet possible, missing api)
* cover for each item (not yet possible, missing api)
* give Amarok hints to correct artist/track (preparations done but not yet possible, interface will be done by nhnFreespirit)
* make the infoHtml for tracks actually appear
* stop script problem
*/

Importer.loadQtBinding( "qt.core" );
Importer.loadQtBinding( "qt.xml" );
Importer.loadQtBinding( "qt.network" );

function onConfigure() {
  Amarok.alert( "sorry", "This script does not require any configuration." );
}

function fmcShowsXmlParser( reply ) {
  Amarok.debug( "start fmc xml parsing..." );
  try {

    doc.setContent( reply );
    Amarok.debug( doc );

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

function FreeMusicCharts() {
  Amarok.debug( "creating fmc service..." );
  ScriptableServiceScript.call( this, "Free Music Charts", 2, "Free Music Charts from Darkerradio.com", html, false );
  Amarok.debug( "done creating fmc service!" );
}

function onPopulate( level, callbackData, filter ) {
  var i = 0;
  Amarok.debug( "populating fmc level: " + level );

  if ( level == 1 ) { // the shows
    Amarok.debug( "fetching fmc xml..." );
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
    var i = elt.firstChildElement( "songcount" ).text(); // get songcount
    Amarok.debug( i );
    elt = elt.firstChildElement( "songs" ); // ascend to songs
    elt = elt.firstChildElement( "song" );  // ascent to first song

    var songArtistTitle = new Array( 2 );

    for ( ; i != 0; i-- ) {
      elt2 = elt.firstChildElement( "rank" );
      item.itemName = elt2.text();
      elt2 = elt.firstChildElement( "name" );
      item.itemName = item.itemName + ": " + elt2.text();

      // split name into artist/title
      // XXX: use the new api to feed that to Amarok
      songArtistTitle = elt2.text().split(" - ");
      Amarok.debug( songArtistTitle[0] );
      Amarok.debug( songArtistTitle[1] );

      // create beautiful infoHtml
      item.infoHtml = "Song: " + elt2.text() + "<br/>";
      item.infoHtml = item.infoHtml + "Rank: ";
      item.infoHtml = item.infoHtml + elt.firstChildElement( "rank" ).text() + "<br/>";
      item.infoHtml = item.infoHtml + "Votes: ";
      item.infoHtml = item.infoHtml + elt.firstChildElement( "votes" ).text();

      elt2 = elt.firstChildElement( "url" );
      item.playableUrl = elt2.text();
      elt = elt.nextSiblingElement( "song" );
      script.insertItem( item );
    }

    Amarok.debug( "done populating fmc track level..." );
    script.donePopulating();
  }
}

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
Amarok.configured.connect( onConfigure );

script = new FreeMusicCharts();
script.populate.connect( onPopulate );