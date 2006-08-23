# Plugin for the Ruby IRC bot (http://linuxbrit.co.uk/rbot/)
# (c) 2005 Mark Kretschmann <markey@web.de>
# Licensed under GPL V2.

require "net/http"


class GrouphugPlugin < Plugin
    def help( plugin, topic="" )
        "Grouphug plugin. Confess! Usage: 'confess' for random confession, 'confess <number>' for specific one."
    end

    def privmsg( m )
        h = Net::HTTP.new( "grouphug.us", 80 )
        data = ""

        if m.params()
            resp, data = h.get( "/confessions/#{m.params()}" )
        else
            resp, data = h.get( "/random" )
        end

        reg = Regexp.new( '(<td class="conf-text")(.*?)(<p>)(.*?)(</p>)', Regexp::MULTILINE )
        confession = reg.match( data )[4]
        confession.gsub!( /<.*?>/, "" ) # Remove html tags
        confession.gsub!( "\t", "" ) # Remove tab characters

        @bot.say(m.replyto, confession)
   end
end


plugin = GrouphugPlugin.new

plugin.register("grouphug")
plugin.register("confess")

