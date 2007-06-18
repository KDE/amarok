#!/usr/bin/env ruby
#
# script to remove stale entries in some database tables (artist)
#
# (c) 2006 Roland Gigler <rolandg@web.de>
# License: GNU General Public License V2

system("dcop", "amarok", "playlist", "shortStatusMessage", "Removing stale 'artist' entries from the database")

qresult = `dcop amarok collection query "SELECT id FROM artist;"`
result = qresult.split( "\n" )

i = 0

result.each do |id|
    print "Checking: #{id}, "
    qresult2 = `dcop amarok collection query "SELECT COUNT(*) FROM tags where artist = #{id};"`
    count = qresult2.chomp()
    printf "count: %s\n", count
    if  count == "0"
        i = i + 1
	qresult3 = `dcop amarok collection query "SELECT name FROM artist where id = #{id} ;"`
        result3 = qresult3.split( "\n" )
        puts "==>: Deleting: #{id}, #{result3}"
        system("dcop", "amarok", "collection", "query", "DELETE FROM artist WHERE id = '#{id}'")
    end
end
puts "i: #{i}"

if i > 0
    system("dcop", "amarok", "playlist", "popupMessage", "Removed #{i} stale 'artist' entries from the database")
end

