# Plugin for the Ruby IRC bot (http://linuxbrit.co.uk/rbot/)
# (c) 2005 Mark Kretschmann <markey@web.de>
# Licensed under GPL V2.


class OpinionPlugin < Plugin
    def help(plugin, topic="")
        "Ask sanity for its opinion on something"
    end

    def privmsg(m)
        opinions = ["makes me happy.",
                    "makes no sense to me.",
                    "- I'm loving it!",
                    "is fantastic!",
                    "cracks me up, dude.",
                    "reminds me of GNOME.",
                    "is like a dream come true.",
                    "is a sexy hot bitch.",
                    "has some usability issues.",
                    "is like a flight with LSD airlines!",
                    "rocks me hard!",
                    "should be forbidden.",
                    "makes me horny.",
                    "is quite amusing."]

        if m.params
            @bot.say(m.replyto, "#{m.params} #{opinions.at(rand(opinions.length()))}")
        end
   end
end

plugin = OpinionPlugin.new
plugin.register("opinion")


