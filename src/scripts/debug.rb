#!/usr/bin/env ruby
#
# Debug functions, emulating the ones we have in amaroK


def debug_block()
    d = Debug_Block.new()
    funcname = /(`)(.*)(')/.match( caller( 1 )[0] )[2].to_s()
    t1 = Time.new
    indent = " " * ( caller( 2 ).size - 1 ) * 2

    ObjectSpace.define_finalizer( d, Proc.new {
        puts( "#{indent}END__: #{funcname}() - Took #{Time.new - t1}s" )
    } )

    puts( "#{indent}BEGIN: #{funcname}() " )
    return d
end


class Debug_Block
end


def funA()
    debug_block

    funB()
end

def funB()
    debug_block

    funC()
end

def funC()
    debug_block
end


funA()
