# RSS feed plugin for RubyBot
# (c) 2004 Stanislav Karchebny <berkus@madfire.net>
# Licensed under MIT License.

require 'rss/parser'
require 'rss/1.0'
require 'rss/2.0'
require 'rss/dublincore'
require 'rss/dublincore/2.0'

class RSSFeedsPlugin < Plugin

    # Keep a 1:1 relation between commands and handlers
    @@handlers = {
        "rss" => "handle_rss",
        "addrss" => "handle_addrss",
        "rmrss" => "handle_rmrss",
        "listrss" => "handle_listrss",
        "rewatch" => "handle_rewatch",
        "watchrss" => "handle_watchrss"
    }

    def initialize
        super
        puts "I'm here"
        @feeds = Hash.new
        @watchList = Hash.new
        [ ["#{@bot.botclass}/rss/feeds", @feeds], ["#{@bot.botclass}/rss/watchlist", @watchList] ].each { |set|
            if File.exists?(set[0])
                IO.foreach(set[0]) { |line|
                    s = line.chomp.split("|", 2)
                    set[1][s[0]] = s[1] if s.length==2
                }
            end
        }
    end

    def save
        Dir.mkdir("#{@bot.botclass}/rss") if not FileTest.directory?("#{@bot.botclass}/rss")
        File.open("#{@bot.botclass}/rss/feeds", "w") { |file|
            @feeds.each { |k,v|
            file.puts(k + "|" + v)
            }
        }
        File.open("#{@bot.botclass}/rss/watchlist", "w") { |file|
            @watchList.each { |url, feedFormat|
                file.puts(url + '|' + feedFormat)
            }
        }
    end


    def help(plugin,topic="")
        "RSS Reader: rss name [limit] => read a named feed [limit maximum posts, default 5], addrss [force] name url => add a feed, listrss => list all available feeds, rmrss name => remove the named feed"
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
        unless @feeds.has_key?(m.params)
            m.reply(m.params + "? whats that feed is about?")
            return
        end

        m.reply("Please wait, querying...")
        title = ''
        items = fetchRSS(m, @feeds[m.params], title)
        if(items == nil)
            return
        end
        m.reply("Channel : #{title}")
        # FIXME: optional by-date sorting if dates present
        items[0...limit].each do |item|
            printRSSItem(m,item)
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
            m.reply("But theres already a feed named #{feed[0][0]} with url #{@feeds[feed[0][0]]}")
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

    def handle_rewatch(m)
        @watchList.each{ |url, feedFormat|
            watchRss(m, url,feedFormat)
        }
    end

    def handle_watchrss(m)
        unless m.params
            m.reply "incorrect usage: " + help(m.plugin)
            return
        end
        feed = m.params.scan(/^(\S+)\s+(\S+)$/)
        url = feed[0][0]
        feedFormat = feed[0][1]
        @watchList[url] = feedFormat;
        watchRss(m, url,feedFormat)
    end
    private

    def watchRss(m, url, feedFormat)
        Thread.new {
            puts 'fetching...'
            oldItems  = []
            firstRun = true
            loop {
                title = ''
                puts 'really fetching'
                newItems = fetchRSS(m, url, title)
                if( newItems.empty? )
                    m.reply "oops"
                    break
                end
                puts "new items?"
                if(firstRun)
                    firstRun = false
                else
                    newItems.each { |nItem|
                        showItem = true;
                        oldItems.each { |oItem|
                            if(nItem.to_s == oItem.to_s)
                                showItem = false
                            end
                        }
                        if showItem
                            puts "showing #{nItem.title}"
                            printFormatedRSS(m, nItem,feedFormat)
                        else
                            puts "not showing  #{nItem.title}"
                            break
                        end
                    }
                end
                oldItems = newItems
                puts "going to sleep..."
                sleep 300
            }
        }
    end

    def printRSSItem(m,item)
        if item.kind_of?(RSS::RDF::Item)
            m.reply(item.title.chomp.riphtml.shorten(20) + " @ " + item.link)
        else
            m.reply("#{item.pubDate.to_s.chomp+": " if item.pubDate}#{item.title.chomp.riphtml.shorten(20)+" :: " if item.title}#{" @ "+item.link.chomp if item.link}")
        end
    end

    def printFormatedRSS(m,item, type)
        case type
            when 'amarokblog'
                m.reply("::#{item.category.content} just blogged at #{item.link}::")
                m.reply("::#{item.title.chomp.riphtml.shorten(20)} - #{item.description.chomp.riphtml.shorten(60)}::")
            when 'mediawiki'
                m.reply("::Wiki:: #{item.title} has been edited by #{item.dc_creator}. #{item.description.split("\n")[0].chomp.riphtml.shorten(60)}::")
                puts "mediawiki #{item.title}"
            else
                printRSSItem(m,item)
        end
    end

    def fetchRSS(m, url, title)
        begin
            xml = Utils.http_get(url)
        rescue URI::InvalidURIError, URI::BadURIError => e
            m.reply("invalid rss feed #{url}")
            return
        end
        puts 'fetched'
        unless xml
            m.reply("reading feed #{url} failed")
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
                m.reply("parsing rss stream failed, whoops =(")
                return
            end
        rescue RSS::Error
            m.reply("parsing rss stream failed, oioi")
            return
        rescue
            m.reply("processing error occured, sorry =(")
            return
        end
        items = []
        if rss.nil?
            m.reply("#{m.params} does not include RSS 1.0 or 0.9x/2.0")
        else
            begin
                rss.output_encoding = "euc-jp"
            rescue RSS::UnknownConvertMethod
                m.reply("bah! something went wrong =(")
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
            m.reply("no items found in the feed, maybe try weed?")
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