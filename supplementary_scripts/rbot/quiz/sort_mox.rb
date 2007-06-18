#!/usr/bin/env ruby
#
# Sort a MoxQuiz file by Answer
# Partly based on quiz.rb by markey and me
#
# (c) 2006 Jocke Andersson <ajocke@gmail.com>
# Licensed under GPL V2.


if $*.empty?() or $*[0] == "--help" or $*[1] == nil
    puts( "Usage: sort_mox.rb source output" )
    exit( 1 )
end

infile = $*[0]
outfile = $*[1]

input  = File.new( infile,  File::RDONLY )
output = File.new( outfile, File::CREAT | File::RDWR | File::TRUNC )

rawdata = input.read
entries = rawdata.split( "\nQuestion: " )
#First entry will be comments only
output.write( entries[0].chomp + "\n#Questions sorted with sort_mox.rb" )
entries.delete_at(0)

QuizBundle = Struct.new( "QuizBundle", :question, :answer, :compare )

@questions = Array.new

entries.each do |e|
    p = e.split( "\n" )
    # We'll need at least two lines of data
    unless p.size < 2
        # Check if question isn't empty
        if p[0].length > 0
            p[0] = p[0].strip
            while p[1].match( /^Answer: (.*)$/ ) == nil and p.size > 2
                # Delete all lines between the question and the answer
                p.delete_at(1)
            end
            p[1] = p[1].gsub( /Answer: /, "" ).strip
            # If the answer was found
            if p[1].length > 0
                # Add the data to the array
                q = QuizBundle.new( p[0], p[1], p[1].gsub( "#", "" ).downcase )
                @questions << q
            end
        end
    end
end

@questions.sort { |a,b| a.compare<=>b.compare }.each do |q|
    output.write( "\n\nQuestion: #{q.question}\nAnswer: #{q.answer}" )
end