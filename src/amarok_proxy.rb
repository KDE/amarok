#!/usr/bin/env ruby
#
# Proxy server for last.fm. Relays the stream from the last.fm server to localhost.
#
# (c) 2006 Paul Cifarelli <paul@cifarelli.net>
# (c) 2006 Mark Kretschmann <markey@web.de>
# (c) 2006 Michael Fellinger <manveru@weez-int.com>
#
# License: GNU General Public License V2

require "net/http"
require 'socket'
require "uri"

include Socket::Constants

class LastFM
  def initialize port, remote_url

    puts( "startup" )

    #
    # amarok is, well, Amarok
    #
    amarok  = TCPServer.new( port )
    amaroks = amarok.accept

    #
    # serv is the lastfm stream server
    #
    uri = URI.parse( remote_url )
    serv = TCPSocket.new( uri.host, uri.port )
    puts "running with port: #{uri.port} and host: #{uri.host}"

    # here we substitute the proxy GET
    data = amaroks.readline
    get = "GET #{uri.path || '/'}?#{uri.query} HTTP/1.1\r\n"

    puts data.inspect
    puts get.inspect
    puts "#{data} but sending #{get}"
    serv.puts get

    # the client initiates everything - so copy it to the server
    puts "COPY from amarok -> serv"
    useragent = cp_to_empty amaroks, serv

    puts "COPY from serv -> amarok"
    cp_to_empty serv, amaroks

    # now copy from the server to amarok
    puts "Before cp_all()"
    cp_all serv, amaroks

    puts "useragent is #{useragent}"
    if !useragent.include?( "xine" ) && amaroks.eof
      puts "EOF Detected, reconnecting"
      amaroks = amarok.accept
      cp_all( serv, amaroks )
    end
  end

  def puts string
    $stderr.puts "AMAROK_PROXY: #{string}"
  end

  def safe_write(output, data)
    begin
      output.write data
    rescue
      puts "error from output.write, #{$!}"
      puts $!.backtrace.inspect
      break
    end
  end

  def cp_to_empty( income, output )
    puts "cp_to_empty( income => #{income.inspect}, output => #{output.inspect}"
    income.each_line do |data|
      puts (data = data.chomp)
      if data.empty?
        label, useragent = data.split(/\s*\:\s*/) if data =~ /User-Agent:/
        puts [label, useragent].inspect
        return "\n#{useragent}"
      end
      safe_write output, data
    end
  end

  def cp_all( income, output )
    puts "cp_all( income => #{income.inspect}, output => #{output.inspect}"
    loop do
      data = income.read( 1000 )
      unless data.empty?
        # takes double memory (2000), but makes safe that no sync comes through
        # i think that's a good trade-off ;)
        if (curr = (last ||= '') + data) =~ /SYNC/
          data = curr.split('SYNC').last
          puts 'SYNC'
        end
        safe_write output, data
      end
      last = data
    end
  end
end

begin
  puts 'startup'
  port, remote_url = ARGV
  puts "running with port: #{port} and url: #{remote_url}"

  LastFM.new port, remote_url
rescue
  $stderr.puts $!.to_s
  $stderr.puts $!.backtrace.inspect
end

puts( "exiting" )