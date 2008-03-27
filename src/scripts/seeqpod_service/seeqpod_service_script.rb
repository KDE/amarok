#!/usr/bin/env ruby

# Simple script for testing the scriptable service browser 
# by creating a simple static browser with some cool radio
# streams. Urls shamelessly stolen Cool-Streams.xml
#
# (c) 2007, 2008 Nikolaj Hald Nielsen  <nhnFreespirit@gmail.com>
#
# License: GNU General Public License V2

require "net/http"
require "rexml/document"
include REXML


#This needs to be made configurable somehow....
uid = "^^^INSERT YOUR OWN UID HERE^^^"


service_name = "Seeqpod.com"

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
            short_description = "Search and stream from seeqpod.com"
            root_html = "A small script to let you search and stream music from all over the internet, using seeqpod.com"

            # init new browser
            system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "initService", service_name, levels, short_description, root_html, "true" )

        when "populate"

            filter = "_none_"

            if args.length == 5 
                filter = args[4].strip();
                name = filter.gsub( "%20", " " );
                name = name.strip();
            else
                name = "Enter Query..."
            end


            if args[1].strip() == "1"
                puts " Populating main level..."

                html = "The results of your query for: " + filter;

                system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "insertItem", service_name, "1", "-1", name, html, filter, "" )

                #tell service that all items has been added ( no parent since these are top level items )
               `qdbus org.kde.amarok /ScriptableServiceManager donePopulating "Seeqpod.com" "-1"`

                
            else if args[1].strip() == "0"

                parent_id = args[2]

                url = "http://www.seeqpod.com/api/v0.2/<UID>/music/search/<QUERY>"

                url = url.gsub( "<UID>", uid )
                url = url.gsub( "<QUERY>", filter )

                #fetch results

                data = Net::HTTP.get_response(URI.parse(url)).body


                #some brute force parsing....
                doc = REXML::Document.new(data)
                titles = []
                links = []
                doc.elements.each('playlist/trackList/track/title') do |ele|
                    titles << ele.text
                end
                doc.elements.each('playlist/trackList/track/location') do |ele|
                    links << ele.text
                end

                titles.each_with_index do |title, idx|
                    link = links[idx]


                    system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "insertItem", service_name, "0", parent_id, title, "", "", link )
                end


                #tell service that all items has been added to a parent item
                system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "donePopulating", service_name, parent_id )
                            
            end
        end
    end
end

