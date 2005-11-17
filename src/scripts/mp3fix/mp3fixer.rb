#!/usr/bin/env ruby
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# License: GNU General Public License V2


def cleanup()
#     `dcop amarok script removeCustomMenuItem MP3Fixer FixIt!`.untaint()
    `kdialog --sorry menuremoved`; return
end


trap( "SIGTERM", cleanup() )

`dcop amarok script addCustomMenuItem MP3Fixer FixIt!`


loop do
    command = gets().chomp()

#     `kdialog --sorry #{command}`

#     case command
#         when "configure" then `kdialog --sorry configure`
#     end

end



