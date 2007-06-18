#!/usr/bin/env ruby
#
# script to remove stale images in the database
#
# (c) 2006 Roland Gigler <rolandg@web.de>
# License: GNU General Public License V2

system("dcop", "amarok", "playlist", "shortStatusMessage", "Removing stale 'images' entries from the database")

qresult = `dcop amarok collection query "SELECT path FROM images;"`
result = qresult.split( "\n" )

i = 0

result.each do |url|
    #puts "url: #{url}"
    unless FileTest.exist?( url )
        i = i + 1
        url.gsub!(/[']/, '\\\\\'')
        puts "Deleting: #{url}"
        system("dcop", "amarok", "collection", "query", "DELETE FROM images WHERE path = '#{url}'")
    end
end

if i > 0
    system("dcop", "amarok", "playlist", "popupMessage" "Removed #{i} stale 'images' entries from the database")
end
