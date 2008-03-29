#!/usr/bin/env ruby
###########################################################################
#   Amarok script for interfacing with Seeqpod.com.                       #
#                                                                         #
#   Copyright                                                             #
#   (C) 2007, 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>         #
#   (C) 2008 Mark Kretschmann <kretschmann@kde.org>                       #
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
  error = "Qt4-Ruby (Ruby bindings for Qt4) is required for this script.\n\nsudo apt-get install libqt4-ruby"
  `kdialog --sorry '#{error}'`
  exit 1
end
include REXML


@uid = ""
service_name = "Seeqpod.com"

app = Qt::Application.new(ARGV)

def configure
  @uid = Qt::InputDialog.getText( nil, "Configuration", "Please enter your Seeqpod UID:" )
end


loop do
    message = gets
    puts "script got message" + message
    args = message.chomp.split(" ")

    case args[0]
        when "configure"
            configure

        when "init"
            #2 levels, categories and stations
            levels = "2"
            short_description = "Search and stream from seeqpod.com"
            root_html = "A small script to let you search and stream music from all over the internet, using seeqpod.com"

            # init new browser
            system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "initService", service_name, levels, short_description, root_html, "true" )

        when "populate"
            configure if @uid.empty?
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

                url = url.gsub( "<UID>", @uid )
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

