# (c) 2005 Mark Kretschmann <markey@web.de>
# License: GPL V2


require "net/http"

h = Net::HTTP.new( "grouphug.us", 80 )
resp, data = h.get( "/random" )

reg = Regexp.new( '(<td class="conf-text")(.*?)(<p>)(.*?)(</p>)', Regexp::MULTILINE )
confession = reg.match( data )[4]
confession.gsub!( /<.*?>/, "" ) # Remove html tags
confession.gsub!( "\t", "" ) # Remove tab characters

puts confession

