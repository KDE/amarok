#(C) Berkus Madfire
#Needs xmlsimple from http://www.maik-schmidt.de/xml-simple.html

require 'net/http'
require 'xmlsimple'


class KDEBugPlugin < Plugin
    def riphtml s
        s.gsub(/<[^>]+>/, '').gsub(/&amp;/,'&').gsub(/&quot;/,'"').gsub(/&lt;/,'<').gsub(/&gt;/,'>').gsub(/&ellip;/,'...').gsub(/&apos;/, "'").gsub("\n",'')
        return s
    end

    def help(plugin,topic="")
	"bug #<number> - displays bugs.kde.org information for given bug number"
    end

    def privmsg(m)
	unless m.params
	    m.reply("incorrect usage: "+help(m.plugin))
	    return
	end
	a = m.params.scan(/^#?([0-9]+)/)
	if a.length == 0
	    m.reply("You called the wrong number...")
	    return
	end
#	m.reply("Please wait, querying...")
	begin
	    h = Net::HTTP.new("bugs.kde.org", 80)
	    resp = h.get("/xml.cgi?id=#{a}", nil)
	    if resp.message == "OK"
		bug = XmlSimple.xml_in(resp.body)
		if bug['bug'][0]['error'] == 'NotFound'
		    m.reply("Bug #{a} was not found in Bugzilla.")
		else
		    str = "PRODUCT: #{bug['bug'][0]['product']}"
		    str += " / #{bug['bug'][0]['component']}" if bug['bug'][0]['component']
		    str += " | STATUS: #{bug['bug'][0]['bug_status']}"
		    str += " | RESOLUTION: #{bug['bug'][0]['resolution']}" if bug['bug'][0]['resolution']
		    str += " | URL: http://bugs.kde.org/show_bug.cgi?id=#{a} | DESCRIPTION: #{riphtml(bug['bug'][0]['short_desc'].to_s)}"
		    m.reply(str)
		end
	    else
		m.reply("Request to bugs.kde.org failed. Message not OK.")
	    end
	rescue
	    m.reply("Request to bugs.kde.org failed")
	end
    end
end

plugin = KDEBugPlugin.new
plugin.register("bug")
