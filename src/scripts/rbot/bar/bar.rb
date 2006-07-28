# Copyright (c) 2006 Harald Sitter <harald.sitter@kdemail.net>
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

class BARPlugin < Plugin

  def name
    "order"
  end

  def help(plugin,topic="")
    "special items you can order (might not be all): beer, brain, can of whoopass, chocolate, coffee, cushion, heineken, good beer, kubuntu, tea, wine"
  end

  def privmsg(m)
    unless m.params
      m.reply("incorrect usage: "+help(m.plugin))
      return
    end

    scantmp = m.params.scan(/.*for.*/)

    if scantmp.length == 0
      wish = m.params
      nick = m.sourcenick
    else
      for_idx = m.params =~ /\s+\bfor\b/
      wish = m.params[0, for_idx]

      for_idx = m.params.reverse =~ /\s+\brof\b/
      nick = m.params.reverse[0, for_idx]
      nick = nick.reverse
    end

    order = case wish.downcase
             # please order alphabetically or we'll all end up in hell :S
             when "beer" then
                 "gives #{wish} a nice frosty mug of #{wish}."
             when "brain" then
                 "shouts: OMG!!!!! RED ALERT! We lost a #{wish}. Get me a medic, NOW!"
             when "can of whoopass" then
                  "opens up a can of whoopass on #{nick}. Chuck Norris jumps out."
             when "chocolate" then
                 "hands #{nick} a bar of Noir de Noir."
             when "coffee" then
                 "slides #{wish} with milk down the bar to #{nick}."
             when "cushion" then
                 "is getting #{nick} a cushion from the chill area of #amarok."
             when "heineken" then
                 "gives #{nick} a nice frosty wonderful green bottle of #{wish}."
             when "good beer" then
                 "slides the finest Belgium Trappist beer down the bar to #{nick}."
             when "kubuntu" then
                 "tells #{nick} to better use http://shipit.kubuntu.org"
             when "tea" then
                 "gives #{nick} a nice hot cup of #{wish}."
             when "wine" then
                 "pours #{nick} a delicious glass from the channel's wine cellar."

             else
                 "slides #{wish} down the bar to #{nick}"
            end

    @bot.action m.replyto, order

  end
end

plugin = BARPlugin.new
plugin.register("order")
