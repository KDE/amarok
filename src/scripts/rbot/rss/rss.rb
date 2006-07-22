# RSS feed plugin for RubyBot
# (c) 2004 Stanislav Karchebny <berkus@madfire.net>
# (c) 2005 Ian Monroe <ian@monroe.nu>
# (c) 2005 Mark Kretschmann <markey@web.de>
# Licensed under MIT License.

require 'rss/parser'
require 'rss/1.0'
require 'rss/2.0'
require 'rss/dublincore'
require 'rss/dublincore/2.0'

class ::String
  def shorten(limit)
    if self.length > limit
      self+". " =~ /^(.{#{limit}}[^.!;?]*[.!;?])/mi
      return $1
    end
    self
  end

  def riphtml
    self.gsub(/<[^>]+>/, '').gsub(/&amp;/,'&').gsub(/&quot;/,'"').gsub(/&lt;/,'<').gsub(/&gt;/,'>').gsub(/&ellip;/,'...').gsub(/&apos;/, "'").gsub("\n",'')
  end

  def mysqlize
    self.gsub(/'/, "''")
  end
end


class RSSFeedsPlugin < Plugin
  @@watchThreads = Hash.new

  # Keep a 1:1 relation between commands and handlers
  @@handlers = {
    "rss" => "handle_rss",
    "addrss" => "handle_addrss",
    "rmrss" => "handle_rmrss",
    "listrss" => "handle_listrss",
    "listwatches" => "handle_listrsswatch",
    "rewatch" => "handle_rewatch",
    "watchrss" => "handle_watchrss",
    "rmwatch" => "handle_rmwatch"
  }

  def initialize
    super
    puts "I'm here"
    @feeds = Hash.new
    @watchList = Hash.new
    begin
      IO.foreach("#{@bot.botclass}/rss/feeds") { |line|
        s = line.chomp.split("|", 2)
        @feeds[s[0]] = s[1]
      }
    rescue
      puts "no feeds";
    end
    begin
      IO.foreach("#{@bot.botclass}/rss/watchlist") { |line|
        s = line.chomp.split("|", 3)
        @watchList[s[0]] = [s[1], s[2]]
        watchRss( s[2], s[0], s[1] )
      }
    rescue
      puts "no watchlist"
    end

  end

  def cleanup
    kill_threads
  end

  def save
    Dir.mkdir("#{@bot.botclass}/rss") if not FileTest.directory?("#{@bot.botclass}/rss")
    File.open("#{@bot.botclass}/rss/feeds", "w") { |file|
      @feeds.each { |k,v|
        file.puts(k + "|" + v)
      }
    }
    File.open("#{@bot.botclass}/rss/watchlist", "w") { |file|
      @watchList.each { |url, d|
        feedFormat = d[0] || ''
        whichChan = d[1] || 'markey'
        file.puts(url + '|' + feedFormat + '|' + whichChan)
      }
    }
  end

  def kill_threads
    Thread.critical=true
    # Abort all running threads.
    @@watchThreads.each { |url, thread|
      puts "Killing thread for #{url}"
      thread.kill
    }
    #         @@watchThreads.each { |url, thread|
    #             puts "Joining on killed thread for #{url}"
    #             thread.join
    #         }
    @@watchThreads = Hash.new
    Thread.critical=false
  end

  def help(plugin,topic="")
    "RSS Reader: rss name [limit] => read a named feed [limit maximum posts, default 5], addrss [force] name url => add a feed, listrss => list all available feeds, rmrss name => remove the named feed, watchrss url [type] => watch a rss feed for changes (type may be 'amarokblog', 'amarokforum', 'mediawiki', 'gmame' or empty - it defines special formatting of feed items), rewatch => restart all rss watches, rmwatch url => stop watching for changes in url, listwatches => see a list of watched feeds"
  end

  def privmsg(m)
    meth = self.method(@@handlers[m.plugin])
    meth.call(m)
  end

  def handle_rss(m)
    unless m.params
      m.reply("incorrect usage: " + help(m.plugin))
      return
    end
    limit = 5
    if m.params =~ /\s+(\d+)$/
      limit = $1.to_i
      if limit < 1 || limit > 15
        m.reply("weird, limit not in [1..15], reverting to default")
        limit = 5
      end
      m.params.gsub!(/\s+\d+$/, '')
    end

    url = ''
    if m.params =~ /^http:\/\//
      url = m.params
    else
      unless @feeds.has_key?(m.params)
        m.reply(m.params + "? what is that feed about?")
        return
      end
      url = @feeds[m.params]
    end

    m.reply("Please wait, querying...")
    title = ''
    items = fetchRSS(m.replyto, url, title)
    if(items == nil)
      return
    end
    m.reply("Channel : #{title}")
    # FIXME: optional by-date sorting if dates present
    items[0...limit].each do |item|
      printRSSItem(m.replyto,item)
    end
  end

  def handle_addrss(m)
    unless m.params
      m.reply "incorrect usage: " + help(m.plugin)
      return
    end
    if m.params =~ /^force /
      forced = true
      m.params.gsub!(/^force /, '')
    end
    feed = m.params.scan(/^(\S+)\s+(\S+)$/)
    unless feed.length == 1 && feed[0].length == 2
      m.reply("incorrect usage: " + help(m.plugin))
      return
    end
    if @feeds.has_key?(feed[0][0]) && !forced
      m.reply("But there is already a feed named #{feed[0][0]} with url #{@feeds[feed[0][0]]}")
      return
    end
    feed[0][0].gsub!("|", '_')
    @feeds[feed[0][0]] = feed[0][1]
    m.reply("RSS: Added #{feed[0][1]} with name #{feed[0][0]}")
  end

  def handle_rmrss(m)
    unless m.params
      m.reply "incorrect usage: " + help(m.plugin)
      return
    end
    unless @feeds.has_key?(m.params)
      m.reply("dunno that feed")
      return
    end
    @feeds.delete(m.params)
    @bot.okay(m.replyto)
  end

  def handle_rmwatch(m)
    unless m.params
      m.reply "incorrect usage: " + help(m.plugin)
      return
    end
    unless @watchList.has_key?(m.params)
      m.reply("no such watch")
      return
    end
    unless @watchList[m.params][1] == m.replyto
      m.reply("no such watch for this channel/nick")
      return
    end
    @watchList.delete(m.params)
    Thread.critical=true
    if @@watchThreads[m.params].kind_of? Thread
      @@watchThreads[m.params].kill
      puts "rmwatch: Killed thread for #{m.params}"
      #             @@watchThreads[m.params].join
      #             puts "rmwatch: Joined killed thread for #{m.params}"
      @@watchThreads.delete(m.params)
    end
    Thread.critical=false
    @bot.okay(m.replyto)
  end

  def handle_listrss(m)
    reply = ''
    if @feeds.length == 0
      reply = "No feeds yet."
    else
      @feeds.each { |k,v|
        reply << k + ": " + v + "\n"
      }
    end
    m.reply(reply)
  end

  def handle_listrsswatch(m)
    reply = ''
    if @watchList.length == 0
      reply = "No watched feeds yet."
    else
      @watchList.each { |url,v|
        reply << url + " for " + v[1] + " (in format: " + (v[0]?v[0]:"default") + ")\n"
      }
    end
    m.reply(reply)
  end

  def handle_rewatch(m)
    kill_threads

    # Read watches from list.
    @watchList.each{ |url, d|
      feedFormat = d[0]
      whichChan = d[1]
      watchRss(whichChan, url,feedFormat)
    }
    @bot.okay(m.replyto)
  end

  def handle_watchrss(m)
    unless m.params
      m.reply "incorrect usage: " + help(m.plugin)
      return
    end
    feed = m.params.scan(/^(\S+)\s+(\S+)$/)
    url = feed[0][0]
    feedFormat = feed[0][1]
    if @watchList.has_key?(url)
      m.reply("But there is already a watch for feed #{url} on chan #{@watchList[url][1]}")
      return
    end
    @watchList[url] = [feedFormat, m.replyto]
    watchRss(m.replyto, url,feedFormat)
    @bot.okay(m.replyto)
  end

  private
  def watchRss(whichChan, url, feedFormat)
    if @@watchThreads.has_key?(url)
      @bot.say whichChan, "ERROR: watcher thread for #{url} is already running! #{@@watchThreads[url]}"
      return
    end
    @@watchThreads[url] = Thread.new do
      puts 'watchRss thread started.'
      oldItems = []
      firstRun = true
      loop do
        begin # exception
          title = ''
          puts 'Fetching rss feed..'
          newItems = fetchRSS(whichChan, url, title)
          if( newItems.empty? )
            @bot.say whichChan, "Oops - Item is empty"
            break
          end
          puts "Checking if new items are available"
          if (firstRun)
            firstRun = false
          else
            newItems.each do |nItem|
              showItem = true;
              oldItems.each do |oItem|
                if (nItem.to_s == oItem.to_s)
                  showItem = false
                end
              end
              if showItem
                puts "showing #{nItem.title}"
                printFormatedRSS(whichChan, nItem,feedFormat)
              else
                puts "not showing  #{nItem.title}"
                break
              end
            end
          end
          oldItems = newItems
        rescue Exception
          $stderr.print "IO failed: " + $! + "\n"
        end

        seconds = 150 + rand(100)
        puts "Thread going to sleep #{seconds} seconds.."
        sleep seconds
      end
    end
  end

  def printRSSItem(whichChan,item)
    if item.kind_of?(RSS::RDF::Item)
      @bot.say whichChan, item.title.chomp.riphtml.shorten(20) + " @ " + item.link
    else
      @bot.say whichChan, "#{item.pubDate.to_s.chomp+": " if item.pubDate}#{item.title.chomp.riphtml.shorten(20)+" :: " if item.title}#{" @ "+item.link.chomp if item.link}"
    end
  end

  def printFormatedRSS(whichChan,item, type)
    case type
    when 'amarokblog'
      @bot.say whichChan, "::#{item.category.content} just blogged at #{item.link}::"
      @bot.say whichChan, "::#{item.title.chomp.riphtml} - #{item.description.chomp.riphtml.shorten(60)}::"
    when 'amarokforum'
      @bot.say whichChan, "::Forum:: #{item.pubDate.to_s.chomp+": " if item.pubDate}#{item.title.chomp.riphtml+" :: " if item.title}#{" @ "+item.link.chomp if item.link}"
    when 'mediawiki'
      @bot.say whichChan, "::Wiki:: #{item.title} has been edited by #{item.dc_creator}. #{item.description.split("\n")[0].chomp.riphtml.shorten(60)} #{item.link} ::"
      puts "mediawiki #{item.title}"
    when "gmame"
      @bot.say whichChan, "::amarok-devel:: Message #{item.title} sent by #{item.dc_creator}. #{item.description.split("\n")[0].chomp.riphtml.shorten(60)}::"
    else
      printRSSItem(whichChan,item)
    end
  end

  def fetchRSS(whichChan, url, title)
    begin
      # Use 60 sec timeout, cause the default is too low
      xml = Utils.http_get(url,60,60)
    rescue URI::InvalidURIError, URI::BadURIError => e
      @bot.say whichChan, "invalid rss feed #{url}"
      return
    end
    puts 'fetched'
    unless xml
      @bot.say whichChan, "reading feed #{url} failed"
      return
    end

    begin
      ## do validate parse
      rss = RSS::Parser.parse(xml)
      puts 'parsed'
    rescue RSS::InvalidRSSError
      ## do non validate parse for invalid RSS 1.0
      begin
        rss = RSS::Parser.parse(xml, false)
      rescue RSS::Error
        @bot.say whichChan, "parsing rss stream failed, whoops =("
        return
      end
    rescue RSS::Error
      @bot.say whichChan, "parsing rss stream failed, oioi"
      return
    rescue
      @bot.say whichChan, "processing error occured, sorry =("
      return
    end
    items = []
    if rss.nil?
      @bot.say whichChan, "#{m.params} does not include RSS 1.0 or 0.9x/2.0"
    else
      begin
        rss.output_encoding = "euc-jp"
      rescue RSS::UnknownConvertMethod
        @bot.say whichChan, "bah! something went wrong =("
        return
      end
      rss.channel.title ||= "Unknown"
      title.replace(rss.channel.title)
      rss.items.each do |item|
        item.title ||= "Unknown"
        items << item
      end
    end

    if items.empty?
      @bot.say whichChan, "no items found in the feed, maybe try weed?"
      return
    end
    return items
  end
end

plugin = RSSFeedsPlugin.new
plugin.register("rss")
plugin.register("addrss")
plugin.register("rmrss")
plugin.register("listrss")
plugin.register("rewatch")
plugin.register("watchrss")
plugin.register("listwatches")
plugin.register("rmwatch")
