#!/usr/bin/ruby

#A DAAP Server
# (c) 2006 Ian Monroe <ian@monroe.nu>
# License: GNU General Public License V2

require 'webrick'
require 'Korundum' 
require 'codes.rb'
require 'pp'

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
    
    @@instance = nil
    def self.get_instance( config, *options )
        @@instance = @@instance || self.new
    end
    
    def initialize
        artists = Array.new
        albums = Array.new
        genre = Array.new
        device_paths = Array.new

        collection = KDE::DCOPRef.new( 'amarok', 'collection' )
        indexes = [  { :dbresult=>collection.query( 'select * from album' ),  :indexed => albums }, 
           { :dbresult=>collection.query( 'select * from artist' ), :indexed => artists },
           { :dbresult=>collection.query( 'select * from genre' )  , :indexed => genre },
           { :dbresult=>collection.query( 'select id, lastmountpoint from devices' ), :indexed => device_paths } ]
        indexes.each { |h|
             0.step( h[ :dbresult ].size, 2 ) { |i|
                h[ :indexed ][ h[ :dbresult ][i].to_i ] = h[ :dbresult ][ i.to_i+1 ]
            }
        }
        columns =     [ "album, ", "artist, ", "genre, ", "track, ", "title, ", "year, ", "length, ", "samplerate, ", "url, ", "deviceid" ]
        @column_keys = [ :songalbum, :songartist, :songgenre,  :songtracknumber, :itemname, :songyear, :songtime, :songsamplerate, :url,  :deviceid ]
        #TODO composer :songcomposer
        dbitems = collection.query( "SELECT #{columns.to_s} FROM tags LIMIT 10" )

        @items = Array.new
        @music = Array.new
        id = 0
        0.step( dbitems.size - columns.size, columns.size ) { |overallIt|
            track = Hash.new
            0.step( 3, 1 ) { |columnIt|
                track[ @column_keys[columnIt] ] = indexes[columnIt][ :indexed ][ dbitems[ overallIt + columnIt].to_i ]
            }
            4.step( @column_keys.size-2, 1 ) { |columnIt|
               track[ @column_keys[columnIt] ] = dbitems[ overallIt + columnIt ]
            }
            if overallIt > (dbitems.size - 500) then
                puts dbitems[ overallIt, overallIt + @column_keys.size].inspect
            end
            columnIt = @column_keys.size-2
            id += 1
            url = dbitems[ overallIt + columnIt ].reverse.chop.reverse
            url[0] = ''
         #   puts "indexes: #{dbitems[ columnIt + overallIt + 1 ]} - #{indexes[3][:indexed][ dbitems[ columnIt + overallIt + 1 ].to_i ]} #{url}"
            @music[id] = "#{indexes[3][:indexed][ dbitems[ columnIt + overallIt + 1 ].to_i ]}/#{url}"
            track[ :itemid ] = id
            ext = File::extname( url ).reverse.chop.reverse;
            track[ :songformat ] = ext
            @items.push(track)
        }
        @column_keys.push( :itemid )
        @column_keys.push( :songformat )
    end
    
    def do_GET( req, resp )
        if @items.nil? then
            initItems()
        end
    
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
                      mlit << Element.new( 'minm', ENV['USER'] + " Amarok" )
                      mlit << Element.new( 'mctc', 1 )
                      mlit << Element.new( 'mimc', @items.size )
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
                requested = req.query['meta'].split(',')
                toDisplay =  Array.new
                requested.each { |str|
                    str[0,5] = ''
                    index = str.to_sym
                    if @column_keys.include?( index ) then
                        if( METAS[ index ] )
                            toDisplay.push( { :index=>index, :code=> METAS[ index ][:code] } )
                        else
                            puts "not being displayed #{index.to_s}"
                        end
                    end
                }
                  adbs = Element.new( 'adbs' )
                  adbs << Element.new( 'muty', 0 )
                  adbs << Element.new( 'mstt', WEBrick::HTTPStatus::OK.code )
                  adbs << Element.new( 'mrco', @items.size )
                  adbs << Element.new( 'mtco', @items.size )
                  mlcl = Element.new( 'mlcl' )
                  adbs << mlcl
                  @items.each { |item|
                    mlit = Element.new( 'mlit' )
                    toDisplay.each{  |meta|                        
                        mlit << Element.new( meta[:code], item[ meta[:index] ] || 0 )
                    }
                    mlcl << mlit
                  }
                 puts adbs.to_s.inspect
                 resp.body = adbs.to_s
            else if command =~ /([\d]*)\.(.*)$/ #1232.mp3
                    puts "sending #{@music[ $1.to_i ]}"
                    resp.body = open( @music[ $1.to_i ] )
                else
                    puts "unimplemented request #{req.path}"
                end
        end
    end
end

class Controller

    def initialize(a)
        server = WEBrick::HTTPServer.new( { :Port=>8081 } )
        ['INT', 'TERM'].each { |signal|
            trap(signal) { server.shutdown; a.quit }
        }
        server.mount( '/login', LoginServlet )
        server.mount( '/update', UpdateServlet )
        server.mount( '/databases', DatabaseServlet )
        server.start
    end

end

class Dummy < Qt::Object
    slots 'itStarts()'    
    
    def itStarts()
        Controller.new( parent() )
    end
end

about = KDE::AboutData.new("amarok_daapserver", 
"amarok_daapserver", "1.4.2")
KDE::CmdLineArgs.init(ARGV, about)
a = KDE::Application.new()
d = Dummy.new( a, "dummyobject" )

Qt::Timer::singleShot( 0, d, SLOT( "itStarts()" ) );

a.exec()

