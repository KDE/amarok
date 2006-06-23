#!/usr/bin/env ruby
#
# Proxy server for last.fm

require 'socket'


puts( "AMAROK_PROXY: startup" )

port = $*[0].to_i

puts( "AMAROK_PROXY: using port: #{port}" )

serv = TCPServer.new( port )
sock = serv.accept

puts( "AMAROK_PROXY: connected" )

sock.puts( "HTTP/1.0 200 Ok\r\nContent-Type: audio/x-mp3; charset=\"utf-8\"\r\n\r\n" )

stream_url = gets().chomp()

puts( "AMAROK_PROXY: remote stream URL: #{stream_url}" )



loop { sleep( 0.1 ) }



