#!/usr/bin/env ruby
#
# Now playing script for IRC. Use with the "/exec -o" command of your client.
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# License: GNU General Public License V2


title  = `dcop amarok player title 2> /dev/null`.chomp()
exit( 1 ) unless $?.success?() # Abort if amaroK isn't running
artist = `dcop amarok player artist`.chomp()
album  = `dcop amarok player album`.chomp()
year   = `dcop amarok player year`.chomp()

output = ""


if title.empty?()
    output += `dcop amarok player nowPlaying`.chomp()
else
    # Strip file extension
    extensions = ".ogg", ".mp3", ".wav"
    if extensions.include?( File.extname( title ).downcase() )
        title = title[0, title.length()-4]
    end

    output += "#{title}"
    output += " - #{artist}" unless artist.empty?()

    unless album.empty?()
        output += " [#{album}"
        output += ", #{year}" unless year == "0"
        output += "]"
    end
end


puts( "np: #{output}" ) unless output.empty?()

