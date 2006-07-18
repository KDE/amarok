#!/usr/bin/ruby

#A DAAP Server
# (c) 2006 Ian Monroe <ian@monroe.nu>
# License: GNU General Public License V2

require 'webrick'
require 'Korundum' 

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

class DatabaseServlet < WEBrick::HTTPServlet::AbstractServlet
 #TODO:
ARGSMETA = { "dmap.itemid"=> "dmap.itemname", "daap.songformat", "daap.songartist", "daap.songalbum" "daap.songtime", "daap.songtracknumber", "daap.songcomment" }

    def intialize
        artists = Array.new
        albums = Array.new
        genre = Array.new
        device_paths = Array.new
        
        collection = KDE::DCOPRef.new( 'amarok', 'collection' )
        [  { :dbresult=>collection.query( 'select * from albums' ),  :indexed => albums }
         , { :dbresult=>collection.query( 'select * from artists' ), :indexed => artists }
         , { :dbresult=>collection.query( 'select * from genre' )  , :indexed => genre } 
         , { :dbresult=>collection.query( 'select id, lastmountpoint from devices' ), :indexed => device_paths } ].each { |h|
             0.step( h[ :dbresult ].size, 2 ) { |i|
                h[ :indexed ][ h[ :dbresult ][i] ] = h[ :dbresult ][ i+1 ]
            }
        }
        columns =     [ "album, ", "artist, ", "genre, ", "url, ", "track, ", "title, ", "year, ", "length, ", "samplerate, ", "composer, ", "deviceid" ]
        column_keys = [ :album,    :artist,    :genre,    :url,    :track,    :title,    :year,    :length,    :samplerate,    :composer,    :deviceid ]
        dbitems = collection.query( 'SELECT #{columns.to_s} FROM tags' )
        @items = Array.new
        0.step( dbitems.size, columns.size ) { |overallIt|
            track = Hash.new
            0.step( 3, 1 ) { |columnIt|
                track[ column_keys[columnIt] ] = collection[columnIt][:indexed][ dbitems[ overallIt + columnIt] ]
            }
            4.step( column_keys.size, 1 ) { |columnIt|
               track[ column_keys[columnIt] ] = dbitems[ overallIt + columnIt ]
            }
            @items.push(track)     
        }
    end
    
    def do_GET( req, resp )
        command = File.basename( req.path )
        case command
            when "databases" then
            # {"avdb"=>
            #   {"muty"=>[nil],
            #    "mstt"=>[200],
            #    "mrco"=>[1],
            #    "mtco"=>[1],
            #    "mlcl"=>
            #     [{"mlit"=>
            #        [{"miid"=>[1],
            #          "mper"=>[0],
            #          "minm"=>["Banshee Music Share"],
            #          "mctc"=>[1],
            #          "mimc"=>[1360]}]}]}}
                avdb = Element.new( 'avdb' )
                  avdb << Element.new( 'muty', 0 )
                  avdb << Element.new( 'mstt', WEBrick::HTTPStatus::OK.code )
                  avdb << Element.new( 'mrco', 1 )
                  avdb << Element.new( 'mtco', 1 )
                  mlcl = Element.new( 'mlcl' )
                  avdb << mlcl
                    mlit = Element.new( 'mlit' )
                    mlcl << mlit
                      mlit << Element.new( 'miid', 1 )
                      mlit << Element.new( 'mper', 0 )
                      mlit << Element.new( 'minm', "Amarok Music Share" ) #TODO i18n? not sure if user-visible
                      mlit << Element.new( 'mctc', 1 )
                      mlit << Element.new( 'mimc' @items.size )
                 resp.body = avdb.to_s
            when "items" then
            # {"adbs"=>
            #     {"muty"=>[nil],
            #     "mstt"=>[200],
            #     "mrco"=>[1360],
            #     "mtco"=>[1360],
            #     "mlcl"=>
            #         [{"mlit"=>
            #             {"asal"=>["Be Human: Ghost in the Shell"],
            #             "miid"=>[581],
            #             "astm"=>[86000],
            #             "minm"=>["FAX me"],
            #             "astn"=>[nil],
            #             "asar"=>["Yoko Kanno"],
            #             "ascm"=>[""]},
            #             ...
                adbs = Element.new( 'adbs' )
                  avdb << Element.new( 'muty', 0 )
                  avdb << Element.new( 'mstt', WEBrick::HTTPStatus::OK.code )
                  avdb << Element.new( 'mrco', @items.size )
                  avdb << Element.new( 'mtco', @items.size )
                  mlcl = Element.new( 'mlcl' )
                  avdb << mlcl
                  root = Element.new( 'mlit' ) 
                  #TODO
            else
                puts "unimplemented request #{req.path}
        end
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