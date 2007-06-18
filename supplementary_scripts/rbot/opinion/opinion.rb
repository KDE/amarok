# Plugin for the Ruby IRC bot (http://linuxbrit.co.uk/rbot/)
# (c) 2005-2006 Mark Kretschmann <markey@web.de>
# Licensed under GPL V2.


class OpinionPlugin < Plugin
    def help(plugin, topic="")
        "Ask the bot for its opinion on something."
    end

    def privmsg(m)
        opinions = [" makes me happy.",
                    " makes no sense to me.",
                    " - I'm loving it!",
                    " is like a dream come true.",
                    " is a sexy hot bitch.",
                    " is like a flight with LSD airlines!",
                    " rocks me hard!",
                    " makes me horny.",
                    " is quite amusing.",
                    " - by eean's beard, I'm into it!",
                    ", waaay into it.",

                    " cracks me up, dude.",
                    " reminds me of GNOME.",
                    " has some usability issues.",
                    " should be forbidden.",
                    ", hell, I'd rather use Winamp.",
                    "!! OMG!! PONIES!!!\nLOL!!"
                    ]

        if m.params
            index = m.params.hash % opinions.length
            @bot.say(m.replyto, "#{m.params}#{opinions.at( index ) }")
        end
   end
end

plugin = OpinionPlugin.new
plugin.register("opinion")


