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
    puts "running with port: #{port} and url: #{remote_url}"

    #
    # amarok is, well, Amarok
    #
    amarok  = TCPServer.new( port )
    amaroks = amarok.accept

    #
    # serv is the lastfm stream server
    #
    uri = URI.parse( remote_url )
    puts("host " << uri.host << " ")
    puts ( port )
    serv = TCPSocket.new( uri.host, uri.port )
    puts "running with port: #{uri.port} and host: #{uri.host}"

    # here we substitute the proxy GET
    data = amaroks.readline
    get = "GET #{uri.path || '/'}?#{uri.query} HTTP/1.1\r\n\r\n"
    #get = "GET " << uri.path
    #get << "/" if uri.path == ""
    #get << "?" << uri.query if uri.query && uri.query != ""
    #get << " HTTP/1.1\r\n"

    puts data.inspect
    puts get.inspect
    puts "#{data} but sending #{get}"
    serv.puts get

    # the client initiates everything - so copy it to the server
    puts "COPY from amarok -> serv"
    cp_to_empty_outward amaroks, serv

    safe_write serv, "\r\n\r\n" 

    puts "COPY from serv -> amarok"
    cp_to_empty_inward serv, amaroks

    # now copy from the server to amarok
    puts "Before cp_all()"
    cp_all_inward serv, amaroks

    engine = `dcop amarok player engine`.chomp
    puts( "Engine is #{engine}" )

    if engine == 'helix-engine' && amaroks.eof
      puts "EOF Detected, reconnecting"
      amaroks = amarok.accept
      cp_all_inward( serv, amaroks )
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

  def cp_to_empty_outward( income, output )
    puts "cp_to_empty_outward( income => #{income.inspect}, output => #{output.inspect}"
    income.each_line do |data|
      puts( data )
      data.chomp!
      safe_write output, data
      return if data.empty?
    end
  end

  def cp_to_empty_inward( income, output )
    puts "cp_to_empty_inward( income => #{income.inspect}, output => #{output.inspect}"
    income.each_line do |data|
      puts( data )
      safe_write output, data
      return if data.chomp == ""
    end
  end

  def cp_all_inward( income, output )
    puts "cp_all( income => #{income.inspect}, output => #{output.inspect}"
    loop do
      data = income.read( 1000 )
      break if data == nil

      # intercept SYNCs
      if data.include?( "SYNC" ) # FIXME won't detect the SYNC if it spreads over fragment boundary
          data.gsub!( "SYNC", "" )
          puts( "SYNC" )
      end

      begin
        safe_write output, data
      rescue
        puts( "error from o.write, #{$!}" )
        break
      end
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