# Copyright (c) 2005 Diego Pettenò
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies of the Software and its documentation and acknowledgment shall be
# given in the documentation and software packages that this Software was
# used.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

require 'open-uri'

class Bugzilla
  def initialize(bughash)
    @name = bughash["name"]
    @dataurl = bughash["dataurl"]
    @reporturl = bughash["reporturl"]
    @useragent = bughash["useragent"]
  end

  def fetch(uri_str)
    puts "wgetting #{uri_str}"
    wget = IO.popen("wget -q -O - '" + uri_str + "'")
    data = wget.read
    wget.close
    return data
  end

  def summary(bugno)
    # OpenOffice's issuezilla is tricky, they call it issue_status
    status_txt = "bug_status"

    require 'uri'

    # doc = OpenURI.open_uri(@dataurl.gsub("${BUGID}", bugno))
    # bugdata = REXML::Document.new(doc)
    bugdata = REXML::Document.new(fetch(@dataurl.gsub("${BUGID}", bugno)))

    return "Unable to load bug # #{bugno}" unless bugdata

    bugxml = bugdata.root.get_elements("bug")[0]
    unless bugxml
      bugxml = bugdata.root.get_elements("issue")[0]
      status_txt = "issue_status"
    end

    return "Unable to parse bug # #{bugno}" unless bugxml

    return "Bug # #{bugno} not found" if bugxml.attribute("status_code").to_s == "404" or
      bugxml.attribute("error")

    return "Bug #{bugno}; \"#{bugxml.get_text("short_desc")}\"; #{bugxml.get_text("product")} | #{bugxml.get_text("component")}; #{bugxml.get_text(status_txt)}, #{bugxml.get_text("resolution")}; #{bugxml.get_text("reporter")} -> #{bugxml.get_text("assigned_to")}; #{@reporturl.gsub("${BUGID}", bugno)}"
  end
end

class BugzillaPlugin < Plugin
  def initialize
    super
    if FileTest.exists?(File.dirname(__FILE__) + "/bugzillas.db")
      data = File.open(File.dirname(__FILE__) + "/bugzillas.db").read
    else
      data = nil
    end

    if data
      @bugdb = Marshal.load(data)
    else
      @bugdb = Array.new
    end

    @zillas = Hash.new
    @bugdb.each { |bugzilla|
      @zillas[bugzilla["name"]] = Bugzilla.new(bugzilla)
    }

    return true
  end

  def help(plugin, topic = "")
    case plugin
      when "bug"
        return "bug <bugzilla> <number> => show the data about given bugzilla's bug."
      when "addzilla"
        return "addzilla <name> <dataurl> <reporturl> [<useragent>] => add a new bugzilla."
      when "listzilla"
        return "listzilla => shows the configured bugzillas"
    end
  end

  def privmsg(m)
    case m.plugin
      when "bug"
        unless m.params =~ /^(\w+) #?(\d+)/
          m.reply "incorrect usage. " + help(m.plugin)
          return
        end

        bugzilla = $1 # => bugzilla to search for
        bugno = $2    # => bug number to search for

        m.reply @zillas[bugzilla].summary(bugno)
      when "addzilla"
        unless m.params =~ /^(\S+)\s+(\S+)\s+(\S+)(\s+(\S+))?$/
          m.reply "incorrect usage. " + help(m.plugin)
          return
        end

        zilla = Hash.new
        zilla["name"] = $1
        zilla["dataurl"] = $2
        zilla["reporturl"] = $3
        zilla["useragent"] = $4 ? $4 : "rBot/#{$version}"

        @bugdb.push(zilla)

        Marshal.dump(@bugdb, File.open(File.dirname(__FILE__) + "/bugzillas.db", "w") )
        @zillas[zilla["name"]] = Bugzilla.new(zilla)

        m.reply "Added #{zilla["name"]}"
      when "listzilla"
        m.reply @zillas.keys.join(", ")
    end
  end
end

plugin = BugzillaPlugin.new
plugin.register("bug")
plugin.register("addzilla")
plugin.register("listzilla")

## Kate modeline: leave at the end
# kate: indent-width 2; replace-trailing-space-save 1; space-indent 1; backspace-indents 1;
