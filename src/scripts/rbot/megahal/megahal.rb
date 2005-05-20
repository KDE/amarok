# This is a port of the MegaHal chatterbot software to Ruby.
# The original code can be downloaded here: http://megahal.alioth.debian.org/
# This port is also based on JMegaHal, http://www.jibble.org/jmegahal/
#
# (C) 1998 Jason Hutchens
# (C) 2001-2004 Paul James Mutton
# (C) 2005 Mark Kretschmann <markey@web.de>
#
# License: GNU General Public License V2


######################################################################
# class Quad
######################################################################

class Quad

    def initialize(s1, s2, s3, s4)
        @tokens = [s1, s2, s3, s4]
        @canStart = false
        @canEnd = false
    end

    def getToken(index)
        return @tokens[index]
    end

    def setCanStart(flag)
        @canStart = flag
    end

    def setCanEnd(flag)
        @canEnd = flag
    end

    def canStart()
        return @canStart
    end

    def canEnd()
        return @canEnd
    end

    def hashCode()
        return tokens[0].hash() +
               tokens[1].hash() +
               tokens[2].hash() +
               tokens[3].hash()
    end

    def equals(other)
        equal = other.token[0] == @token[0] and
                other.token[1] == @token[1] and
                other.token[2] == @token[2] and
                other.token[3] == @token[3]
        return equal
    end

end


######################################################################
# class MegaHal
######################################################################

class MegaHal

    def initialize
        @words = Hash.new()
        @quads = Hash.new()
        @next = Hash.new()
        @previous = Hash.new()
    end


    # Returns whether or not a word boundary exists in a string at the specified location.
    def boundary?(string, pos)
        alpha = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
        digit = '0123456789'

        if pos == 0
            return false
        end

        if pos == string.length
            return true
        end

        if string[pos, 1] == ' ' and alpha.include?(string[pos-1, 1]) and alpha.include?(string[pos+1, 1])
            return false
        end

        if pos > 1 and string[pos-1, 1] == ' ' and alpha.include?(string[pos-2, 1]) and alpha.include?(string[pos, 1])
            return false
        end

        if alpha.include?(string[pos, 1]) and not alpha.include?(string[pos-1, 1])
            return true
        end

        if not alpha.include?(string[pos, 1]) and alpha.include?(string[pos-1, 1])
            return true
        end

        if digit.include?(string[pos, 1]) != digit.include?(string[pos-1, 1])
            return true
        end
    end


    # Breaks a string into an array of words.
    def make_parts(input)
        input.upcase!()
        offset = 0
        parts = Array.new()

        parts = input.split(' ')
#         loop do
#             if boundary?(input, offset)
#                 # Add word to array
#                 parts << input.slice(0..offset-1)
#
#                 break if offset == input.length()
#                 input.slice!(0..offset-1)
#                 offset = 0
#             else
#                 offset = offset+1
#             end
#         end

        return parts
    end


    def learn(parts)
        if parts.size() >= 4
            i = 0
            while i < parts.size()-3
                quad = Quad.new(parts[i+0], parts[i+1], parts[i+2], parts[i+3])
                if @quads.has_key?(quad)
                    quad = @quads[quad]
                else
                    @quads[quad] = quad
                end
                if i == 0
                    quad.setCanStart(true)
                end
                if i == parts.size()-4
                    quad.setCanEnd(true)
                end

                n = 0
                while n < 4
                    token = parts[i+n]
                    if not @words.has_key?(token)
                        @words[token] = Array.new()
                    end
                    @words[token].push(quad)
                    n = n+1
                end
                if i > 0
                    previousToken = parts[i-1]
                    if not @previous.has_key?(quad)
                        @previous[quad] = Array.new()
                    end
                    @previous[quad].push(previousToken)
                end
                if i < parts.size()-4
                    nextToken = parts[i+4]
                    if not @next.has_key?(quad)
                        @next[quad] = Array.new()
                    end
                    @next[quad].push(nextToken)
                end
                i = i+1
            end
        else
            puts "Parts has less than 4 elements, skipping."
        end
    end


    def learnFile(path)
        file = File.new(path,  File::RDONLY)
        sentences = file.readlines()
        sentences.each do |line|
            unless line[0, 1] == "#"
                parts = make_parts(line)
#                 print parts.size()
#                 print parts
                learn(parts)
            end
        end
    end


    def generateReply(word)
        word.upcase!()
        parts = Array.new()
        quads = Array.new()

        print "Keys in @words: \n"
        @words.each_key { |key| print key + "\n" }

        if @words.has_key?(word)
            quads = @words[word]
            print "@words has the key.\n"
        end

        return "Error: quads is empty." if quads.empty?()

        middleQuad = quads[rand( quads.size()-1 )]
        quad = middleQuad

        0.upto(3) do |i|
            parts << quad.getToken(i)
        end

        count = 0
        while not quad.canEnd()
            print "Count: #{count}\n"
            count = count + 1
            nextTokens = @next[quad]
            nextToken = nextTokens[rand( nextTokens.size()-1 )]
            quad = @quads[Quad.new(quad.getToken(1), quad.getToken(2), quad.getToken(3), nextToken)]
            parts.push(nextToken)
        end

        quad = middleQuad
        count = 0
        while not quad.canStart()
            print "Count: #{count}\n"
            count = count + 1
            previousTokens = @previous[quad]
            previousToken = previousTokens[rand( previousTokens.size()-1 )]
            quad = @quads[Quad.new(previousToken, quad.getToken(0), quad.getToken(1), quad.getToken(2))]
            parts.unshift(previousToken)
        end

        sentence = String.new()
        parts.each { |x| sentence << " " << x }
        sentence << "."

        return sentence
    end


    def do_reply(input)
        parts = make_parts(input)

        learn(parts)
#         output = generate_reply(@model, @words)
#         capitalize(output)

        # testing
        output = parts.join()

        return output
    end

end


######################################################################
# Main
######################################################################

hal = MegaHal.new()
hal.learnFile("megahal.trn")
puts "Enter word: "
text = readline().chomp()
puts "\n"
puts "Answer: \n"
puts hal.generateReply(text)


# hal = MegaHal.new()
# puts "Enter text: \n"
# text = readline()
# puts "\n"
# puts "Words: \n"
# puts hal.do_reply(text)

