/*#########################################################################
#                                                                         #
#   Simple script for testing the scriptable service browser              #
#   by creating a simple static browser with some cool radio              #
#   streams. URLs shamelessly stolen Cool-Streams.xml                     #
#                                                                         #
#   Copyright                                                             #
#   (C) 2007, 2008 Nikolaj Hald Nielsen  <nhnFreespirit@gmail.com>        #
#   (C)       2008 Peter ZHOU            <peterzhoulei@gmail.com>         #
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
##########################################################################*/

//we don't have map in QtScript

var stationName = new Array( "Afterhours.FM [Trance/Livesets]",
                             "Bassdrive [Drum \'n Bass]",
                             "Bluemars [Ambient/Space-Music]",
                             "Digitally Imported - Chillout [Chill-Out]",
                             "Digitally Imported - Classic Techno [Techno]",
                             "Digitally Imported - Trance [Trance]",
                             "Electronic Culture [Minimal Techno]",
                             "Frequence 3 [Pop]",
                             "Gaming FM [Computer-Music]",
                             "Groove Salad [Chill-Out]",
                             "Drone Zone [Ambient]",
                             "Tags Trance Trip [Trance]",
                             "Indie Pop Rocks [Indie]",
                             "Kohina [Computer-Music]",
                             "Mostly Classical [Classical]",
                             "MTH.House [House]",
                             "Nectarine Demoscene Radio [Computer-Music]",
                             "Philosomatika [Psytrance]",
                             "Proton Radio [House/Dance]",
                             "Pure DJ [Trance]",
                             "Radio.BMJ.net [Trance/Livesets]",
                             "Radio Paradise [Rock/Pop/Alternative]",
                             "Raggakings [Reggae]",
                             "Secret Agent [Downtempo/Lounge]",
                             "SLAY Radio [C64 Remixes]",
                             "Virgin Radio [Rock/Pop]",
                             "X T C Radio [Techno/Trance]" );

var stationURL = new Array( "http://www.ah.fm/192k.m3u",
                            "http://www.bassdrive.com/v2/streams/BassDrive.m3u']",
                            "http://207.200.96.225:8020/listen.pls",
                            "http://di.fm/mp3/chillout.pls",
                            "http://di.fm/mp3/classictechno.pls",
                            "http://di.fm/mp3/trance.pls",
                            "http://www.shouted.fm/tunein/electro-dsl.m3u",
                            "http://streams.frequence3.net/hd-mp3.m3u",
                            "http://208.64.81.140:7500/listen.pls",
                            "http://www.somafm.com/groovesalad.pls",
                            "http://somafm.com/dronezone.pls",
                            "http://somafm.com/tagstrance.pls",
                            "http://www.somafm.com/indiepop.pls",
                            "http://la.campus.ltu.se:8000/stream.ogg.m3u",
                            "http://www.sky.fm/mp3/classical.pls",
                            "http://stream.mth-house.de:8500/listen.pls",
                            "http://nectarine.sik.fi:8002/live.mp3.m3u",
                            "http://philosomatika.com/Philosomatika.pls",
                            "http://protonradio.com/proton.m3u",
                            "http://www.puredj.com/etc/pls/128K.pls",
                            "http://radio.bmj.net:8000/listen.pls",
                            "http://www.radioparadise.com/musiclinks/rp_128.m3u",
                            "http://www.raggakings.net/listen.m3u",
                            "http://somafm.com/secretagent.pls",
                            "http://sc.slayradio.org:8000/listen.pls",
                            "http://www.smgradio.com/core/audio/mp3/live.pls?service=vrbb",
                            "http://stream.xtcradio.com:8069/listen.pls" );

service_name = "Cool Streams";


function onConfigure()
{
    Amarok.Statusbar.longMessageThreadSafe( "This script does not require any configuration." );
}

function onInit()
{
    levels = 2;
    short_description = "List of some really cool radio streams";
    root_html = "Some really cool radio streams, hand picked for your listening pleasure by your friendly Amarok developers";
    Amarok.ScriptableServiceManager.initService( service_name, levels, short_description, root_html, false );
}

function onPopulate( level, parent_id, path, filter )
{
    if (level == 1)
    {
        print( "Populating main level..." );

        #add top level item
        Amarok.ScriptableServiceManager.insertItem( service_name, 1, -1, "The Amarok Crew\'s Top Streams", "Just a parent item to show how nesting works", "get_stations", "" );

        #tell service that all items has been added ( no parent since these are top level items )
        Amarok.ScriptableServiceManager.donePopulating( "Cool Streams", -1 );
        print( "... done" );
    }
    else if (level == 0 && filter == "get_stations")
    {
        print( " Populating station level..." );

        #no callback string needed for leaf nodes
        callback_string = "";

        #add the station streams as leaf nodes
//        stations.each() do |station|
//            html_info = "A cool stream called" + station[0];
//            Amarok.ScriptableServiceManager.insertItem( service_name, 0, parent_id, station[0], html_info, callback_string, station[1] );
//        end

        #tell service that all items has been added to a parent item
        Amarok.ScriptableServiceManager.donePopulating( "Cool Streams", parent_id );
    }
}

Amarok.configured.connect( conConfigure );
Amarok.ScriptableServiceManager.init.connect( onInit );
Amarok.ScriptableServiceManager.populate.connect( onPopulate );




