#!/usr/bin/env ruby
#
# Convert a file with the old wiki syntax to a file with MoxQuiz compatible syntax.
#
# You'll need internet acces to run this script.
#
# Based on import_mox.rb and quiz.rb by markey.
#
# (c) 2006 Mark Kretschmann <markey@web.de>
# (c) 2006 Jocke Andersson <ajocke@gmail.com>
# Licensed under GPL V2.

require "net/http"

if $*.empty?() or $*[0] == "--help"
    puts( "Usage: wiki_to_mox.rb output" )
    exit( 1 )
end

outfile = $*[0]

output = File.new( outfile, File::CREAT | File::RDWR | File::TRUNC )

h        = Net::HTTP.new( "amarok.kde.org", 80 )
response = h.get( "/amarokwiki/index.php/Rbot_Quiz" )
rawdata  = response.body()

output.write( "#Questions converted using wiki_to_mox.rb" )

rawdata = rawdata.split( "QUIZ DATA START\n</p><p>" )[1]
rawdata = rawdata.split( "QUIZ DATA END" )[0]
items   = rawdata.split( "</p><p><br />\n" )

items.each do |item|
    qdata = item.split( "</p><p>" )

    question = qdata[0].chomp()
    answer   = qdata[1].chomp()

    output.write( "\n\nQuestion: #{question}\nAnswer: #{answer}" )
end


