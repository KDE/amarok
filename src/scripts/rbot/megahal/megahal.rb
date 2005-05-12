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

        if pos == 0
            return false
        end

        if pos == string.length
            return true
        end

        if string[pos] == "\" and alpha.include?(string[pos-1]) and alpha.include?(string[pos+1])
            return false
        end
    end


    # Breaks a string into an array of words.
    private
    def make_words(input, words)
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

