#!/usr/bin/env ruby
#
# Now playing script for IRC.

# Use with the "/exec -out" command of your client. You can bind an alias like this:
# /alias np exec -out /home/myself/amaroknowplaying.rb
#
# (c) 2005-2008 Mark Kretschmann <kretschmann@kde.org>
# License: GNU General Public License V2


title  = `qdbus org.kde.amarok /Player title 2> /dev/null`.chomp
exit( 1 ) unless $?.success? # Abort if Amarok isn't running
artist = `qdbus org.kde.amarok /Player artist`.chomp
album  = `qdbus org.kde.amarok /Player album`.chomp
year   = `qdbus org.kde.amarok /Player year`.chomp
streamName = `qdbus org.kde.amarok /Player streamName`.chomp
version = `qdbus org.kde.amarok /Player version`.chomp 

output = ""


if title.empty?
    output += `qdbus org.kde.amarok /Player nowPlaying`.chomp
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

exit 0

