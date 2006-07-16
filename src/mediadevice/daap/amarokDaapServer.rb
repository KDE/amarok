#!/usr/bin/ruby

#A DAAP Server
# (c) 2006 Ian Monroe <ian@monroe.nu>
# License: GNU General Public License V2

require 'webrick'
 
class Fixnum
    def to_daapNet
        a = Array.new
        a[0] = self
        a.pack('N')
    end
end
 
class Element
    #attr_accessor :length, :name, :value
    public 
        def initialize(name, value = Array.new)
            @name, @value = name, value
        end
        
        def to_s
            content = valueToString
            @name + content.length.to_daapNet + content
        end
        
        def collection?
            @value.class == Array
        end
        
        def <<( child )
            @value << child
        end
    private
        def valueToString
            case @value
                when Fixnum then
                    @value.to_daapNet
                when String then
                    @value
                when Array then
                    values = String.new
                    @value.each do |i|
                        values += i.to_s
                    end
                    values
                else
                    puts "type error! #{@value} #{@value.class}"
            end
        end
end

#{"mlog"=>{"mlid"=>[1842003488], "mstt"=>[200]}}
class LoginServlet < WEBrick::HTTPServlet::AbstractServlet
    @@sessionId = 42
    
    def do_GET( req, resp )
        root =  Element.new( 'mlog' )
        root << Element.new( 'mlid', @@sessionId )
        root << Element.new( 'mstt',  WEBrick::HTTPStatus::OK.code )
        resp.body = root.to_s
        puts resp.body.dump
        @@sessionId += 1
    end
end

#{"mupd"=>{"mstt"=>[200], "musr"=>[2]}}
class UpdateServlet < WEBrick::HTTPServlet::AbstractServlet

    def do_GET( req, resp )
        root = Element.new( 'mupd' )
        root << Element.new( 'mstt', WEBrick::HTTPStatus::OK.code )
        root << Element.new( 'musr', 2 )
        resp.body = root.to_s
        puts resp.body.dump
    end

end

class Controller

    def initialize
        server = WEBrick::HTTPServer.new( { :Port=>8081 } )
        ['INT', 'TERM'].each { |signal|
            trap(signal) { server.shutdown } #play nice in irb
        }
        server.mount( '/login', LoginServlet )
        server.mount( '/update', UpdateServlet )
        server.start
    end

end


Controller.new