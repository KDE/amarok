# Plugin for the Ruby IRC bot (http://linuxbrit.co.uk/rbot/)
# Fetches the "Word of the day" from http://uncyclopedia.org
# (c) 2005 Mark Kretschmann <markey@web.de>
# Licensed under GPL V2.


class WordofthedayPlugin < Plugin
    def help( plugin, topic="" )
        "Fetches the 'Word of the day' from uncyclopedia.org. Knowledge is power!"
    end

    def privmsg( m )
        begin
            data = bot.httputil.get( URI.parse("http://uncyclopedia.org/wiki/Main_Page") )

            index2 = data.index( "</a></span><br /><i>Try to use it in conversation." )
            index1 = data.rindex( ">", index2 )
            word = data[index1+1..index2-1]

            @bot.say(m.replyto, "Word of the day: \"#{word}\"\nTry to use it in conversation. Knowledge is power.")
        rescue
            m.reply( "Failed to connect to uncyclopedia.org" )
        end
   end
end


plugin = WordofthedayPlugin.new
plugin.register("wordoftheday")

