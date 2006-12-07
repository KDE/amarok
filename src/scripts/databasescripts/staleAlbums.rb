#!/usr/bin/env ruby
#
# script to remove stale entries in some database tables (album)
#
# (c) 2006 Roland Gigler <rolandg@web.de>
# License: GNU General Public License V2

class String
    def shellquote
        return "'" + self.gsub("'", "'\\\\''") + "'"
    end
end

system("dcop", "amarok", "playlist", "shortStatusMessage", "Removing stale 'album' entries from the database")

qresult = `dcop amarok collection query #{"SELECT id FROM album;".shellquote}`
result = qresult.split( "\n" )

i = 0

result.each do |id|
    print "Checking: #{id}, "
    qresult2 = `dcop amarok collection query #{"SELECT COUNT(*) FROM tags where album = #{id};".shellquote}`
    count = qresult2.chomp()
    printf "count: %s", count
    if  count == "0"
        i = i + 1
	qresult3 = `dcop amarok collection query #{"SELECT name FROM album where id = #{id} ;".shellquote}`
        result3 = qresult3.split( "\n" )
        puts "==>: Deleting: #{id}, #{result3}"
        system("dcop", "amarok", "collection", "query", "DELETE FROM album WHERE id = '#{id}'")
    end
    print "\n"
end
puts "removed #{i} albums."

if i > 0
    system("dcop", "amarok", "playlist", "popupMessage", "Removed #{i.shellquote} stale 'album' entries from the database")
end

