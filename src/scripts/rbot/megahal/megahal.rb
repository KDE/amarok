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
        @tokens = Array[s1, s2, s3, s4]
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

    def ==(other)
        return other.token[0] == @token[0] and
               other.token[1] == @token[1] and
               other.token[2] == @token[2] and
               other.token[3] == @token[3]
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


######################################################################
    private
######################################################################

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

        if string[pos] == ' ' and alpha.include?(string[pos-1]) and alpha.include?(string[pos+1])
            return false
        end

        if pos > 1 and string[pos-1] == ' ' and alpha.include?(string[pos-2]) and alpha.include?(string[pos])
            return false
        end

        if alpha.include?(string[pos]) and not alpha.include?(string[pos-1])
            return true
        end

        if not alpha.include?(string[pos]) and alpha.include?(string[pos-1])
            return true
        end

        if digit.include?(string[pos]) != digit.include?(string[pos-1])
            return true
        end
    end


    # Breaks a string into an array of words.
    def make_parts(input)
        offset = 0
        parts = Array.new()

        loop do
            if boundary?(input, offset)
                # Add word to array
                parts << input.slice(0..offset-1)

                break if offset == input.length()
                input.slice!(0..offset-1)
                offset = 0
            else
                offset = offset+1
            end
        end

        return parts
    end


    def learn(parts)
        if parts.size() >= 4

        i = 0
        while i < parts.size()-3 do
            quad = Quad.new(parts[0], parts[1], parts[2], parts[3])
            if quads.containsKey(quad)
                quad = quads[quad]
            else


            i = i+1
        end

        else
            puts "Parts has less than 4 elements, skipping."
        end
    end


######################################################################
    public
######################################################################

    def do_reply(input)
        input.upcase!
        parts = make_parts(input)

#         learn(parts)
#
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
puts "Enter text: \n"
text = readline()

puts "\n"
puts "Words: \n"
puts hal.do_reply(text)

