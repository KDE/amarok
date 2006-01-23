#!/usr/bin/env ruby
#
# amaroK Script for fetching song lyrics from http://lyrics.astraweb.com.
#
# (c) 2006 Mark Kretschmann <markey@web.de>
#
# License: GNU General Public License V2


require "net/http"
require "uri"

def showLyrics( lyrics )
    # Important, otherwise we might execute arbitrary nonsense in the DCOP call
    lyrics.gsub!( '"', "'" )
    lyrics.gsub!( '`', "'" )

    `dcop amarok contextbrowser showLyrics "#{lyrics}"`
end

def fetchLyrics( artist, title )
    h = Net::HTTP.new( "search.lyrics.astraweb.com", 80 )
    response = h.get( "/?word=#{artist}+#{title}" )

    unless response.code == "200"
        lyrics = "HTTP Error: #{response.message}"
        `dcop amarok contextbrowser showLyrics "#{lyrics}"`
        return
    end

    body = response.body()
    body = body[body.index( '<tr><td bgcolor="#BBBBBB"' )..body.index( "More Songs &gt;" ) - 1]
    body.gsub!( "\n", "" ) # No need for \n, just complicates our RegExps

    entries = body.split( '<tr><td bgcolor="#BBBBBB"' )
    entries.delete_at( 0 )

    lyrics = "Suggestions : <br>"
    entries.each do |entry|
        url = /display.[^"]*/.match( entry ).to_s()
        title = /(http:\/\/display\.lyrics.*">)([^<]*)/.match( entry )[2].to_s()
        artist = /(Artist:.*html">)([^<]*)/.match( entry )[2].to_s()

        lyrics += "<a href='#{url}'>#{artist} - #{title}</a><br>"
    end

    puts( lyrics )
#     showLyrics( lyrics )
end


##################################################################
# MAIN
##################################################################

fetchLyrics( "radio", "gaga" )
exit()


loop do
    message = gets().chomp()
    command = /[A-Za-z]*/.match( message ).to_s()

    case command
        when "configure"
            msg  = '"This script does not have configuration options."'
            `dcop amarok playlist popupMessage "#{msg}"`

        when "fetchLyrics"
            args = message.split()

            artist = args[1]
            title  = args[2]

            fetchLyrics( artist, title )

        when "fetchLyricsByUrl"
            url = message.split()[1]

#             fetchLyrics( url )
    end
end

