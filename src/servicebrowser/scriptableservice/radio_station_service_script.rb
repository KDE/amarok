#!/usr/bin/env ruby

# Simple script for testing the scriptable service browser 
# by creating a simple static browser with some cool radio
# streams. Urls shamelessly stolen Cool-Streams.xml
#
# (c) 2007 Nikolaj Hald Nielsen  <nhnFreespirit@gmail.com>
#
# License: GNU General Public License V2


stations = [ [ 'Bassdrive [Drum \'n Bass]',                   'http://www.bassdrive.com/v2/streams/BassDrive.m3u' ],
             [ 'Bluemars [Ambient/Space-Music]',              'http://207.200.96.225:8020/listen.pls' ],
             ['Digitally Imported - Chillout [Chill-Out]',    'http://di.fm/mp3/chillout.pls' ],
             ['Digitally Imported - Classic Techno [Techno]', 'http://di.fm/mp3/classictechno.pls' ],
             ['Digitally Imported - Trance [Trance]',         'http://di.fm/mp3/trance.pls' ],
             ['Electronic Culture [Minimal Techno]',          'http://www.shouted.fm/tunein/electro-dsl.m3u' ],
             ['Frequence 3 [Pop]',                            'http://streams.frequence3.net/hd-mp3.m3u' ],
             ['Gaming FM [Computer-Music]',                   'http://208.64.81.140:7500/listen.pls' ],
             ['Groove Salad [Chill-Out]',                     'http://www.somafm.com/groovesalad.pls' ],
             ['Drone Zone [Ambient]',                         'http://somafm.com/dronezone.pls' ],
             ['Tags Trance Trip [Trance]',                    'http://somafm.com/tagstrance.pls' ],
             ['Indie Pop Rocks [Indie]',                      'http://www.somafm.com/indiepop.pls' ],
             ['Kohina [Computer-Music]',                      'http://la.campus.ltu.se:8000/stream.ogg.m3u' ],
             ['Mostly Classical [Classical]',                 'http://www.sky.fm/mp3/classical.pls' ],
             ['MTH.House [House]',                            'http://stream.mth-house.de:8500/listen.pls' ],
             ['Nectarine Demoscene Radio [Computer-Music]',   'http://nectarine.sik.fi:8002/live.mp3.m3u' ],
             ['Philosomatika [Psytrance]',                    'http://philosomatika.com/Philosomatika.pls' ],
             ['Proton Radio [House/Dance]',                   'http://protonradio.com/proton.m3u' ],
             ['Pure DJ [Trance]',                             'http://www.puredj.com/etc/pls/128K.pls' ],
             ['Radio.BMJ.net [Trance/Livesets]',              'http://radio.bmj.net:8000/listen.pls' ],
             ['Radio Paradise [Rock/Pop/Alternative]',        'http://www.radioparadise.com/musiclinks/rp_128.m3u' ],
             ['Raggakings [Reggae]',                          'http://www.raggakings.net/listen.m3u' ],
             ['Secret Agent [Downtempo/Lounge]',              'http://somafm.com/secretagent.pls' ],
             ['SLAY Radio [C64 Remixes]',                     'http://sc.slayradio.org:8000/listen.pls' ],
             ['Virgin Radio [Rock/Pop]',                      'http://www.smgradio.com/core/audio/mp3/live.pls?service=vrbb' ],
             ['X T C Radio [Techno/Trance]',                  'http://stream.xtcradio.com:8069/listen.pls' ] ]



# create new browser
`qdbus org.kde.amarok /ScriptableServiceManager createService "Cool Streams" "Streams"`

parentId = `qdbus org.kde.amarok /ScriptableServiceManager insertElement "The Amarok crews top picks" "" "Just a parent item to show how nesting works" 0 "Cool Streams"`.chomp

stations.each() do |station|
    system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "insertElement", station[0], station[1], "Dummy html info", parentId, "Cool Streams")
end

`qdbus org.kde.amarok /ScriptableServiceManager updateComplete "Cool Streams"`

