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
destination = ""

if $*.empty?() or $*[0] == "--help"
    puts( "Usage: mp3fix.rb source [destination]" )
    puts()
    puts( "Mp3fix is a tool for fixing VBR encoded mp3 that show a bogus tracklength in your" )
    puts( "audio player. Mp3fix calculates the real track length and adds the missing XING" )
    puts( "header to the file." )
    exit( 1 )
end

path = $*[0]
destination = path

if $*.length() == 2
    destination = $*[1]
end


if not FileTest::exist?( path )
    puts( "Error: File not found." )
    exit( 1 )
end

if not path.include?( ".mp3" ) #FIXME
    puts( "Error: File is not mp3." )
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
BitRateTable = []
BitRateTable << nil << 32 << 40 << 48 << 56 << 64 << 80 << 96
BitRateTable << 112 << 128 << 160 << 192 << 224 << 256 << 320
SampleRateTable = []
SampleRateTable << 44100 << 48000 << 32100

channelMode = data[offset+3] & 0xc0 >> 6
xingOffset = channelMode == 0x03 ? 17 : 32

if data[id3length + 4 + xingOffset, 4] == "Xing"
    puts( "Error: This file already contains a XING header. Aborting." )
    exit( 1 )
end


frameCount = 0
bitCount = 0
firstFrameBroken = false


# Iterate over all frames
while offset < data.length() - 4
    validHeader = true
    bitrate = 0
    samplerate = 0
    header = data[offset+0]*2**24 + data[offset+1]*2**16 + data[offset+2]*2**8 + data[offset+3]

    # Check for frame sync
    unless header & 0xfff00000 == 0xfff00000
        validHeader = false
        puts( "Sync check failed. Header: #{header}" )
    end

    br = BitRateTable[( header & 0x0000f000 ) >> 12]
    if br == nil
        validHeader = false
        puts( "Bitrate invalid." )
    else
        bitrate = br * 1000
    end

    samplerate = SampleRateTable[( header & 0x00000c00 ) >> 10]
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

        frameCount += 1
        bitCount += bitrate

        offset += frameSize
    else
        if frameCount == 0
            firstFrameBroken = true
        end

        # Find next frame sync
        offset = data.index( 0xff, offset + 1 )
        puts( "Trying to locate frame sync. New offset: #{offset}" )
        puts()
    end
end


# Repair first frame header, if it is broken:

if firstFrameBroken
    puts( "Repairing broken header in first frame." )

    firstHeader = data[id3length+0]*2**24 + data[id3length+1]*2**16 + data[id3length+2]*2**8 + data[id3length+3]
    firstHeader |= 0xfff00000  # Frame sync

    # MPEG ID, Layer, Protection
    firstHeader &= 0xfff0ffff
    firstHeader |= 0x000b0000

    # FIXME
    br = BitRateTable.index( 160 )
    sr = SampleRateTable.index( 44100 )

    firstHeader &= 0xffff00ff
    firstHeader |= br << 12
    firstHeader |= sr << 10

    # Write header back
    data[id3length+0] = ( firstHeader >> 24 ) & 0xff
    data[id3length+1] = ( firstHeader >> 16 ) & 0xff
    data[id3length+2] = ( firstHeader >> 8  ) & 0xff
    data[id3length+3] = ( firstHeader >> 0  ) & 0xff
end


averageBitrate = bitCount / frameCount
length = data.length() / averageBitrate * 8

puts( "Number of frames : #{frameCount}" )
puts( "Average bitrate  : #{averageBitrate}" )
puts( "Length (seconds) : #{length}" )


xing = String.new()
xing << "Xing"

flags = 0x0001 | 0x0002 | 0x0004  # Frames and Bytes fields valid
xing << 0 << 0 << 0 << flags

xing << ( ( frameCount & 0xff000000 ) >> 24 )
xing << ( ( frameCount & 0x00ff0000 ) >> 16 )
xing << ( ( frameCount & 0x0000ff00 ) >> 8 )
xing << ( ( frameCount & 0x000000ff ) >> 0 )

xing << ( ( data.length() & 0xff000000 ) >> 24 )
xing << ( ( data.length() & 0x00ff0000 ) >> 16 )
xing << ( ( data.length() & 0x0000ff00 ) >> 8 )
xing << ( ( data.length() & 0x000000ff ) >> 0 )


# Insert XING header into string, after the first MPEG header
data[id3length + 4 + xingOffset, 0] = xing


destfile = File::open( destination, File::CREAT|File::TRUNC|File::WRONLY )
destfile << data

puts()
puts( "done." )

