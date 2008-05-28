#!/usr/bin/env ruby
###########################################################################
#   Amarok script for interfacing with Librivox.org.                       #
#                                                                         #
#   Copyright                                                             #
#   (C) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>               #
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

require "net/http"
require "rexml/document"
begin
  require "Qt4"
rescue LoadError
  error = "Qt4-Ruby (Ruby bindings for Qt4) is required for the 'SeeqPod' script.\n\nsudo apt-get install libqt4-ruby"
  `kdialog --sorry '#{error}'`
  exit 1
end
include REXML


def configure
  msg  = 'This script does not require any configuration.'
 `qdbus org.kde.amarok /PlayerList popupMessage "#{msg}"`
end


app = Qt::Application.new(ARGV)
service_name = "Librivox.org"


#SeeqPod has given permission to use their background and logo in the html front page below!

root_html = "Librivox service script"


loop do
    message = gets
    puts "script got message" + message
    args = message.chomp.split(" ")

    case args[0]
        when "configure"
            configure

        when "init"
            #3 levels, query, books and episodes
            levels = "3"
            short_description = "Search for books from Librivox"

            # init new browser
            system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "initService", service_name, levels, short_description, root_html, "true" )

        when "populate"
        
            filter = "_none_"

            offset = 0;

            if args.length == 5
                filter = args[4].strip();

                name = filter.gsub( "%20", " " );
                name = name.strip();

            else
                name = "Enter Query..."
            end

            if args[1].strip() == "2"
                puts " Populating main level..."

                html = "The results of your query for: " + filter;

                if offset > 0
                    name = name + " ( " + offset.to_s + " - " + (offset + 100).to_s + " )"
                end

                system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "insertItem", service_name, "2", "-1", name, html, filter, "" )

                #tell service that all items has been added ( no parent since these are top level items )
               `qdbus org.kde.amarok /ScriptableServiceManager donePopulating "Librivox.org" "-1"`
                
            else if args[1].strip() == "1"
                parent_id = args[2]

                puts " Populating book level..."

                url = "http://librivox.org/newcatalog/search_xml.php?simple=<QUERY>"
                url = url.gsub( "<QUERY>", filter )

                #fetch results
                data = Net::HTTP.get_response(URI.parse(url)).body

                #some brute force parsing....
                doc = REXML::Document.new(data)
                titles = []
                links = []

                doc.elements.each('results/book/title') do |ele|
                    titles << ele.text
                end
                            
                doc.elements.each('results/book/url') do |ele|
                    links << ele.text
                end

                count = 0
                        
                titles.each_with_index do |title, idx|
                    link = links[idx]

                    system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "insertItem", service_name, "1", parent_id, title, "", link, "" )
                    count = count + 1
                end

                #tell service that all items has been added to a parent item
                system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "donePopulating", service_name, parent_id )


            else if args[1].strip() == "0"
                parent_id = args[2]
                puts " Populating episode level..."
                puts " url: " +  args[3].strip() ;

                #fetch results
                    data = Net::HTTP.get_response( URI.parse( args[3].strip() ) ).body

                #cut result down to size a little
                startIndex = data.index( "<ul id=\"chapters\">" )
                data = data.slice!(startIndex..data.length-1)

                #remove all <em> and </em> as they screw up simple parsing if present
                data = data.gsub("<em>", "")
                data = data.gsub("</em>", "")

                #get stuff we need
                    data.scan(/<li>(.*?)<br\s\/>\n.*\n.*\n.*href=\"(.*?\.ogg)\">ogg\svorbis/) do |a|
                    puts "title: " + a[0]
                    puts "url: " + a[1]
                    
                   system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "insertItem", service_name, "0", parent_id, a[0], "", "", a[1] )
                end

                system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "donePopulating", service_name, parent_id )

            end
        end
    end
end
end
