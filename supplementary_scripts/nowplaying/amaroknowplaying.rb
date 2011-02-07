#!/usr/bin/env ruby
###########################################################################
#   Now playing script for IRC. Supports both Amarok 1 and Amarok 2.      #
#                                                                         #
#   Use with the "/exec -out" command of your client.                     #
#   You can bind an alias like this (tested with irssi):                  #
#   /alias np exec -out - /home/myself/amaroknowplaying.rb                #
#                                                                         #
#   Copyright                                                             #
#   (C) 2005-2008 Mark Kretschmann <kretschmann@kde.org>                  #
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

require 'cgi'

def amarok_1
    title  = `dcop amarok player title 2> /dev/null`.chomp
    artist = `dcop amarok player artist`.chomp
    album  = `dcop amarok player album`.chomp
    year   = `dcop amarok player year`.chomp
    lastfm = `dcop amarok player lastfmStation`.chomp

    output = ""

    if title.empty?
        output += `dcop amarok player nowPlaying`.chomp
    else
        # Strip file extension
        extensions = ".ogg", ".mp3", ".wav", ".flac", ".fla", ".wma", ".mpc"
        ext = File.extname( title ).downcase

        if extensions.include?( ext )
            title = title[0, title.length - ext.length]
        end

        if artist.empty?
            output += "#{title}"
        else
            output += "#{artist} - #{title}"
        end

        unless album.empty?
            output += " [#{album}"
            output += ", #{year}" unless year == "0"
            output += "]"
        end

        unless lastfm.empty?
            output += " (Last.fm #{lastfm})"
        end
    end

    puts( "np: #{output}" ) unless output.empty?
end


def amarok_2
    metadata  = `qdbus org.mpris.amarok /Player GetMetadata 2> /dev/null`.chomp

    hash = Hash.new
    metadata = metadata.split("\n")
    metadata.each do |line|
      key = line.partition(":")[0] # head
      value = line.partition(":")[2].lstrip() # tail
      hash[key] = value 
    end

    title  = hash["title"]
    artist = hash["artist"] 
    album  = hash["album"] 
    year   = hash["year"] 
    streamName = "" #CGI.unescape(`qdbus org.mpris.amarok /Player streamName`.chomp)
    version = "" #`qdbus org.mpris.amarok /Player version`.chomp 

    output = ""

    if title.empty?
        output += `qdbus org.mpris.amarok /Player nowPlaying`.chomp
    else
        # Strip file extension
        extensions = ".ogg", ".mp3", ".wav", ".flac", ".fla", ".wma", ".mpc", ".oga"
        ext = File.extname( title ).downcase

        if extensions.include?( ext )
            title = title[0, title.length - ext.length]
        end

        if artist.empty?
            output += "#{title}"
        else
            output += "#{artist} - #{title}"
        end

        unless album.empty?
            output += " [#{album}"
            output += ", #{year}" unless year == "0" or year.empty?
            output += "]"
        end

        unless streamName.empty?
            output += " (#{streamName})"
        end
        
        unless version.empty?
            output += " (Amarok #{version})"
        end
    end

    puts( "np: #{output}" ) unless output.empty?
end


system("export DBUS_SESSION_BUS_ADDRESS=")  # Hack for making qdbus work in a screen session

test = `qdbus org.mpris.amarok /Player GetStatus 2> /dev/null`.chomp
if $?.success?
  amarok_2
  exit
end
test = `dcop amarok player title 2> /dev/null`.chomp
if $?.success?
  amarok_1
  exit
end

exit( 1 ) # Abort if Amarok isn't running
