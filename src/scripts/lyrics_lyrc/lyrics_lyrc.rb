#!/usr/bin/env ruby
#
# amaroK Script for fetching song lyrics from lyrc.com.ar.
# Ported from amaroK's contextbrowser.cpp.
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# (c) 2004 Christian Muehlhaeuser <chris@chris.de>
# (c) 2005 Reigo Reinmets <xatax@hot.ee>
# (c) 2005 Alexandre Pereira de Oliveira <aleprj@gmail.com>
#
# License: GNU General Public License V2


require "net/http"
require "uri"

def cleanup()
    `dcop amarok script enableExternalLyrics false`
end

def fetchLyrics( artist, title, url )
    h = Net::HTTP.new( "lyrc.com.ar", 80 )

    if url.empty?()
        response = h.get( "/en/tema1en.php?artist=#{artist}&songname=#{title}" )
    else
        puts( "Fetching by URL: #{url}" )
        response = h.get( "/en/#{URI.unescape( url )}" )
    end

    unless response.code == "200"
        return "HTTP Error: #{response.message}"
#         exit( 1 )
    end

    lyrics = response.body()

    # Remove images, links, scripts, and styles
    lyrics.gsub!( /<[iI][mM][gG][^>]*>/, "" )
    lyrics.gsub!( /<[aA][^>]*>[^<]*<\/[aA]>/, "" )
    lyrics.gsub!( /<[sS][cC][rR][iI][pP][tT][^>]*>[^<]*(<!--[^>]*>)*[^<]*<\/[sS][cC][rR][iI][pP][tT]>/, "" )
    lyrics.gsub!( /<[sS][tT][yY][lL][eE][^>]*>[^<]*(<!--[^>]*>)*[^<]*<\/[sS][tT][yY][lL][eE]>/, "" )


    lyricsPos = lyrics.index( /<[fF][oO][nN][tT][ ]*[sS][iI][zZ][eE][ ]*='2'[ ]*>/ )

    if not lyricsPos == nil
        lyrics = lyrics[lyricsPos..lyrics.length()]
        if lyrics.include?( "<p><hr" )
            lyrics = lyrics[0, lyrics.index( "<p><hr" )]
        else
            lyrics = lyrics[0, lyrics.index( "<br><br>" )]
        end

    elsif lyrics.include?( "Suggestions : " )
        lyrics = lyrics[lyrics.index( "Suggestions : " )..lyrics.index( "<br><br>" )]

        lyrics.gsub!( "<font color='white'>", "" )
        lyrics.gsub!( "</font>", "" )
        lyrics.gsub!( "<br /><br />", "" )

    else
        lyrics = ""
    end


    # Important, otherwise we might execute arbitrary nonsense in the DCOP call
    lyrics.gsub!( '"', "'" )
    lyrics.gsub!( '`', "'" )

    `dcop amarok script showLyrics "#{lyrics}"`
end


##################################################################
# MAIN
##################################################################

trap( "SIGTERM" ) { cleanup() }

`dcop amarok script enableExternalLyrics true`


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

            fetchLyrics( artist, title, "" )

        when "fetchLyricsByUrl"
            url = message.split()[1]

            fetchLyrics( "", "", url )
    end
end

