#!/usr/bin/env ruby
#
# amaroK Script for custom scoring
#
# (c) 2006 Gábor Lehel <illissius@gmail.com>
#
# License: GNU General Public License V2

require 'uri'

loop do
    args = gets.chomp.split(" ")

    case args[0]
        when "configure"
            msg  = 'This script does not require any configuration.'
            `dcop amarok playlist popupMessage "#{msg}"`

        when "requestNewScore"
            prevscore = args[1].to_f
            playcount = args[2].to_i
            length = args[3].to_i
            percentage = args[4].to_i
            url = args[5]

            newscore = ( ( prevscore + percentage ) / 2 ).to_i

            system( "dcop", "amarok", "player", "setScoreByPath", URI::decode( url ), newscore.to_s )
    end
end
