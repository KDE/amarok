# Copyright (c) 2006 Harald Sitter
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

class KDESVNPlugin < Plugin
    def help(plugin,topic="")
	"svn <path>/<revision#> - outputs websvn.kde.org URL of given path or revision"
    end

    def privmsg(m)
	unless m.params
	    m.reply("incorrect usage: "+help(m.plugin))
	    return
	end
	revision = m.params.scan(/^#?([0-9]+)/)
	if revision.length == 0
	    revision = m.params.scan(/^#?[bt].{4,}/)
	        if revision.length == 0
	            m.reply("Input not processible")
	            return
		else
		    m.reply("http://websvn.kde.org/#{revision}")
	            return
		end
	end
	str = "http://websvn.kde.org/?rev=#{revision}&view=rev"
	m.reply(str)
    end
end

plugin = KDESVNPlugin.new
plugin.register("revision")
plugin.register("svn")
