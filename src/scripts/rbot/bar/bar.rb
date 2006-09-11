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

# Class for storing good values
Stock = Struct.new( "Stock", :amount, :max, :beats, :deliverybeat, :version )

# Class for storing costum replies
Reply = Struct.new( "Reply", :reply, :version )


class BARPlugin < Plugin

  ####
  # Initialize Database
  ####
  def initialize()
    super
    if @registry.has_key?(:stock)
      @stock = @registry[:stock]
    else
      @stock = Hash.new
    end

    if @registry.has_key?(:reply)
      @reply = @registry[:reply]
    else
      @reply = Hash.new
    end
  end

  ####
  # Save Database
  ####
  def save
    @registry[:stock] = @stock
    @registry[:reply] = @reply
  end

  ####
  # Get Delivery status
  ####
  def delivery( m, name )
    beat_cur     = Time.new.strftime '%Y%j%H%M%S'
    deliverybeat = @stock[name].deliverybeat

    if beat_cur.to_f >= deliverybeat.to_f
      @stock[name].amount       = @stock[name].max
      @stock[name].deliverybeat = nil
      @registry[:stock]         = @stock
      @available                 = true
      # if it's not available, set value according
    else
      @togo = (deliverybeat.to_f - beat_cur.to_f) / 100
      @available = false
    end

    #     TODO: compare current beat with order beat and decied whether to increase
    #           the amount to amount of order or not. If not, tell the user how long
    #           to wait.
  end

  ####
  # Check whether good is in database
  ####
  def status( name, reg )
    @inreg = true

    unless reg.has_key?( name )
      @inreg = false
    end
  end

################################################################################

  ####
  # Add new Good to Database
  ####
  def handle_add( m, params )
    # lame auth check
    # TODO: need to get a look into new auth system
    if m.sourcenick.to_s != "apachelogger"
      m.reply( "sorry, not yet available for public use" )
      return
    end

    name         = params[:name].downcase
    amount       = params[:amount]
    beats        = params[:beats]
    # temp check whether values got entered in proper order etc.
    if amount.to_s.scan(/^([0-9]+)/).length == 0
      m.reply( "You have to define amount and beats - note that items which names consits of more than 2 words don't work yet")
      return
    end
    # !!!tag version!!!
    version      = "1"

    # caluclate beats for one good and multiply with 100 -> better for later use
    beats = beats.to_f / amount.to_f
    beats *= 100

    # current amount || maximal amount || beats for one good || deliverybeat (always 'nil' if not reordered) || tag version
    stock = Stock.new( amount, amount, beats, nil, version )
    @stock[name] = stock

    m.reply( "done" )
  end

  ####
  # Add reply for defined Good
  ####
  def handle_add( m, params )
    # lame auth check
    # TODO: need to get a look into new auth system
    if m.sourcenick.to_s != "apachelogger"
      m.reply( "sorry, not yet available for public use" )
      return
    end

    name    = params[:name].downcase
    reply   = params[:reply]
    version = "1"  # !!!tag version!!!

    # check whether good is in database
    status( name, @stock )
    if @inreg == false
      m.reply( "#{name} does not exist." ); return
    end

    # Put data into database, should be understandable ^^
    stock = Reply.new( reply, version )
    @reply[name] = reply

    m.reply( "done" )
  end

  ####
  # Delete Good from Database
  ####
  def handle_del( m, params )
    name = params[:name].downcase.sub(/[ ]/, '_')
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

  ####
  # Show information about Good (amount, max, beats, deliverybeat, tag version)
  ####
  def handle_show( m, params )
    name = params[:name].downcase

    unless @stock.has_key?( name )
      m.reply( "#{name} does not exist." ); return
    end

    cmd = @stock[name]
    m.reply( "Amount: #{cmd.amount} ||| Max: #{cmd.max} ||| 1 good per #{cmd.beats} beats ||| #{if @stock[name].deliverybeat != nil; @stock[name].deliverybeat; end } ||| Tag version: #{cmd.version}" )
  end

  ####
  # Show costum reply
  ####
  def handle_show_reply( m, params )
    name = params[:name].downcase

    status( name, @reply )
    if @inreg == false
      m.reply( "#{name} does not exist." ); return
    end

    cmd = @reply[name]
    m.reply( "Reply: #{cmd.reply} ||| Tag version: #{cmd.version}" )
  end

  ####
  # Process reording of Good (calculate deliverybeat and set it)
  ####
  def handle_reorder( m, params )
    name         = params[:name].downcase
    #     cur_y        = Time.new.strftime '%Y'
    #     cur_d        = Time.new.strftime '%j'
    #     cur_h        = Time.new.strftime '%H'
    #     cur_m        = Time.new.strftime '%M'
    #     cur_s        = Time.new.strftime '%S'
    #     cur_beat     = ((cur_y.to_i * 31536000) + (cur_d.to_i * 86400) + (cur_h.to_i * 3600) + (cur_m.to_i * 60) + cur_s.to_i)
    beat_cur     = Time.new.strftime '%Y%j%H%M%S'

    unless @stock.has_key?( name )
      m.reply( "#{name} does not exist." ); return
    end

    if @stock[name].deliverybeat != nil
      delivery( m, name )
      if @available == true
        m.reply( "#{m.sourcenick}: #{name} is again available now." ); return
      else
        m.reply( "#{m.sourcenick}: #{name} has already been ordered (#{@togo.to_s} minutes)." ); return
      end
    end

    if @stock[name].amount.to_f >= ((@stock[name].max.to_f * 25) / 100)
      m.reply( " #{m.sourcenick}: We still have enough #{name}, no need to reorder." ); return
    end

    # run calculation after checks to save resources if not needed
    deliverybeat = beat_cur.to_i + (@stock[name].beats.to_i * @stock[name].max.to_i)

    @stock[name].deliverybeat = deliverybeat
    @registry[:stock] = @stock

    m.reply( "Billy Kay is on his way to the supplier....")

    #     NOTE: needs new registry hashes for beat per amount and for beat at time
    #           of order
    #     TODO: implement amount option
  end

  ####
  # Reset reording of Good (set deliverybeat to nil)
  ####
  def handle_reorder_reset( m, params )
    name = params[:name].downcase

    unless @stock.has_key?( name )
      m.reply( "#{name} does not exist." ); return
    end

    @stock[name].deliverybeat = nil
    @registry[:stock]         = @stock

    m.reply( "done" )
  end

  ####
  # Main Process, takes care of usual order cmd (check availability, delivery status, special order etc.)
  ####
  def process( m, params )

    order      = nil
    @available = false
    @togo      = nil

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

    # check whether the order is in our database
    if @stock.has_key?( name )
      # if the order is in our db, check whether we still have some of it - if yes: reduze amount by one
      if @stock[name].amount.to_f <= 0
        # if we don't have the order anymore - check whether the deliverybeat equals nil, if this is case tell the user to reorder
        if @stock[name].deliverybeat != nil
          # check whether the reordered good is available - if not, tell the user it's in progress and how long to go
          delivery( m, name )
          if @available != true
            order = "tells #{nick} that we are out of #{name} but a new delivery is already in progress (#{@togo.to_s} minutes)."
            @bot.action m.replyto, order; return
          end
          # tell _the ordering user_ to reorder because we are out of the ordered good
        else
          m.reply( "#{m.sourcenick}: We are out of #{name}, you should reorder some of it."); return
        end
      end
      # decrease amount of ordered good by one
      @stock[name].amount = @stock[name].amount.to_i - 1
    end

    # !!!! special feature if someone stands something for everyone !!!!
    if nick == "everyone"
      order = "is going to his secret storehouse to get #{name} for #{nick} - might be expensive."
      @bot.action m.replyto, order
      # !!!! result: amount is going down to 0 no matter which value it was before
      @stock[name].amount = "0"
      return
    end

    order = case name
      # please keep alphabetical order!!!!!!!
    when "bed" then
      "is placing a cot for #{nick} in the corner of #{m.target}."
    when "beer" then
      "gives #{name} a nice frosty mug of #{name}."
    when "birthday package" then
      "is running to the corner shop to get a birthday present."
    when "breakfast" then
      "slides a cigarette, a cup of hot coffee and a bagel with cream cheese down the bar to #{nick}."
    when "breakfast, au" then
      "slides a slab of bread with some mixture of salt and battery acid, called vegemite, and a glass of water down the bar to #{nick}."
    when "breakfast, at" then
      "slides 5 floors of backon and some wurst with bread, a glass of schnaps and a new deck down the bar to #{nick}."
    when "breakfast, de" then
      "slides a noppers down the bar to #{nick}."
    when "breakfast, hangover" then
      "slides 2 liter of water, one glass of tomato juice, 3 rollmops and chips down the bar to #{nick}"
    when "breakfast, uk, full" then
      "slides scrambled eggs, bacon, one sausage, black pudding, mushrooms, baked beans, hash browns, half of a tomato, one toast and a tea with fresh milk down the bar to #{nick}."
    when "breakfast, us" then
      "slides breakfast cereals, a bagel, 2 pancakes, scrambled eggs and a cup of hot coffee down the bar to #{nick}."
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
    when "release" then
      "pokes apachelogger."
    when "tea" then
      "gives #{nick} a nice hot cup of #{name}."
    when "tea, earl grey, hot" then
      "is replicating a hot cup of earl grey for captain #{nick}."
    when "wine" then
      "pours #{nick} a delicious glass from the channel's wine cellar."
    else
      "slides #{name} down the bar to #{nick}"
    end

    @bot.action m.replyto, order
    if name == "birthday package"
      @bot.action m.replyto, "slides a birthday cake and a present down the bar to #{nick} and gives everyone a nice frosty mug of beer."
      m.reply( "Happy birthday to you, happy birthday to you, happy birthday #{nick}, happy birthday to you!!!! - Wooooho!" )
      m.reply( "\002Happy Birthday #{nick} :D" )
      m.reply( "To your health!" )
    end

  end
end

plugin = BARPlugin.new
plugin.register("order")


plugin.map 'order *wish',                       :action => 'process'
# plugin.map 'order :wish for *nick',              :action => 'process'
plugin.map 'order-adm add :name :amount :beats', :action => 'handle_add',    :auth => '80'
# TODO: point out how to make it work with 2 or more word names
plugin.map 'order-adm del :name',                :action => 'handle_del',    :auth => 'brain'
plugin.map 'order-adm list',                     :action => 'handle_list'
plugin.map 'order-adm show :name',               :action => 'handle_show'
# plugin.map 'order-adm show-reply :name',         :action => 'handle_show_reply'
plugin.map 'reorder :name',                      :action => 'handle_reorder'
plugin.map 'reorder reset :name',                :action => 'handle_reorder_reset'
