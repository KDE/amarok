/*#########################################################################
#                                                                         #
#   Simple script for testing the scriptable service browser              #
#   by creating a simple static browser with some cool radio              #
#   streams. URLs shamelessly stolen from Cool-Streams.xml.               #
#                                                                         #
#   Copyright                                                             #
#   (C) 2007, 2008 Nikolaj Hald Nielsen  <nhnFreespirit@gmail.com>        #
#   (C)       2008 Peter ZHOU <peterzhoulei@gmail.com>                    #
#   (C)       2008 Mark Kretschmann <kretschmann@kde.org>                 #
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

function Station( name, url )
{
    this.name = name;
    this.url = url;
}

var stationArray = new Array (
    new Station( "Afterhours.FM [Trance/Livesets]",                     "http://www.ah.fm/192k.m3u" ),
    new Station( "Bassdrive [Drum \'n Bass]",                           "http://www.bassdrive.com/v2/streams/BassDrive.m3u" ),
    new Station( "Bluemars [Ambient/Space-Music]",                      "http://207.200.96.225:8020/listen.pls" ),
    new Station( "Digitally Imported - Chillout [Chill-Out]",           "http://di.fm/mp3/classictechno.pls" ),
    new Station( "Digitally Imported - Classic Techno [Techno]",        "http://di.fm/mp3/classictechno.pls" ),
    new Station( "Digitally Imported - Trance [Trance]",                "http://di.fm/mp3/trance.pls" ),
    new Station( "Evolved.fm [Techno/Trance]",                          "http://vhost.desync.com/evolved.pls" ),
    new Station( "Frequence 3 [Pop]",                                   "http://streams.frequence3.net/hd-mp3.m3u" ),
    new Station( "Kohina [Computer-Music]",                             "http://la.campus.ltu.se:8000/stream.ogg.m3u" ),
    new Station( "Mostly Classical [Classical]",                        "http://www.sky.fm/mp3/classical.pls" ),
    new Station( "MTH.Electro [Minimal Techno]",                        "http://www.shouted.fm/tunein/electro-dsl.m3u" ),
    new Station( "MTH.House [House]",                                   "http://stream.mth-house.de:8500/listen.pls" ),
    new Station( "Nectarine Demoscene Radio [Computer-Music]",          "http://demovibes.de:8000/necta128.ogg" ),
    new Station( "Philosomatika [Psytrance]",                           "http://yp.shoutcast.com/sbin/tunein-station.pls?id=5628" ),
    new Station( "Proton Radio [House/Dance]",                          "http://protonradio.com/proton.m3u" ),
    new Station( "Pure DJ [Trance]",                                    "http://www.puredj.com/etc/pls/128K.pls" ),
    new Station( "Radio Paradise [Rock/Pop/Alternative]",               "http://www.radioparadise.com/musiclinks/rp_128.m3u" ),
    new Station( "Raggakings [Reggae]",                                 "http://www.raggakings.net/listen.m3u" ),
    new Station( "SLAY Radio [C64 Remixes]",                            "http://www.slayradio.org/tune_in.php/128kbps/listen.m3u" ),
    new Station( "SomaFM - Drone Zone [Ambient]",                       "http://somafm.com/dronezone.pls" ),
    new Station( "SomaFM - Groove Salad [Chill-Out]",                   "http://somafm.com/groovesalad.pls" ),
    new Station( "SomaFM - Indie Pop Rocks [Indie]",                    "http://somafm.com/indiepop.pls" ),
    new Station( "SomaFM - Secret Agent [Downtempo/Lounge]",            "http://somafm.com/secretagent.pls" ),
    new Station( "SomaFM - Tags Trance Trip [Trance]",                  "http://somafm.com/tagstrance.pls" ),
    new Station( "Virgin Radio [Rock/Pop]",                             "http://www.smgradio.com/core/audio/mp3/live.pls?service=vrbb" )
);

function CoolStream()
{
    ScriptableServiceScript.call( this, "Cool Streams", 1, "List of some high quality radio streams", "Some really cool radio streams, hand picked for your listening pleasure by your friendly Amarok developers", false );
}

function onPopulating( level, callbackData, filter )
{
    Amarok.debug( " Populating station level..." );
    //add the station streams as leaf nodes
    for ( i = 0; i < stationArray.length; i++ )
    {
        item = Amarok.StreamItem;
        item.level = 0;
        item.callbackData = "";
        item.itemName = stationArray[i].name;
        item.playableUrl = stationArray[i].url;
        item.infoHtml = "A cool stream called " + item.itemName;
        script.insertItem( item );
    }
    script.donePopulating();
}

script = new CoolStream();
script.populate.connect( onPopulating );
