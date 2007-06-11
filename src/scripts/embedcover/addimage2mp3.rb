#!/usr/bin/env ruby
#
# Script for embedding album cover images in MP3 files. This requires an existing
# ID3-V2 tag in the file.
#
# (c) 2005-2006 Mark Kretschmann <markey@web.de>
# License: GNU General Public License V2


#
# Returns the size of the ID3-V2 tag. In other words, the offset from
# where the real mp3 data starts.
# @see http://id3lib.sourceforge.net/id3/id3v2com-00.html#sec3.1
#
def getId3v2Size( data )
  return data[6]*2**21 + data[7]*2**14 + data[8]*2**7 + data[9]
end

#
# Sets the size of the ID3-V2 tag.
# @see http://id3lib.sourceforge.net/id3/id3v2com-00.html#sec3.1
#
def setId3v2Size( data, size )
  data[6] = size >> 21 & 0x7f
  data[7] = size >> 14 & 0x7f
  data[8] = size >> 7  & 0x7f
  data[9] = size >> 0  & 0x7f
end

#
# Unsynchronize the entire ID3-V2 tag, frame by frame (replace 0xfff* with 0xff00f*)
#
def unsynchronize( data )
  data[5] |= 0b10000000  # Set Unsychronization global tag flag
  index = 10  # Skip ID3 Header

  until data[index] == 0 or data[index..index+2] == "3DI"
    frametype = data[index..index+3]
    framesize = data[index+4]*2**21 + data[index+5]*2**14 + data[index+6]*2**7 + data[index+7]

    # Check if frame is already unsynced
    if data[index+9] & 0x02 == 0
      puts( "#{frametype}  unsynced " )
      data[index+9] |= 0x02
      index += 10

      framesize.times() do
        sync = data[index] == 0xff
        sync1 = data[index+1] & 0b11100000 == 0b11100000
        sync2 = data[index+1] & 0b11111111 == 0b00000000

        if sync and ( sync1 or sync2 ) and index < framesize - 2
          print( "." ) if sync1
          print( "O" ) if sync2
          data[index+1, 0] = "\0"
          framesize += 1
          index += 1
        end

        index += 1
      end
    else
      puts( "#{frametype}  synced" )
      index += framesize + 10
    end
  end

  # Index == new tag length
  return index - 10
end


############################################################################
# MAIN
############################################################################

path = ""
destination = ""

if $*.length() < 2 or $*[0] == "--help"
  puts( "Usage: addimage2mp3.rb image source [destination]" )
  puts()
  puts( "Adds an image to the ID3-V2 tag of a MP3 file. Requires an existing ID3-V2 tag in" )
  puts( "the file." )
  puts()
  exit( 1 )
end

imagepath = $*[0]
path = $*[1]
destination = path


if $*.length() == 3
  destination = $*[2]
end


unless FileTest::readable_real?( imagepath )
  puts( "Error: Image not found or not readable." )
  exit( 1 )
end

unless FileTest::readable_real?( path )
  puts( "Error: Source not found or not readable." )
  exit( 1 )
end

unless File.extname( path ).downcase() == ".mp3"
  puts( "Error: File is not mp3." )
  exit( 1 )
end


mimetype = case File.extname( imagepath ).downcase()
when ".bmp" then "image/bmp"
when ".gif" then "image/gif"
when ".jpg" then "image/jpeg"
when ".jpeg" then "image/jpeg"
when ".png" then "image/png"
when "" then "image/png"  # Amarok's cover images are always PNG

else
  puts( "Error: Image type invalid." )
  exit( 1 )
end


file = File.new( path, "r" )
data = file.read()
id3length = 0


if data[0,3] == "ID3"
  id3length = getId3v2Size( data )
  puts( "ID3-V2 detected. Tag size: #{id3length}" )
else
  puts( "ID3-V1 detected." )
  puts( "Error: File does not have a ID3-V2 tag." )
  exit( 1 )
end


file = File.new( imagepath, "r" )
image = file.read()


apicframe = String.new()
apicframe << 0x00  # text encoding
apicframe << mimetype
apicframe << 0x00  # mimetype end
apicframe << 0x03  # Picture type (Cover front)
apicframe << 0x00  # Description (empty)
apicframe << image

apicheader = String.new()
apicheader << "APIC"
apicheader << ( ( apicframe.length() >> 21 ) & 0x7f )
apicheader << ( ( apicframe.length() >> 14 ) & 0x7f )
apicheader << ( ( apicframe.length() >> 7  ) & 0x7f )
apicheader << ( ( apicframe.length() >> 0  ) & 0x7f )
apicheader << 0x00  # Flags
apicheader << 0x00  # Flags


# Find end of last ID3 frame, before padding or footer (if present)

puts()
puts( "Locating end of last ID3 frame.." )

index = 10  # Skip ID3 Header
until data[index] == 0 or data[index..index+2] == "3DI"
  frametype = data[index..index+3]
  puts( "Frame Type : #{frametype}" )
  if frametype == "APIC"
    puts( "Error: File already contains an image." )
    exit( 1 )
  end
  framesize = data[index+4]*2**21 + data[index+5]*2**14 + data[index+6]*2**7 + data[index+7]
  index += 10 + framesize
end

index += 10 if data[index..index+2] == "3DI"  # Footer


# Insert APIC frame into string, after the last ID3 frame
data[index, 0] = apicheader + apicframe
id3length += apicheader.length() + apicframe.length()


# Unsynchronization isn't supported by TagLib at this point :|
#
# puts()
# puts( "Unsynchronizing tag.." )
# id3length = unsynchronize( data )
# data[5] |= 0b10000000  # Set ID3 Unsychronization flag


# Adjust ID3V2 tag size
setId3v2Size( data, id3length )
puts()
puts( "Adjusting new ID3 size: #{id3length + 10}" )


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
