#!/usr/bin/env ruby
#
# Convert simple one-line-per-question quiz files to MoxQuiz compatible syntax.
#
# The only requirements for this script to work is that the question and answer are on the same line,
# with some divider in between.
# The question and the answer also needs to have the same position in the divided string.
# If one line has the syntax:
#  question|answer
# and another has the syntax:
#  category|question|answer
# this script won't work.
# Excessive data on the lines will be ignored.
#
# You'll need to remove any comments from the beginning of the file to convert before running this script.
#
# Based on import_mox.rb by markey.
#
# (c) 2006 Mark Kretschmann <markey@web.de>
# (c) 2006 Jocke Andersson <ajocke@gmail.com>
# Licensed under GPL V2.


if $*.empty?() or $*[0] == "--help" or $*[1] == nil
    puts( "Usage: oneline_to_mox.rb source output" )
    exit( 1 )
end

infile = $*[0]
outfile = $*[1]

puts( "What divider is used between the question and the answer (and any other data)?" )
divider = STDIN.gets().chomp()
puts( "What position in the divided string does the QUESTION have?\n(The first position, \"fu\" in \"fu#{divider}bar\", is 1)" )
qpos = Integer( STDIN.gets().chomp() )
puts( "And what position does the ANSWER have?" )
apos = Integer( STDIN.gets().chomp() )

input  = File.new( infile,  File::RDONLY )
output = File.new( outfile, File::CREAT | File::RDWR | File::TRUNC )

rawdata = input.read
output.write( "#Questions converted using oneline_to_mox.rb" )

items = rawdata.split( "\n" )

items.each do |item|
    qdata = item.split( divider )

    question = qdata[qpos - 1]
    answer = qdata[apos - 1]

    out = "\n\nQuestion: #{question}\nAnswer: #{answer}"
    output.write( out )
end


