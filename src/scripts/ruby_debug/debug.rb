#!/usr/bin/env ruby
#
# Debug functions, emulating the ones we have in amaroK
#
# (c) 2006   Mark Kretschmann <markey@web.de>
# (c) 2003-5 Max Howell <max.howell@methylblue.com>


DEBUG_PREFIX = "TestClass"

 #
 # @short Use this to label sections of your code
 #
 # Usage:
 #
 #     def function()
 #         Debug_Block
 #
 #         debug "output1"
 #         debug "output2"
 #     end
 #
 # Will output:
 #
 #     BEGIN: function()
 #       [prefix] output1
 #       [prefix] output2
 #     END: function() - Took 0.1s
 #
def debug_block()
    d = Debug_Block.new()
    funcname = /(`)(.*)(')/.match( caller( 1 )[0] )[2].to_s()
    t1 = Time.new
    indent = " " * ( caller( 3 ).size ) * 2

    ObjectSpace.define_finalizer( d, Proc.new {
        puts( "#{indent}END__: #{funcname}() - Took #{Time.new - t1}s" )
    } )

    puts( "#{indent}BEGIN: #{funcname}() " )
    return d
end

class Debug_Block
end

def debug( str )
    indent = " " * ( caller( 2 ).size ) * 2
    puts( "#{indent}[#{DEBUG_PREFIX}] #{str}" )
end

def warning( str )
    indent = " " * ( caller( 2 ).size ) * 2
    puts( "#{indent}WARNING: [#{DEBUG_PREFIX}] #{str}" )
end

def error( str )
    indent = " " * ( caller( 2 ).size ) * 2
    puts( "#{indent}ERROR: [#{DEBUG_PREFIX}] #{str}" )
end


####################################################################
# MAIN
####################################################################

def funA()
    debug_block

    funB()
end

def funB()
    debug_block

    debug( "This is a test" )

    funC()
end

def funC()
    debug_block

    warning( "What the hell is going on here" )
end


funA()
