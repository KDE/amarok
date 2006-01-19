#!/usr/bin/env ruby
#
# amaroK Script for fetching song lyrics from lyrc.com.ar.
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# License: GNU General Public License V2


require "net/http"
require "uri"

def cleanup()
    `dcop amarok script enableExternalLyrics false`
end

def fetchLyrics( artist, title )
    # artist = "John Lennon"
    # title  = "Imagine"

    h = Net::HTTP.new( "lyrc.com.ar", 80 )
    response = h.get( "/en/tema1en.php?artist=#{artist}&songname=#{title}" )

    unless response.code == "200"
        puts( "HTTP Error: #{response.message}" )
        exit( 1 )
    end

    lyrics = response.body()

    # Remove images, links, scripts, and styles
    lyrics.gsub!( /<[iI][mM][gG][^>]*>/, "" );
    lyrics.gsub!( /<[aA][^>]*>[^<]*<\/[aA]>/, "" )
    lyrics.gsub!( /<[sS][cC][rR][iI][pP][tT][^>]*>[^<]*(<!--[^>]*>)*[^<]*<\/[sS][cC][rR][iI][pP][tT]>/, "" )
    lyrics.gsub!( /<[sS][tT][yY][lL][eE][^>]*>[^<]*(<!--[^>]*>)*[^<]*<\/[sS][tT][yY][lL][eE]>/, "" )

    lyricsPos = lyrics.index( /<[fF][oO][nN][tT][ ]*[sS][iI][zZ][eE][ ]*='2'[ ]*>/ )

    if lyricsPos == nil
        puts( "Could not locate lyrics in the body." )
        exit( 1 )
    end

    lyrics = lyrics[lyricsPos..lyrics.length()]

    if lyrics.include?( "<p><hr" )
        lyrics = lyrics[0, lyrics.index( "<p><hr" )]
    else
        lyrics = lyrics[0, lyrics.index( "<br><br>" )]
    end


    return lyrics
end


##################################################################
# MAIN
##################################################################

trap( "SIGTERM" ) { cleanup() }

`dcop amarok script enableExternalLyrics true`


loop do
    message = gets().chomp()
    args = message.split()[1, args.length()]
    command = /[A-Za-z]*/.match( message ).to_s()

    case command
        when "configure"
            msg  = '"This script does not have configuration options."'
            `dcop amarok playlist popupMessage "#{msg}"`

        when "fetchLyrics"
            artist = args[0]
            title  = args[1]

            result = fetchLyrics( artist, title )
    end
end

