#!/usr/bin/env ruby
#
# Proxy server for last.fm
#
# (c) 2006 Mark Kretschmann <markey@web.de>
#
# License: GNU General Public License V2

require "net/http"
require 'socket'
require "uri"


puts( "AMAROK_PROXY: startup" )

port = $*[0].to_i
remote_url = $*[1]
$*.clear # Ensure that gets() will use Stdin, see gets() docs

puts( "AMAROK_PROXY: using port: #{port}" )
puts( "AMAROK_PROXY: remote stream URL: #{remote_url}" )

serv = TCPServer.new( port )
sock = serv.accept

puts( "AMAROK_PROXY: connected" )

sock.puts( "HTTP/1.0 200 Ok\r\nContent-Type: audio/x-mp3; charset=\"utf-8\"\r\n\r\n" )


uri = URI.parse( remote_url )

h = Net::HTTP.new( uri.host, uri.port )

response = h.get( "#{uri.path}?#{uri.query}" ) do |data|
    if data[0, 4] == "SYNC"
        $stderr.puts( "AMAROK_PROXY: SYNC frame" )
        $stdout.puts( "AMAROK_PROXY: SYNC frame" )
        next
    end

    sock.write( data )
end


unless response.code == "200"
    puts( "AMAROK_PROXY: ERROR! Could not connect to last.fm. Code: #{response.code}" )
end



