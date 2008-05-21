#!/usr/bin/env ruby
###########################################################################
#                                                                         #
#   Simple script for testing the scriptable service browser              #
#   by creating a simple static browser with some cool radio              #
#   streams. Urls shamelessly stolen Cool-Streams.xml                     #
#                                                                         #
#   Copyright                                                             #
#   (c) 2007, 2008 Nikolaj Hald Nielsen  <nhnFreespirit@gmail.com>        #
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
###########################################################################


stations = [ ['Bassdrive [Drum \'n Bass]',                    'http://www.bassdrive.com/v2/streams/BassDrive.m3u'],
             ['Bluemars [Ambient/Space-Music]',               'http://207.200.96.225:8020/listen.pls'],
             ['Digitally Imported - Chillout [Chill-Out]',    'http://di.fm/mp3/chillout.pls'],
             ['Digitally Imported - Classic Techno [Techno]', 'http://di.fm/mp3/classictechno.pls'],
             ['Digitally Imported - Trance [Trance]',         'http://di.fm/mp3/trance.pls'],
             ['Electronic Culture [Minimal Techno]',          'http://www.shouted.fm/tunein/electro-dsl.m3u'],
             ['Frequence 3 [Pop]',                            'http://streams.frequence3.net/hd-mp3.m3u'],
             ['Gaming FM [Computer-Music]',                   'http://208.64.81.140:7500/listen.pls'],
             ['Groove Salad [Chill-Out]',                     'http://www.somafm.com/groovesalad.pls'],
             ['Drone Zone [Ambient]',                         'http://somafm.com/dronezone.pls'],
             ['Tags Trance Trip [Trance]',                    'http://somafm.com/tagstrance.pls'],
             ['Indie Pop Rocks [Indie]',                      'http://www.somafm.com/indiepop.pls'],
             ['Kohina [Computer-Music]',                      'http://la.campus.ltu.se:8000/stream.ogg.m3u'],
             ['Mostly Classical [Classical]',                 'http://www.sky.fm/mp3/classical.pls'],
             ['MTH.House [House]',                            'http://stream.mth-house.de:8500/listen.pls'],
             ['Nectarine Demoscene Radio [Computer-Music]',   'http://nectarine.sik.fi:8002/live.mp3.m3u'],
             ['Philosomatika [Psytrance]',                    'http://philosomatika.com/Philosomatika.pls'],
             ['Proton Radio [House/Dance]',                   'http://protonradio.com/proton.m3u' ],
             ['Pure DJ [Trance]',                             'http://www.puredj.com/etc/pls/128K.pls'],
             ['Radio.BMJ.net [Trance/Livesets]',              'http://radio.bmj.net:8000/listen.pls'],
             ['Radio Paradise [Rock/Pop/Alternative]',        'http://www.radioparadise.com/musiclinks/rp_128.m3u'],
             ['Raggakings [Reggae]',                          'http://www.raggakings.net/listen.m3u'],
             ['Secret Agent [Downtempo/Lounge]',              'http://somafm.com/secretagent.pls'],
             ['SLAY Radio [C64 Remixes]',                     'http://sc.slayradio.org:8000/listen.pls'],
             ['Virgin Radio [Rock/Pop]',                      'http://www.smgradio.com/core/audio/mp3/live.pls?service=vrbb'],
             ['X T C Radio [Techno/Trance]',                  'http://stream.xtcradio.com:8069/listen.pls'] ]

service_name = "Cool Streams"

loop do
    message = gets
    puts "script got message" + message
    args = message.chomp.split(" ")

    case args[0]
        when "configure"
            `qdbus org.kde.amarok /Playlist popupMessage "This script does not require any configuration."`
        when "init"

            #2 levels, categories and stations
            levels = "2"
            short_description = "List of some really cool radio streams"
            root_html = "Some really cool radio streams, hand picked for your listening pleasure by your friendly Amarok developers"

            # init new browser
            system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "initService", service_name, levels, short_description, root_html, "false")

        when "populate"
            if args[1].strip() == "1"
                puts " Populating main level..."

                #add top level item 
                parentId = `qdbus org.kde.amarok /ScriptableServiceManager insertItem "Cool Streams" 1 -1 "The Amarok Crew's Top Streams" "Just a parent item to show how nesting works" "get_stations" ""`.chomp

                #tell service that all items has been added ( no parent since these are top level items )
                `qdbus org.kde.amarok /ScriptableServiceManager donePopulating "Cool Streams" "-1"`
                puts "... done"
                
            else if args[1].strip() == "0" and args[3].strip() == "get_stations"
                puts " Populating station level..."

                #leaf nodes
                level = "0"
                parent_id = args[2]

                #no callback string needed for leaf nodes
                callback_string = ""

                #add the station streams as leaf nodes
                stations.each() do |station|
                    html_info = "A cool stream called" + station[0]
                    system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "insertItem", service_name, "0", parent_id, station[0], html_info, callback_string, station[1])
                end

                #tell service that all items has been added to a parent item
                `qdbus org.kde.amarok /ScriptableServiceManager donePopulating "Cool Streams" args[2]`
            end
        end
    end
end

