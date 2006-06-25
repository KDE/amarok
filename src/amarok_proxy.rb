#!/usr/bin/env ruby
#
# Proxy server for last.fm. Relays the stream from the last.fm server to localhost.
#
# (c) 2006 Mark Kretschmann <markey@web.de>
#
# License: GNU General Public License V2

require "net/http"
require 'socket'
require "uri"


def puts( string )
    $stdout.puts( "AMAROK_PROXY: " + string )
    $stderr.puts( "AMAROK_PROXY: " + string )
end


puts( "startup" )

port = $*[0].to_i
remote_url = $*[1]


serv = TCPServer.new( port )
sock = serv.accept

puts( "connected" )

sock.puts( "HTTP/1.0 200 Ok\r\nContent-Type: audio/x-mp3; charset=\"utf-8\"\r\n\r\n" )


uri = URI.parse( remote_url )

h = Net::HTTP.new( uri.host, uri.port )

response = h.get( "#{uri.path}?#{uri.query}" ) do |data|
    if data[0, 4] == "SYNC"
        data[0, 4] = ""
        puts( "SYNC" )
    end

    begin
        sock.write( data )
    rescue
        break
    end
end


unless response.code == "200"
    puts( "ERROR! Could not connect to last.fm. Code: #{response.code}" )
end


puts( "exiting" )
