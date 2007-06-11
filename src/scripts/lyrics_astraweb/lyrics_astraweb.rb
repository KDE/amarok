#!/usr/bin/env ruby
#
# Amarok Script for fetching song lyrics from http://lyrics.astraweb.com.
#
# (c) 2006 Mark Kretschmann <markey@web.de>
#
# License: GNU General Public License V2


require "net/http"
require "net/telnet"
require "rexml/document"
require "uri"

def showLyrics( lyrics )
    system("dcop", "amarok", "contextbrowser", "showLyrics", lyrics)
end


def fetchLyrics( artist, title )
    # Astraweb search term is just a number of words separated by "+"
    artist.gsub!( " ", "+" )
    title.gsub!( " ", "+" )

    host = "search.lyrics.astraweb.com"
    path = "/?word=#{artist}+#{title}"
    page_url = "http://" + host + path

    h = Net::HTTP.new( host, 80 )
    response = h.get( path )

    unless response.code == "200"
#         lyrics = "HTTP Error: #{response.message}"
        system("dcop", "amarok", "contextbrowser", "showLyrics")
        return
    end

    body = response.body()

    body.gsub!( "\n", "" ) # No need for \n, just complicates our RegExps

    md = /(<tr><td bgcolor="#BBBBBB".*)(More Songs &gt)/.match( body )

    doc = REXML::Document.new( "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" )
    root = doc.add_element( "suggestions" )
    root.add_attribute( "page_url", page_url )
    xml = ""

    if  not md == nil

        body = md[1].to_s()

        entries = body.split( '<tr><td bgcolor="#BBBBBB"' )
        entries.delete_at( 0 )

        entries.each do |entry|
            url = /(display\.lyrics\.astraweb.com:2000)([^"]*)/.match( entry )[2].to_s()
            artist = /(Artist:.*html">)([^<]*)/.match( entry )[2].to_s()
            title = /(display\.lyrics.*?>)([^<]*)/.match( entry )[2].to_s()
    #         album = /(Album:.*?">)([^<]*)/.match( entry )[2].to_s()

            suggestion = root.add_element( "suggestion" )
            suggestion.add_attribute( "url", url )
            suggestion.add_attribute( "artist", artist.unpack("C*").pack("U*") ) if artist
            suggestion.add_attribute( "title", title.unpack("C*").pack("U*") ) if title
        end
    end
    doc.write( xml )
#     puts( xml )
    showLyrics( xml )

    rescue SocketError
        showLyrics( "" )
end


def fetchLyricsByUrl( url )
    # Note: Using telnet here cause the fucking site has a broken cgi script, delivering
    #       a broken header, which makes Net::HTTP::get() crap out

    host = "display.lyrics.astraweb.com"
    port = 2000
    page_url = "http://#{host}:#{port}#{url}"

    h = Net::Telnet.new( "Host" => host, "Port" => port )

    body = h.cmd( "GET #{url}\n" )
    body.gsub!( "\n", "" ) # No need for \n, just complicates our RegExps

    artist_title = /(<title>Lyrics: )([^<]*)/.match( body )[2].to_s()
    artist = artist_title.split( " - " )[0]
    title  = artist_title.split( " - " )[1]

    lyrics = /(<font face=arial size=2>)(.*)(<br><br><br><center>)/.match( body )[2].to_s()
    lyricstwo = /(SPONSORS<\/font><br><\/center>)(.*?)(<\/font>)/.match( body )[2].to_s()
    lyrics.concat(lyricstwo)
    lyrics.gsub!( /<[Bb][Rr][^>]*>/, "\n" ) # HTML -> Plaintext
    doc = REXML::Document.new( "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" )
    root = doc.add_element( "lyrics" )
    root.add_attribute( "page_url", page_url )
    root.add_attribute( "artist", artist.unpack("C*").pack("U*") ) if artist
    root.add_attribute( "title", title.unpack("C*").pack("U*") ) if title
    root.text = lyrics.unpack("C*").pack("U*") if lyrics

    xml = ""
    doc.write( xml )

#     puts( xml )
    showLyrics( xml )

    rescue SocketError
        showLyrics( "" )
end


##################################################################
# MAIN
##################################################################

# fetchLyrics( "The Cardigans", "Lovefool" )
# fetchLyricsByUrl( '/display.cgi?whiskeytown..faithless_street..faithless_street' )
# exit()


loop do
    message = gets().chomp()
    command = /[A-Za-z]*/.match( message ).to_s()

    case command
        when "configure"
            system("dcop", "amarok", "playlist", "popupMessage", 'This script does not require any configuration.')

        when "fetchLyrics"
            args = message.split()

            artist = args[1]
            title  = args[2]

            fetchLyrics( URI.unescape( artist ), URI.unescape( title ) )

        when "fetchLyricsByUrl"
            url = message.split()[1]

            fetchLyricsByUrl( url )
    end
end

