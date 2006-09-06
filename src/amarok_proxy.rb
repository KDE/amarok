#!/usr/bin/env ruby
#
# Proxy server for Last.fm and DAAP. Relays the stream from the server to localhost, and
# converts the protocol to http on the fly.
#
# (c) 2006 Paul Cifarelli <paul@cifarelli.net>
# (c) 2006 Mark Kretschmann <markey@web.de>
# (c) 2006 Michael Fellinger <manveru@weez-int.com>
# (c) 2006 Ian Monroe <ian@monroe.nu>
# (c) 2006 Martin Ellis <martin.ellis@kdemail.net>
# (c) 2006 Alexandre Oliveira <aleprj@gmail.net>
#
# License: GNU General Public License V2

# Amarok listens to stderr and recognizes these magic strings, do not remove them:
# "AMAROK_PROXY: startup", "AMAROK_PROXY: SYNC"


require 'socket'
require "uri"
$stdout.sync = true

class Proxy
  ENDL = "\r\n"

  def initialize( port, remote_url, engine, proxy )
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

    #Check for proxy
    proxy_uri = URI.parse( proxy )
    if ( proxy_uri.class != URI::Generic )
      serv = TCPSocket.new( proxy_uri.host, proxy_uri.port )
    else
      serv = TCPSocket.new( uri.host, uri.port )
    end

    serv.sync = true
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

    if @engine == 'gst10-engine'
      3.times do
        myputs( "gst10-engine waiting for reconnect" )
        sleep 1
        break if amaroks.eof
      end
      amaroks = amarok.accept
      safe_write( amaroks, "HTTP/1.0 200 OK\r\n\r\n" )
      amaroks.each_line do |data|
        myputs( data )
        data.chomp!
        break if data.empty?
      end
    end

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
    if self.is_a?( LastFM ) and @engine == 'xine-engine'
      myputs( "Using buffer fill workaround." )
      filler = Array.new( 4096, 0 )
      safe_write( output, filler ) # HACK: Fill xine's buffer so that xine_open() won't block
    end
    loop do
      data = income.read( 1024 )
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
# Last.fm protocol:
# Stream consists of pure MP3 files concatenated, with the string "SYNC" in between, which
# marks a track change. The proxy notifies Amarok on track change.
  def get_request( remote_uri )
    get =  "GET #{remote_uri.path || '/'}?#{remote_uri.query} HTTP/1.1" + ENDL
    get += "Host: #{remote_uri.host}:#{remote_uri.port}" + ENDL + ENDL
    myputs( "\n\n\n\n" + get )
    get
  end
end

class DaapProxy < Proxy
  def initialize( port, remote_url, engine, hash, request_id, proxy )
    @hash = hash
    @requestId = request_id
    super( port, remote_url, engine, proxy )
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
  $stdout.puts( "AMAROK_PROXY: #{string}" )
end

begin
  myputs( ARGV )
  if( ARGV[0] == "--lastfm" ) then
    option, port, remote_url, engine, proxy = ARGV
    LastFM.new( port, remote_url, engine, proxy )
  else
    option, port, remote_url, engine, hash, request_id, proxy = ARGV
    DaapProxy.new( port, remote_url, engine, hash, request_id, proxy )
  end
rescue
  myputs( $!.to_s )
  myputs( $!.backtrace.inspect )
end

puts( "exiting" )
