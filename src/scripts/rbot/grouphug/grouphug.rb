# Plugin for the Ruby IRC bot (http://linuxbrit.co.uk/rbot/)
# (c) 2005 Mark Kretschmann <markey@web.de>
# Licensed under GPL V2.

require "net/http"


class OpinionPlugin < Plugin
    def help(plugin, topic="")
        "Grouphug plugin. Confess now!"
    end

    def privmsg(m)
        h = Net::HTTP.new( "grouphug.us", 80 )
        resp, data = h.get( "/random" )

        reg = Regexp.new( '(<td class="conf-text")(.*?)(<p>)(.*?)(</p>)', Regexp::MULTILINE )
        confession = reg.match( data )[4]
        confession.gsub!( /<.*?>/, "" ) # Remove html tags
        confession.gsub!( "\t", "" ) # Remove tab characters

        @bot.say(m.replyto, confession)
   end
end

plugin = OpinionPlugin.new
plugin.register("grouphug")


