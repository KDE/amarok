#!/usr/bin/env ruby
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# License: GNU General Public License V2


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
                file = message.split()[3]
                output = `mp3fix #{file}`

                if $?.success?()
                    `kdialog --msgbox Mp3Fixer was successful :)`
                else
                    reg = Regexp.new( "Error:.*", Regexp::MULTILINE )
                    errormsg = reg.match( output )

                    `kdialog --error #{errormsg}`
                end
            end
    end
end

