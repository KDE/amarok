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
#
# NOTE: for description of beats see 'calc beats' in #amaork
# ______   _______    _        _______ _________
#(  __  \ (  ___  )  ( (    /|(  ___  )\__   __/
#| (  \  )| (   ) |  |  \  ( || (   ) |   ) (   
#| |   ) || |   | |  |   \ | || |   | |   | |   
#| |   | || |   | |  | (\ \) || |   | |   | |   
#| |   ) || |   | |  | | \   || |   | |   | |   
#| (__/  )| (___) |  | )  \  || (___) |   | |   
#(______/ (_______)  |/    )_)(_______)   )_(   
#                                               
#_________ _______           _______          
#\__   __/(  ___  )|\     /|(  ____ \|\     /|
#   ) (   | (   ) || )   ( || (    \/| )   ( |
#   | |   | |   | || |   | || |      | (___) |
#   | |   | |   | || |   | || |      |  ___  |
#   | |   | |   | || |   | || |      | (   ) |
#   | |   | (___) || (___) || (____/\| )   ( |
#   )_(   (_______)(_______)(_______/|/     \|
#                                             
#          _______ _________ _  _  _ 
#|\     /|(  ____ \\__   __/( )( )( )
#( \   / )| (    \/   ) (   | || || |
# \ (_) / | (__       | |   | || || |
#  \   /  |  __)      | |   | || || |
#   ) (   | (         | |   (_)(_)(_)
#   | |   | (____/\   | |    _  _  _ 
#   \_/   (_______/   )_(   (_)(_)(_)

Stock = Struct.new( "Stock", :amount, :max, :beats, :deliverybeat, :version )

class BAREXTPlugin < Plugin

  def initialize()
    super
    if @registry.has_key?(:stock)
      @stock = @registry[:stock]
    else
      @stock = Hash.new
    end
  end

  def save
    @registry[:stock] = @stock
  end

  def handle_add( m, params )
    name         = params[:name].downcase
    amount       = params[:amount]
    beats        = params[:beats]
    version      = "1"

    beats = beats.to_f / amount.to_f
    beats *= 100

    stock = Stock.new( amount, amount, beats, nil, version )
    @stock[name] = stock

    m.reply( "done" )
  end

  def handle_del( m, params )
    name = params[:name].downcase
    unless @stock.has_key?( name )
      m.reply( "Not in registry." ); return
    end
    @stock.delete(name)
      @registry[:stock] = @stock
    m.reply( "done" )
  end

  def handle_list( m, params )
    if @stock.length == 0
      m.reply( "Nothing available." ); return
    end

    m.reply "Finite goods: #{@stock.keys.sort.join(', ')}" 
  end

  def handle_show( m, params )
    name = params[:name].downcase

    unless @stock.has_key?( name )
      m.reply( "#{name} does not exist." ); return
    end

    cmd = @stock[name]
    m.reply( "Amount: #{cmd.amount} ||| Max: #{cmd.max} ||| 1 good per #{cmd.beats} beats ||| #{if @stock[name].deliverybeat != nil; @stock[name].deliverybeat; end } ||| Tag version: #{cmd.version}" )
  end

  def handle_reorder( m, params )
    name         = params[:name].downcase
    beat_cur     = Time.new.strftime '%Y%j%H%M%S'

    unless @stock.has_key?( name )
      m.reply( "#{name} does not exist." ); return
    end

    if @stock[name].deliverybeat != nil
      m.reply( "#{m.sourcenick}: #{name} has already been ordered." ); return
#       m.reply( "#{m.sourcenick}: #{name} has already been ordered (#{(@stock[name].deliverybeat.to_i - beat_cur.to_i).to_s} minutes)." ); return
    end

    if @stock[name].amount.to_f >= ((@stock[name].max.to_f * 25) / 100)
      m.reply( " #{m.sourcenick}: We still have enough #{name}, no need to reorder." ); return
    end

    # run calculation after checks to save resources if not needed
    deliverybeat = beat_cur.to_i + (@stock[name].beats.to_i * @stock[name].max.to_i)

    @stock[name].deliverybeat = deliverybeat
    @registry[:stock] = @stock

    m.reply( "Billy Kay is on it's way to the supplier....")

#     NOTE: needs new registry hashes for beat per amount and for beat at time
#           of order
#     TODO: implement amount option
  end

  def handle_reorder_reset( m, params )
    name = params[:name].downcase

    unless @stock.has_key?( name )
      m.reply( "#{name} does not exist." ); return
    end

    @stock[name].deliverybeat = nil
    @registry[:stock]         = @stock

    m.reply( "done" )
  end

#   def delivery
#     beat_cur   = Time.new.strftime '%Y%j%H%M%S'
#     beat_order = @orderbeat[name]
#     beat_diff  = beat_cur.to_i - beat_order.to_i
# m.reply("1")
# 
#     if beat_diff >= 500
# m.reply("2")
#       available = true
#       @orderbeat.delete(name)
#     else
# m.reply("3")
#       available = false
#     end
# m.reply("5")
# 
# #     TODO: compare current beat with order beat and decied whether to increase
# #           the amount to amount of order or not. If not, tell the user how long
# #           to wait.
#   end

def process( m, params )

# m.reply("working")

order  = nil
available = false

    scantmp = m.params.scan(/.*for.*/)
    if scantmp.length == 0
      name = m.params
      nick = m.sourcenick
    else
      for_idx = m.params =~ /\s+\bfor\b/
      name = m.params[0, for_idx]

      for_idx = m.params.reverse =~ /\s+\brof\b/
      nick = m.params.reverse[0, for_idx]
      nick = nick.reverse
    end
    name = name.downcase

if @stock.has_key?( name )
  if @stock[name].amount.to_f <= 0
    if @stock[name].deliverybeat != nil
      beat_cur     = Time.new.strftime '%Y%j%H%M%S'
      deliverybeat = @stock[name].deliverybeat
        if beat_cur.to_f >= deliverybeat.to_f
          @stock[name].amount       = @stock[name].max
          @stock[name].deliverybeat = nil
          @registry[:stock]         = @stock
          available                 = true
        else
          togo = (deliverybeat.to_f - beat_cur.to_f) / 100
          available = false
        end
      if available != true
        order = "tells #{nick} that we are out of #{name} but a new delivery is already in progress (#{togo.to_s} minutes)."
        @bot.action m.replyto, order; return
      end
    else
      m.reply( "#{nick}: We are out of #{name}, you should reorder some of it."); return
    end
  end
  @stock[name].amount = @stock[name].amount.to_i - 1
end

if nick == "everyone"
  order = "is goning to his secret storehouse to get #{name} for #{nick} - might be expansive."
  @bot.action m.replyto, order
  @stock[name].amount = "0"
  return
end

# unless order != nil
    order = case name
             # please order alphabetically or we'll all end up in hell :S
             when "bed" then
                 "is placing a cot for #{nick} in the corner of #{m.target}."
             when "beer" then
                 "gives #{name} a nice frosty mug of #{name}."
             when "brain" then
                 "shouts: OMG!!!!! RED ALERT! We lost a #{name}. Get me a medic, NOW!"
             when "can of whoopass" then
                  "opens up a can of whoopass on #{nick}. Chuck Norris jumps out."
             when "chocolate" then
                 "hands #{nick} a bar of Noir de Noir."
             when "coffee" then
                 "slides #{name} with milk down the bar to #{nick}."
             when "cushion" then
                 "is getting #{nick} a cushion from the chill area of #amarok."
             when "heineken" then
                 "gives #{nick} a nice frosty wonderful green bottle of #{name}."
             when "good beer" then
                 "slides the finest Belgium Trappist beer down the bar to #{nick}."
             when "kubuntu" then
                 "tells #{nick} to better use http://shipit.kubuntu.org"
             when "tea" then
                 "gives #{nick} a nice hot cup of #{name}."
             when "wine" then
                 "pours #{nick} a delicious glass from the channel's wine cellar."

             else
                 "slides #{name} down the bar to #{nick}"
            end

# end

    @bot.action m.replyto, order

  end
end

plugin = BAREXTPlugin.new
plugin.register("order2")


plugin.map 'order2 *wish',                       :action => 'process'
# plugin.map 'order :wish for *nick',              :action => 'process'
plugin.map 'order-adm add :name :amount :beats', :action => 'handle_add',    :auth => 'brain'
# TODO: point out how to make it work with 2 or more word names
plugin.map 'order-adm del :name',                :action => 'handle_del',    :auth => 'brain'
plugin.map 'order-adm list',                     :action => 'handle_list'
plugin.map 'order-adm show :name',               :action => 'handle_show'
plugin.map 'reorder :name',                      :action => 'handle_reorder'
plugin.map 'reorder reset :name',                :action => 'handle_reorder_reset'

