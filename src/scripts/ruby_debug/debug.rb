#!/usr/bin/env ruby
#
# Debug functions, emulating the ones we have in Amarok
#
# (c) 2006   Ian Monroe <ian@monroe.nu>
# (c) 2006   Mark Kretschmann <markey@web.de>
# (c) 2003-5 Max Howell <max.howell@methylblue.com>


$app_name = ""
$debug_prefix = "" #this is global, so doesn't seem like it would work well

 #
 # @short Use this to label sections of your code
 #
 # Usage:
 #
 #     debugMethod( :mymethod )
 #         debug "output1"
 #         debug "output2"
 #     end
 #
 # Will output:
 #
 #     BEGIN: mymethod()
 #       [prefix] output1
 #       [prefix] output2
 #     END:   mymethod() - Took 0.1s
 #

module DebugMethods
    #module adapted from code by Martin Traverso and Brian McCallister
    #from http://split-s.blogspot.com/2006/02/design-by-contract-for-ruby.html
    @@pending = Hash.new { |hash, key| hash[key] = {} }

    private

    def self.extract(this, method_name)
        old_method = this.instance_method(method_name) if !method_name.nil? && this.method_defined?(method_name)

        return old_method, method_name
    end

    def self.schedule(type, mod, method_name)
        @@pending[mod][method_name] = {:type => type}
    end

    def self.included(mod)
        old_method_added = mod.method :method_added

        new_method_added = lambda { |id|
            if @@pending.has_key? mod
                # save the list of methods and clear the entry
                # otherwise, we'll have infinite recursion on the call to mod.send(...)
                hooks = []
                if @@pending[mod].has_key? id
                    hooks << @@pending[mod][id]
                    @@pending[mod].delete id
                end
                if @@pending[mod].has_key? nil
                    # hooks with no method name are to be added to method
                    # definition that follows them
                    hooks << @@pending[mod][nil]
                    @@pending[mod].delete nil
                end
                # define scheduled hooks
                hooks.each { |entry|
                    mod.send entry[:type], id
                }
            end
            old_method_added.call id
        }

        class << mod; self; end.send :define_method, :method_added, new_method_added

        class << mod

            def debugMethod(method_name)
                old_method, method_name = DebugMethods.extract self, method_name
                if !old_method.nil?
                    self.send(:define_method, method_name) { |*args|
                        indent = " " * ( caller( 2 ).size ) * 2
                        puts "#{$app_name}: #{indent}BEGIN: #{method_name.to_s}() "
                        t1 = Time.new
                        returnValue = old_method.bind(self).call(*args)
                        puts "#{$app_name}: #{indent}END:   #{method_name.to_s} - Took #{Time.new - t1}s"
                        return returnValue
                    }
                else
                    DebugMethods.schedule :debugMethod, self, method_name
                end
            end

        end
    end
end

def debug( str )
    indent = " " * ( caller( 2 ).size ) * 2
    prefix = $debug_prefix.empty?() ? "" : "[#{$debug_prefix}]"
    puts( "#{$app_name}: #{indent}#{prefix} #{str}" )
end

def warning( str )
    indent = " " * ( caller( 2 ).size ) * 2
    prefix = $debug_prefix.empty?() ? "" : "[#{$debug_prefix}]"
    puts( "#{$app_name}: #{indent}WARNING: #{prefix} #{str}" )
end

def error( str )
    indent = " " * ( caller( 2 ).size ) * 2
    prefix = $debug_prefix.empty?() ? "" : "[#{$debug_prefix}]"
    puts( "#{$app_name}: #{indent}ERROR: #{prefix} #{str}" )
end

def fatal( str )
    indent = " " * ( caller( 2 ).size ) * 2
    prefix = $debug_prefix.empty?() ? "" : "[#{$debug_prefix}]"
    puts( "#{$app_name}: #{indent}FATAL: #{prefix} #{str}" )

    exit( 1 )
end


####################################################################
# MAIN
####################################################################


if $0 == __FILE__

$app_name = "deBugger"
$debug_prefix = "header"

class Foo
    include DebugMethods

    debugMethod(:funA)
    def funA()
        funB()
    end

    debugMethod(:funB)
    def funB()
        debug( "This is a test" )
        funC()
    end

    debugMethod(:funC)
    def funC()
        warning( "What the hell is going on here" )
    end
end


Foo.new.funA()
end