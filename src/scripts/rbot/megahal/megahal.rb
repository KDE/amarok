# This is a port of the MegaHal chatterbot software to Ruby.
# The original code can be downloaded here: http://megahal.alioth.debian.org/
#
# (C) 1998 Jason Hutchens
# (C) 2005 Mark Kretschmann <markey@web.de>
#
# Licens GNU General Public License V2


class MegaHal

    def initialize
    end


    # Returns whether or not a word boundary exists in a string at the specified location.
    private
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

        if digit.include?(string[pos] != digit.incldue[pos-1]
            return true
        end
    end


    # Breaks a string into an array of words.
    private
    def make_words(input, words)
        offset = 0

        loop do
            if boundary?(input, offset)

                offset = 0
            else
                offset = offset+1
            end
        end
    end


    def do_reply(input)
        input.upcase!
        make_words(input, @words)

        learn(@model, @words)

        output = generate_reply(@model, @words)
        capitalize(output)
        return output
    end

end

