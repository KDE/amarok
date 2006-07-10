#!/usr/bin/env ruby
#
# Proxy server for last.fm. Relays the stream from the last.fm server to localhost.
#
# Stream consists of pure MP3 files concatenated, with the string "SYNC" in between, which
# marks a track change. We notify Amarok on trackchange.
# Amarok listens to stderr and recognizes these magic strings, do not remove them:
# "AMAROK_PROXY: startup", "AMAROK_PROXY: SYNC"
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

class Proxy
  def initialize( port, remote_url, engine )
    @engine = engine

    myputs( "running with port: #{port} and url: #{remote_url} and engine: #{engine}" )

    #
    # amarok is, well, Amarok
    #
    amarok  = TCPServer.new( port )
    myputs( "startup" )

    amaroks = amarok.accept

    #
    # serv is the lastfm stream server
    #
    uri = URI.parse( remote_url )
    myputs("host " << uri.host << " ")
    myputs( port )
    serv = TCPSocket.new( uri.host, uri.port )
    myputs( "running with port: #{uri.port} and host: #{uri.host}" )

    # here we substitute the proxy GET
    data = amaroks.readline
    get = get_request( uri )

    myputs( data.inspect )
    myputs( get.inspect )
    myputs( "#{data} but sending #{get}" )
    serv.puts( get )

    # the client initiates everything - so copy it to the server
    myputs( "COPY from amarok -> serv" )
    cp_to_empty_outward( amaroks, serv )

    safe_write( serv, "\r\n\r\n" )

    myputs( "COPY from serv -> amarok" )
    cp_to_empty_inward( serv, amaroks )

    # now copy from the server to amarok
    myputs( "Before cp_all()" )
    cp_all_inward( serv, amaroks )

    if @engine == 'helix-engine' && amaroks.eof
      myputs( "EOF Detected, reconnecting" )
      amaroks = amarok.accept
      cp_all_inward( serv, amaroks )
    end
  end

  def safe_write( output, data )
    begin
      output.write data
    rescue
      myputs( "error from output.write, #{$!}" )
      myputs( $!.backtrace.inspect )
      break
    end
  end

  def cp_to_empty_outward( income, output )
    myputs "cp_to_empty_outward( income => #{income.inspect}, output => #{output.inspect}"
    income.each_line do |data|
      myputs( data )
      data.chomp!
      safe_write( output, data )
      return if data.empty?
    end
  end

  def cp_to_empty_inward( income, output )
    myputs( "cp_to_empty_inward( income => #{income.inspect}, output => #{output.inspect}" )
    income.each_line do |data|
      myputs( data )
      safe_write( output, data )
      return if data.chomp == ""
    end
  end

  def cp_all_inward( income, output )
    myputs( "cp_all( income => #{income.inspect}, output => #{output.inspect}" )
    #     if @engine == 'xine-engine'
    #       filler = Array.new( 4096, 0 )
    #       safe_write( output, filler ) # HACK: Fill xine's buffer so that xine_open() won't block
    #     end
    loop do
      data = income.read( 512 )
      break if data == nil
      # Detect and remove SYNCs. Removal is not strictly necessary.
      if data.include?( "SYNC" ) # FIXME won't detect the SYNC if it spreads over fragment boundary
        data.gsub!( "SYNC", "" )
        myputs( "SYNC" )
      end
      begin
        safe_write( output, data )
      rescue
        myputs( "error from o.write, #{$!}" )
        break
      end
    end
  end
end

class LastFM < Proxy
  def get_request( remote_uri )
    "GET #{remote_uri.path || '/'}?#{remote_uri.query} HTTP/1.1\r\n\r\n"
  end
end

class DaapProxy < Proxy
  ENDL = "\r\n"

  def initialize( port, remote_url, engine, hash, request_id )
    @hash = hash
    @requestId = request_id
    super( port, remote_url, engine )
  end

  def get_request( remote_uri )
    get = "GET #{remote_uri.path || '/'}?#{remote_uri.query} HTTP/1.1" + ENDL
    get +=  "Accept: */*" + ENDL
    get += "User-Agent: iTunes/4.6 (Windows; N)" + ENDL
    get += "Client-DAAP-Version: 3.0" + ENDL
    get += "Client-DAAP-Validation: #{@hash}" + ENDL
    get += "Client-DAAP-Access-Index: 2" + ENDL
    get += "Client-DAAP-Request-ID: #{@requestId}" + ENDL
    get += "Host: #{remote_uri.host}:#{remote_uri.port}" + ENDL + ENDL
    get
  end
end

def myputs( string )
  $stderr.puts( "AMAROK_PROXY: #{string}" )
end

begin
  myputs( ARGV )
  if( ARGV[0] == "--lastfm" ) then
    option, port, remote_url, engine = ARGV
    LastFM.new( port, remote_url, engine )
  else
    option, port, remote_url, engine, hash, request_id = ARGV
    DaapProxy.new( port, remote_url, engine, hash, request_id )
  end
rescue
  myputs( $!.to_s )
  myputs( $!.backtrace.inspect )
end

puts( "exiting" )
