#-- vim:sw=2:et
#++
# Author:: Berkus Madfire
# Author:: Casey Link <unnamedrambler@gmail.com>
#
# Copyright:: (C) Berkus Madfire
# Copyright:: (C) 2008 Casey Link
#
# License:: GPL v2
require 'rexml/document'

class KDEBugPlugin < Plugin
   include REXML
   def help(plugin,topic="")
	  "bug #<number> - displays bugs.kde.org information for given bug number"
   end

   def message(m)
      a = m.message.to_s.scan(/(?:bug|#|show_bug.cgi\?id=)+([0-9]{5,8})/)
      m.reply bug(a[0]) if a.length > 0
   end

   def bug(a)
     if a.length == 0
         return "You called the wrong number..."
     end
     begin
         uri = "http://bugs.kde.org/xml.cgi?id=#{a}"
         xml = @bot.httputil.get_response(uri)
         doc = Document.new xml.body
         bug = doc.root.elements["bug"]
       if xml.message == "OK"
         if bug.attributes["error"] == "NotFound"
           return "Bug #{a} was not found in Bugzilla."
         else
           str = "PRODUCT: #{bug.elements["product"].text}"
           if res = XPath.first( doc, "/bugzilla/bug/component" ) then
             str += " / #{bug.elements["component"].text}"
           end
           str += " | STATUS: #{bug.elements["bug_status"].text}"
           if res = XPath.first( doc, "/bugzilla/bug/resolution" ) then
             str += " | RESOLUTION: #{bug.elements["resolution"].text}"
           end
           str += " | URL: http://bugs.kde.org/show_bug.cgi?id=#{a} | DESCRIPTION: #{bug.elements["short_desc"].text.ircify_html}"
           return str
         end
       else
         return "Request to bugs.kde.org failed. Message not OK."
       end
     rescue
       return "Request to bugs.kde.org failed"
     end
    end

    def privmsg(m)
      unless m.params
          m.reply("incorrect usage: "+help(m.plugin))
          return
      end
      a = m.params.scan(/^#?([0-9]+)/)
      m.reply bug(a)
    end
end

plugin = KDEBugPlugin.new
plugin.register("bug")
