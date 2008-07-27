
# Simple script for testing the scriptable service browser 
# by creating a simple dynamic shoutcast directory browser.
# when called with no arguments it creates the service, and 
# when called with a parent node id and a genre, it populates that
# parent node with all stations in that genre
#
# (c) 2007 Nikolaj Hald Nielsen  <nhnFreespirit@gmail.com>
#
# License: GNU General Public License V2


require 'net/http'
require 'uri'




def parse_genre(nodeId, genre)

    puts "genre: " + genre
    stations_xml =  Net::HTTP.get(URI.parse('http://www.shoutcast.com/sbin/newxml.phtml?genre=' + genre)  )
    station_elements = stations_xml.split("\n")
    station_elements.each  do |station| 
        station =~ /(<station name=")(.*?)(")/
        station_name = $2.to_s
        station =~ /(id=")(.*?)(")/
        station_id = $2.to_s
        station_url = 'http://www.shoutcast.com/sbin/shoutcast-playlist.pls?rn=' + station_id + '&file=filename.pls'
        if station_name != "" 
            system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "insertElement", station_name, station_url, "...", nodeId, "Shoutcast Streams")
        end
    end
    `qdbus org.kde.amarok /ScriptableServiceManager updateComplete "Shoutcast Streams"`

end

def setup_service()

    `qdbus org.kde.amarok /ScriptableServiceManager createService "Shoutcast Streams" "Streams" "The largest collection of radio streams on the net, gathered in one place"`
   
    genre_xml =  Net::HTTP.get(URI.parse('http://www.shoutcast.com/sbin/newxml.phtml')  )
    genre_elements = genre_xml.split("\n")
    genre_elements.shift
    genre_elements.shift
    genre_elements.pop
    
    script_command = "ruby ~/multimedia/amarok/src/servicebrowser/scriptableservice/shoutcast_service.rb"

    genre_elements.each  do |genre| 
        genre =~ /(<genre name=")(.*)("><\/genre>)/#

        system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "insertDynamicElement", $2, script_command, $2, "A collection of streams in the " + genre + " genre", 0.to_s, "Shoutcast Streams")

    end

   `qdbus org.kde.amarok /ScriptableServiceManager updateComplete "Shoutcast Streams"`

end

if ARGV.size == 0 then
   setup_service()
else
   parse_genre(ARGV[0], ARGV[1])
end










