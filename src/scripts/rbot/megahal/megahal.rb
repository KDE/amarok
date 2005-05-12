# This is a port of the MegaHal chatterbot software to Ruby.
# The original code can be downloaded here: http://megahal.alioth.debian.org/
#
# (C) 1998 Jason Hutchens
# (C) 2005 Mark Kretschmann <markey@web.de>
#
# Licens GNU General Public License V2


def initialize

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


