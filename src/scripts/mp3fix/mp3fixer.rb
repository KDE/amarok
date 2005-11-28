#!/usr/bin/env ruby
#
# amaroK-Script for fixing VBR encoded mp3 files without XING header. Calculates the real
# track length and adds the XING header to the file. This script is a frontend for the
# mp3fix.rb tool.
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# License: GNU General Public License V2


require "uri"


MenuItemName = "MP3Fixer FixIt!"


def cleanup()
    `dcop amarok script removeCustomMenuItem #{MenuItemName}`
end


trap( "SIGTERM" ) { cleanup() }

`dcop amarok script addCustomMenuItem #{MenuItemName}`


loop do
    message = gets().chomp()
    command = /[A-Za-z]*/.match( message ).to_s()

    case command
        when "configure"
            msg  = '"Mp3Fixer does not have configuration options. Simply select a track in the '
            msg += 'playlist, then start Mp3Fixer from the context-menu (right mouse click)."'

            `dcop amarok playlist popupMessage "#{msg}"`

        when "customMenuClicked"
            if message.include?( MenuItemName )
                args = message.split()
                # Remove the command args
                3.times() { args.delete_at( 0 ) }

                # Iterate over all selected files
                args.each() do |arg|
                    uri = URI.parse( arg )
                    file = URI.unescape( uri.path() )
                    puts file

                    mp3fix = File.dirname( File.expand_path( __FILE__ ) ) + "/mp3fix.rb"

                    `dcop amarok playlist shortStatusMessage "Mp3Fixer is analyzing the file.."`
                    output = `ruby #{mp3fix} "#{file}"`

                    if $?.success?()
                        `dcop amarok playlist popupMessage "Mp3Fixer has successfully repaired your file."`
                    else
                        reg = Regexp.new( "Error:.*", Regexp::MULTILINE )
                        errormsg = reg.match( output )

                        `dcop amarok playlist popupMessage "Mp3Fixer #{errormsg}"`
                    end
                end
            end
    end
end

