#!/usr/bin/env ruby
#
# amaroK Script for fetching song lyrics from http://lyrc.com.ar.
# Ported from amaroK's contextbrowser.cpp.
#
# (c) 2006 Mark Kretschmann <markey@web.de>
# (c) 2004 Christian Muehlhaeuser <chris@chris.de>
# (c) 2005 Reigo Reinmets <xatax@hot.ee>
# (c) 2005 Alexandre Pereira de Oliveira <aleprj@gmail.com>
#
# License: GNU General Public License V2

require "net/http"
require "rexml/document"
# require File.dirname( File.expand_path( __FILE__ ) ) + "/../ruby_debug/debug.rb"
require "uri"

@app_name = "Lyrics_Lyrc"


def showLyrics( lyrics )
    # Important, otherwise we might execute arbitrary nonsense in the DCOP call
    lyrics.gsub!( '"', "'" )
    lyrics.gsub!( '`', "'" )

    `dcop amarok contextbrowser showLyrics "#{lyrics}"`
end


def parseLyrics( lyrics )
    if lyrics.include?( "<p><hr" )
        lyrics = lyrics[0, lyrics.index( "<p><hr" )]
    else
        lyrics = lyrics[0, lyrics.index( "<br><br>" )]
    end

    lyrics.gsub!( /<[fF][oO][nN][tT][^>]*>/, "" )

#    doc = REXML::Document.new()
    doc = REXML::Document.new( "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" )
    root = doc.add_element( "lyrics" )

    root.add_attribute( "page_url", @page_url )
    root.add_attribute( "title", /(<b>)([^<]*)/.match( lyrics )[2].to_s().unpack("C*").pack("U*") )
    root.add_attribute( "artist", /(<u>)([^<]*)/.match( lyrics )[2].to_s().unpack("C*").pack("U*") )

    lyrics = /(<\/u><\/font>)(.*)/.match( lyrics )[2].to_s()
    lyrics.gsub!( /<[Bb][Rr][^>]*>/, "\n" ) # HTML -> Plaintext

    root.text = lyrics.unpack("C*").pack("U*") #Convert to UTF-8

    xml = ""
    doc.write( xml )

#     debug xml
    showLyrics( xml )
end


def notFound()
    doc = REXML::Document.new( "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" )
    root = doc.add_element( "suggestions" )
    root.add_attribute( "page_url", @page_url )
    xml = ""
    doc.write( xml )
    showLyrics( xml )
end

def parseSuggestions( lyrics )
    lyrics = lyrics[lyrics.index( "Suggestions : " )..lyrics.index( "<br><br>" )]

    lyrics.gsub!( "<font color='white'>", "" )
    lyrics.gsub!( "</font>", "" )
    lyrics.gsub!( "<br /><br />", "" )

    doc = REXML::Document.new( "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" )
    root = doc.add_element( "suggestions" )
    root.add_attribute( "page_url", @page_url )

    entries = lyrics.split( "<br>" )
    entries.delete_at( 0 )

    entries.each() do |entry|
        url = /(<a href=")([^"]*)/.match( entry )[2].to_s()
        artist_title = /(<a href=.*?>)([^<]*)/.match( entry )[2].to_s()
        artist = artist_title.split( " - " )[0]
        title  = artist_title.split( " - " )[1]

        suggestion = root.add_element( "suggestion" )
        suggestion.add_attribute( "url", url )
        suggestion.add_attribute( "artist", artist.unpack("C*").pack("U*") )
        suggestion.add_attribute( "title", title.unpack("C*").pack("U*") )
    end

    xml = ""
    doc.write( xml )

#     debug xml
    showLyrics( xml )
end


def fetchLyrics( artist, title, url )
    proxy_host = nil
    proxy_port = nil
    if ( ENV['http_proxy'] && proxy_uri = URI.parse( ENV['http_proxy'] ) )
        proxy_host = proxy_uri.host
        proxy_port = proxy_uri.port
    end

    host = "lyrc.com.ar"
    path = url.empty? ? "/en/tema1en.php?artist=#{artist}&songname=#{title}" : "/en/#{url}"
    @page_url = "http://" + host + path

    h = Net::HTTP.new( host, 80, proxy_host, proxy_port )
    response = h.get( path )

    unless response.code == "200"
#         error "HTTP Error: #{response.message}"
        `dcop amarok contextbrowser showLyrics ""`
        return
    end

    lyrics = response.body()

#     puts( lyrics )

    lyrics.gsub!( "\n", "" ) # No need for LF, just complicates our RegExps
    lyrics.gsub!( "\r", "" ) # No need for CR, just complicates our RegExps
#     lyrics.gsub!( '´', "'" ) # Lyrc has weird encodings

    # Remove images, links, scripts, styles and fonts
    lyrics.gsub!( /<[iI][mM][gG][^>]*>/, "" )
    lyrics.gsub!( /<[aA][^>]*>[^<]*<\/[aA]>/, "" )
    lyrics.gsub!( /<[sS][cC][rR][iI][pP][tT][^>]*>[^<]*(<!--[^>]*>)*[^<]*<\/[sS][cC][rR][iI][pP][tT]>/, "" )
    lyrics.gsub!( /<[sS][tT][yY][lL][eE][^>]*>[^<]*(<!--[^>]*>)*[^<]*<\/[sS][tT][yY][lL][eE]>/, "" )

    lyricsPos = lyrics.index( /<[fF][oO][nN][tT][ ]*[sS][iI][zZ][eE][ ]*='2'[ ]*>/ )

    if not lyricsPos == nil
        parseLyrics( lyrics[lyricsPos..lyrics.length()] )
        return

    elsif lyrics.include?( "Suggestions : " )
        parseSuggestions( lyrics )

    else
        notFound()
    end
end


##################################################################
# MAIN
##################################################################

# fetchLyrics( "Cardigans", "Lovefool", "" )
# fetchLyrics( "queen", "mama", "" )
# fetchLyrics( "station_rose_", "_dave_(original_1992)", "" )
# exit()


loop do
    message = gets().chomp()
    command = /[A-Za-z]*/.match( message ).to_s()

    case command
        when "configure"
            msg  = 'This script does not require any configuration.'
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

