#!/usr/bin/env ruby
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# License: GNU General Public License V2


def cleanup()
    `dcop amarok script removeCustomMenuItem MP3Fixer FixIt!`
end


trap( "SIGTERM" ) { cleanup() }

`dcop amarok script addCustomMenuItem MP3Fixer FixIt!`


loop do
    message = gets().chomp()
    command = /[A-Za-z]*/.match( message )

    puts( "RECEIVED: #{command}" )

    case command
        when "configure"
            puts( "C0nfig" )

            msg  = '"Mp3Fixer does not have configuration options. Simply select a track in the '
            msg += 'playlist, then start Mp3Fixer from the context-menu (right mouse click)."'

            `kdialog --msgbox #{msg}`

        when "customMenuClicked"
            i = 1
    end
end
