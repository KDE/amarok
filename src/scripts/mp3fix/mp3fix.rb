#!/usr/bin/env ruby
#
# Script for fixing VBR encoded mp3 files without XING header. Calculates the real
# track length and adds the XING header to the file.
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# License: GNU General Public License V2

path = ""

unless $*.empty?()
    path = $*[0]
else
    puts( "Error: Please specify an mp3 file for input.\n" )
    exit()
end

if not path.include?( ".mp3" ) #FIXME
    puts( "Error: File is not mp3.\n" )
    exit()
end

file = File.new( path, "r" )

if not file.exist?()
    puts( "Error: File not found.\n" )
    exit()
end


data = file.read()

if data[0.3] = "ID3"
    puts( "ID3-V2 detected.\n" )
else
    puts( "ID3-V1 detected.\n" )
end


