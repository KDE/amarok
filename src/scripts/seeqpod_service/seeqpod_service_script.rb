#!/usr/bin/env ruby
###########################################################################
#   Amarok script for interfacing with SeeqPod.com.                       #
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
  error = "Qt4-Ruby (Ruby bindings for Qt4) is required for the 'SeeqPod' script.\n\nsudo apt-get install libqt4-ruby"
  `kdialog --sorry '#{error}'`
  exit 1
end
include REXML


def configure
  ok = Qt::Boolean.new  # Will become nil if the user presses Cancel
  uid = Qt::InputDialog.getText( nil, "Configuration", "Please enter your SeeqPod API UID (check your SeeqPod profile):", Qt::LineEdit::Normal, "", ok )
  unless ok.nil?
    @uid = uid
    @settings.setValue( "uid", Qt::Variant.new( @uid ) )
    @settings.sync
  end
end


app = Qt::Application.new(ARGV)
@settings = Qt::Settings.new( "./seeqpod_service_rc", Qt::Settings::IniFormat )
@uid = @settings.contains( "uid" ) ? @settings.value( "uid" ).toString : ""
service_name = "SeeqPod.com"


#SeeqPod has given permission to use their background and logo in the html front page below!

root_html = "" +
"<HTML>" +
"    <STYLE type=\"text/css\">" +
"        BODY {" +
"            margin: 0px;" +
"            padding: 0px;" +
"            font-family: Arial, Helvetica, sans-serif;" +
"            font-size: 13px;" +
"            background-color: #FFF;" +
"            color: #878787;" +
"            background-attachment: fixed;" +
"            background-image:url(\"http://www.seeqpod.com/css/page/bg2.jpg\");" +
"            background-repeat: no-repeat;" +
"            background-position: center top;" +
"        }" +
"        DIV { text-align: center;  }" +
"    </STYLE>" +
"    <BODY>" +
"        <DIV>" +
"        <br><br>" +
"        <IMG src=\"http://www.seeqpod.com/images/logo.png\" alt=\"SeeqPod.com\">" +
"        <br><br>" +
"        Welcome to the Amarok 2 SeeqPod.com scripted service. To search SeeqPod, simply type in your search in the search box in the service and expand the item that appears in the list. By default, this script will only return at most 100 matches, but by appending an #offset to your search, it will return further matches. For instance, typing in \"Foobar\" will return the first 100 matches, \"Foobar#100\" the next 100, and so on." +
"        </DIV>" +
"    </BODY>" +
"<HTML>"


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
            short_description = "Search and stream from SeeqPod.com"

            # init new browser
            system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "initService", service_name, levels, short_description, root_html, "true" )

        when "populate"
            configure if @uid.empty?
            filter = "_none_"

            offset = 0;

            if args.length == 5
                callback = args[4].strip();
                callback_args = callback.chomp.split("#")
                filter = callback_args[0];

                if callback_args.length == 2
                    offset = callback_args[1].to_i
                end

                name = filter.gsub( "%20", " " );
                name = name.strip();

            else
                name = "Enter Query..."
            end

            if args[1].strip() == "1"
                puts " Populating main level..."

                html = "The results of your query for: " + filter;

                if offset > 0
                    name = name + " ( " + offset.to_s + " - " + (offset + 100).to_s + " )"
                end

                system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "insertItem", service_name, "1", "-1", name, html, filter, "" )

                #tell service that all items has been added ( no parent since these are top level items )
               `qdbus org.kde.amarok /ScriptableServiceManager donePopulating "SeeqPod.com" "-1"`
                
            else if args[1].strip() == "0"
                parent_id = args[2]

                url = "http://www.seeqpod.com/api/v0.2/<UID>/music/search/<QUERY>/<OFFSET>/100"

                url = url.gsub( "<UID>", @uid )
                url = url.gsub( "<QUERY>", filter )
                url = url.gsub( "<OFFSET>", offset.to_s );

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

                count = 0
                        
                titles.each_with_index do |title, idx|
                    link = links[idx]

                    system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "insertItem", service_name, "0", parent_id, title, "", "", link )
                    count = count + 1
                end

                #tell service that all items has been added to a parent item
                system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "donePopulating", service_name, parent_id )

            end
        end
    end
end

