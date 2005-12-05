#!/usr/bin/env ruby
#
# Script for fixing mp3 files which show a bogus track length. Calculates the real
# track length and adds the XING header to the file, and repairs broken MPEG headers.
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# License: GNU General Public License V2


class NumArray < Array
    #
    # Return the nearest matching number from the array
    #
    def nearest_match( num )
        smallest     = 0
        smallestDiff = nil

        each() do |x|
            next if x == nil
            diff = ( x - num ).abs()

            if smallestDiff == nil or diff < smallestDiff
                smallestDiff = diff
                smallest = x
            end
        end

        return smallest
    end
end


#
# Returns the size of the entire ID3-V2 tag. In other words, the offset from
# where the real mp3 data starts.
# @see http://id3lib.sourceforge.net/id3/id3v2com-00.html#sec3.1
#
def calcId3v2Size( data )
    size = data[6]*2**21 + data[7]*2**14 + data[8]*2**7 + data[9]
    size = size + 10 # Header

    return size
end


############################################################################
# MAIN
############################################################################

path = ""
destination = ""

if $*.empty?() or $*[0] == "--help"
    puts( "Usage: mp3fix.rb source [destination]" )
    puts()
    puts( "Mp3fix is a tool for fixing VBR encoded mp3 that show a bogus track length in your" )
    puts( "audio player. Mp3fix calculates the real track length and adds the missing XING" )
    puts( "header to the file. " )
    puts()
    puts( "Additionally, Mp3Fix can repair files with broken MPEG frame headers. This is useful " )
    puts( "for tracks which show an invalid bitrate and samplerate, which also results in bogus " )
    puts( "track length." )
    exit( 1 )
end

path = $*[0]
destination = path

if $*.length() == 2
    destination = $*[1]
end


unless FileTest::readable_real?( path )
    puts( "Error: File not found or not readable." )
    exit( 1 )
end

unless File.extname( path ) == ".mp3" or File.extname( path ) == ".MP3"
    puts( "Error: File is not mp3." )
    exit( 1 )
end

unless FileTest.writable_real?( destination )
    puts( "Error: Destination file not writable." )
    exit( 1 )
end



file = File.new( path, "r" )

data = file.read()
id3length = 0
offset = 0

if data[0,3] == "ID3"
    id3length = calcId3v2Size( data )
    puts( "ID3-V2 detected. Tag size: #{id3length}" )
else
    puts( "ID3-V1 detected." )
end

offset = id3length

SamplesPerFrame = 1152  # Constant for MPEG1 layer 3
BitrateTable = NumArray.new()
BitrateTable << nil << 32000 << 40000 << 48000 << 56000 << 64000 << 80000 << 96000
BitrateTable << 112000 << 128000 << 160000 << 192000 << 224000 << 256000 << 320000
SamplerateTable = NumArray.new()
SamplerateTable << 44100 << 48000 << 32100

ChannelMode = data[offset + 3] & 0xc0 >> 6
XingOffset = ChannelMode == 0x03 ? 17 : 32


frameCount       = 0
bitrateCount     = 0
samplerateCount  = 0
firstFrameBroken = false

puts()
puts( "Analyzing mpeg frames.." )
puts()

# Iterate over all frames
while offset < data.length() - 4
    validHeader = true
    header = data[offset+0]*2**24 + data[offset+1]*2**16 + data[offset+2]*2**8 + data[offset+3]

    # Check for frame sync
    unless header & 0xfff00000 == 0xfff00000
        validHeader = false
        puts( "Sync check failed. Header: #{sprintf("%.8x", header)}" )
    end

    bitrate = BitrateTable[( header & 0x0000f000 ) >> 12]
    if bitrate == nil
        validHeader = false
        puts( "Bitrate invalid." )
    end

    samplerate = SamplerateTable[( header & 0x00000c00 ) >> 10]
    if samplerate == nil
        validHeader = false
        puts( "Samplerate invalid." )
    end


    padding = ( header & 0x00000200 ) >> 9

    if validHeader
        frameSize = ( SamplesPerFrame / 8 * bitrate ) / samplerate + padding

#         puts( "bitrate     : #{bitrate.to_s()}" )
#         puts( "samplerate  : #{samplerate.to_s()}" )
#         puts( "padding     : #{padding.to_s()}" )
#         puts( "framesize   : #{frameSize}" )
#         puts()

        frameCount      += 1
        bitrateCount    += bitrate
        samplerateCount += samplerate
        offset          += frameSize

    else
        firstFrameBroken = true if frameCount == 0

        # Find next frame sync
        offset = data.index( 0xff, offset + 1 )
        break if offset == nil
        puts( "Trying to locate frame sync. New offset: #{offset}" )
        puts()
    end
end


AverageBitrate = bitrateCount / frameCount
Length = data.length() / AverageBitrate * 8

puts( "Number of frames : #{frameCount}" )
puts( "Average bitrate  : #{AverageBitrate}" )
puts( "Length (seconds) : #{Length}" )
puts()


# Repair first frame header, if it is broken:

if firstFrameBroken
    puts()
    puts( "Repairing broken header in first frame." )

    firstHeader = data[id3length+0]*2**24 + data[id3length+1]*2**16 + data[id3length+2]*2**8 + data[id3length+3]
    firstHeader |= 0xfff00000  # Frame sync

    # MPEG ID, Layer, Protection
    firstHeader &= 0xfff0ffff
    firstHeader |= 0x000b0000

    br = BitrateTable.nearest_match( ( bitrateCount / frameCount ).round() )
    sr = SamplerateTable.nearest_match( ( samplerateCount / frameCount ).round() )

    puts( "Setting bitrate    : #{br}" )
    puts( "Setting samplerate : #{sr}" )

    firstHeader &= 0xffff00ff
    firstHeader |= BitrateTable.index( br )    << 12
    firstHeader |= SamplerateTable.index( sr ) << 10

    # Write header back
    data[id3length+0] = ( firstHeader >> 24 ) & 0xff
    data[id3length+1] = ( firstHeader >> 16 ) & 0xff
    data[id3length+2] = ( firstHeader >> 8  ) & 0xff
    data[id3length+3] = ( firstHeader >> 0  ) & 0xff
end


unless data[id3length + 4 + XingOffset, 4] == "Xing"
    puts()
    puts( "Adding XING header." )

    xing = String.new()
    xing << "Xing"

    Flags = 0x0001 | 0x0002 | 0x0004  # Frames and Bytes fields valid
    xing << 0 << 0 << 0 << Flags

    xing << ( ( frameCount & 0xff000000 ) >> 24 )
    xing << ( ( frameCount & 0x00ff0000 ) >> 16 )
    xing << ( ( frameCount & 0x0000ff00 ) >> 8 )
    xing << ( ( frameCount & 0x000000ff ) >> 0 )

    xing << ( ( data.length() & 0xff000000 ) >> 24 )
    xing << ( ( data.length() & 0x00ff0000 ) >> 16 )
    xing << ( ( data.length() & 0x0000ff00 ) >> 8 )
    xing << ( ( data.length() & 0x000000ff ) >> 0 )


    # Insert XING header into string, after the first MPEG header
    data[id3length + 4 + XingOffset, 0] = xing
end


puts()
print( "Writing file..  " )
destfile = File::open( destination, File::CREAT|File::TRUNC|File::WRONLY )

if destfile == nil
    puts( "Error: Destination file is not writable." )
    exit( 1 )
end

destfile << data
puts( "done." )


puts()
puts( "done." )

