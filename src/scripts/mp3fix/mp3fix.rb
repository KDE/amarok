#!/usr/bin/env ruby
#
# Script for fixing VBR encoded mp3 files without XING header. Calculates the real
# track length and adds the XING header to the file.
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# License: GNU General Public License V2


def calcId3v2Size( data )
    # Returns the size of the entire ID3-V2 tag. In other words, the offset from
    # where the real mp3 data starts.
    # @see http://id3lib.sourceforge.net/id3/id3v2com-00.html#sec3.1

    size = data[6]*2**21 + data[7]*2**14 + data[8]*2**7 + data[9]
    size = size + 10 # Header

    return size
end


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

if not FileTest::exist?( path )
    puts( "Error: File not found.\n" )
    exit()
end


file = File.new( path, "r" )

data = file.read()
offset = 0

if data[0,3] = "ID3"
    offset = calcId3v2Size( data )
    puts( "ID3-V2 detected. Tag size: #{offset}\n" )
else
    puts( "ID3-V1 detected.\n" )
end


