#!/usr/bin/env ruby
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

            `kdialog --msgbox #{msg}`

        when "customMenuClicked"
            if message.include?( MenuItemName )
                filereg = Regexp.new( "/.*" )
                file = URI::unescape( filereg.match( message.split()[3] ).to_s() )
                mp3fix = File.dirname( File.expand_path( __FILE__ ) ) + "/mp3fix.rb"
                output = `ruby #{mp3fix} "#{file}"`

                puts( file )

                if $?.success?()
                    `kdialog --msgbox "Mp3Fixer was successful :)"`
                else
                    reg = Regexp.new( "Error:.*", Regexp::MULTILINE )
                    errormsg = reg.match( output )

                    `kdialog --error "Mp3Fixer #{errormsg}"`
                end
            end
    end
end

