#!/usr/bin/env ruby
#
# Ruby script to remove stale statistics in the database
# (c) 2005 Seb Ruiz <me@sebruiz.net>
# License: GNU General Public License V2

system("dcop", "amarok", "playlist", "shortStatusMessage", "Removing stale entries from the database")

qresult = `dcop amarok collection query "SELECT url FROM statistics;"`
result = qresult.split( "\n" )

i = 0

result.each do |url|
    unless FileTest.exist?( url )
        i = i + 1
        url.gsub!(/[']/, '\\\\\'')
        puts "Deleting: #{url}"
        system("dcop", "amarok", "collection", "query", "DELETE FROM statistics WHERE url = '#{url}'")
    end
end

if i > 0
    system("dcop", "amarok", "playlist", "popupMessage", "Removed #{i} stale entries from the database")
end
