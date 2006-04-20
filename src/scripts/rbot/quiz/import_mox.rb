#!/usr/bin/env ruby
#
# Import quiz questions/answer from MoxQuiz files.
#
# (c) 2006 Mark Kretschmann <markey@web.de>
# Licensed under GPL V2.


if $*.empty?() or $*[0] == "--help"
    puts( "Usage: import_mox.rb source" )
    exit( 1 )
end

path = $*[0]

input  = File.new( path,  File::RDONLY )
output = File.new( "RBOT.txt", File::CREAT | File::RDWR | File::TRUNC )

text = input.read
target = ""

items = text.split( /Category:.*\n/ )

items.each do |item|
    lines = item.split( "\n" )

    question = lines[0].split( "Question: " )
    answer = lines[1].split( "Answer: " )

    target += "#{question}\n\n#{answer}\n\n\n"
end


output.write( target )


