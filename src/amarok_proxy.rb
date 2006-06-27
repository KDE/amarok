#!/usr/bin/env ruby
#
# Proxy server for last.fm. Relays the stream from the last.fm server to localhost.
#
# (c) 2006 Paul Cifarelli <paul@cifarelli.net>
# (c) 2006 Mark Kretschmann <markey@web.de>
#
# License: GNU General Public License V2

require "net/http"
require 'socket'
require "uri"

include Socket::Constants


def puts( string )
    $stdout.puts( "AMAROK_PROXY: " + string )
    $stderr.puts( "AMAROK_PROXY: " + string )
end


def cptoempty( s, o )
    s.each_line do |data|
        # intercept SYNCs
        if data[0, 4] == "SYNC"
            data[0, 4] = ""
            puts( "SYNC" )
        end
        puts( data )
        o.write( data )
        break if data.chomp == ""
    end
end


def cpall( s, o )
    loop do
        data = s.read( 1000 )
        break if data == nil
        # intercept SYNCs
        if data.include?( "SYNC" ) # FIXME won't detect the SYNC if it spreads over fragment boundary
            data.gsub!( "SYNC", "" )
            puts( "SYNC" )
        end
        o.write( data )
    end
end


puts( "startup" )

port = $*[0].to_i
remote_url = $*[1]

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

# here we substitute the proxy GET
data = amaroks.readline
puts( data << " but sending GET " << uri.path << "?" << uri.query << " HTTP/1.1\r\n" )
serv.puts( "GET " << uri.path << "?" << uri.query << " HTTP/1.1\r\n\r\n" )

# the client initiates everything - so copy it to the server
cptoempty( amaroks, serv )

cptoempty( serv, amaroks )

puts( "Before cpall()" )

# now copy from the server to amarok
cpall( serv, amaroks )

puts( "exiting" )
