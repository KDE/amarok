#!/usr/bin/env ruby

#A DAAP Server
# (c) 2006 Ian Monroe <ian@monroe.nu>
# License: GNU General Public License V2

require 'webrick'
require 'pp'

class Element
    #attr_accessor :length, :name, :value
    public 
        def initialize(name, value = Array.new)
            @name, @value = name, value
        end
        
        def to_s
            if @value.nil? then
                debug @name + ' is null'
                @name + long_convert( 0 )
            else
                content = valueToString()
                @name + long_convert(content.length) + content
            end
        end
        
        def collection?
            @value.class == Array
        end
        
        def <<( child )
            @value << child
        end
    private
        def valueToString
            case CODE_TYPE[@name]
                when :char then
                    char_convert( @value )
                when :short then
                    short_convert( @value )
                when :long then 
                    long_convert( @value )
                when :longlong then
                    longlong_convert( @value )
                when :string then
                    @value
                when :date then
                    long_convert( @value )
                when :version then
                     short_convert( @value )
                when :container then
                    values = String.new
                    @value.each do |i|
                        values += i.to_s
                    end
                    values
                else
                    debug "type error! #{@value} #{CODE_TYPE[@name]}"
            end
        end
        
        def char_convert( v ) 
            packing( v, 'c' )
        end
        
        def short_convert( v )
            packing( v, 'n' )
        end
        
        def long_convert( v )
            packing( v, 'N' )
        end
        
        def longlong_convert( v )
            v = v.to_i  if( v.is_a?(String) )
            a = Array.new
            a[0] = v >> 32
            b = Array.new
            b[0] = v & 0xffffffff
            a.pack('N') + b.pack('N')
        end
 
        def packing( v, packer )
            v = v.to_i  if( v.is_a?(String) )
            a = Array.new
            a[0] = v
            a.pack(packer)
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
        debug resp.body.dump
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
        debug resp.body.dump
    end

end

class DatabaseServlet < WEBrick::HTTPServlet::AbstractServlet

  public
      @@instance = nil
      def self.get_instance( config, *options )
          @@instance = @@instance || self.new
      end

      def initialize
          artists = Array.new
          albums = Array.new
          genre = Array.new
          device_paths = Array.new

          indexes = [  { :dbresult=> query( 'select * from album' ),  :indexed => albums }, 
          { :dbresult=> query( 'select * from artist' ), :indexed => artists },
          { :dbresult=> query( 'select * from genre' )  , :indexed => genre },
          { :dbresult=> query( 'select id, lastmountpoint from devices' ), :indexed => device_paths } ]
          indexes.each { |h|
              0.step( h[ :dbresult ].size, 2 ) { |i|
                  h[ :indexed ][ h[ :dbresult ][i].to_i ] = h[ :dbresult ][ i.to_i+1 ]
              }
          }
          columns =     [ "album, ", "artist, ", "genre, ", "track, ", "title, ", "year, ", "length, ", "samplerate, ", "url, ", "deviceid" ]
          @column_keys = [ :songalbum, :songartist, :songgenre,  :songtracknumber, :itemname, :songyear, :songtime, :songsamplerate, :url,  :deviceid ]
          #TODO composer :songcomposer
          dbitems = query( "SELECT #{columns.to_s} FROM tags" )

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
                  debug dbitems[ overallIt, overallIt + @column_keys.size].inspect
              end
              columnIt = @column_keys.size-2
              id += 1
              url = dbitems[ overallIt + columnIt ].reverse.chop.reverse
              url[0] = ''
          #   debug "indexes: #{dbitems[ columnIt + overallIt + 1 ]} - #{indexes[3][:indexed][ dbitems[ columnIt + overallIt + 1 ].to_i ]} #{url}"
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
                              debug "not being displayed #{index.to_s}"
                          end
                      end
                  }
                  adbs = Element.new( 'adbs' )
                  adbs << Element.new( 'muty', nil )
                  adbs << Element.new( 'mstt', WEBrick::HTTPStatus::OK.code )
                  adbs << Element.new( 'mrco', @items.size )
                  adbs << Element.new( 'mtco', @items.size )
                  mlcl = Element.new( 'mlcl' )
                  adbs << mlcl
                  @items.each { |item|
                      mlit = Element.new( 'mlit' )
                      toDisplay.each{  |meta|                        
                          mlit << Element.new( meta[:code], item[ meta[:index] ] )
                      }
                      mlcl << mlit
                  }
                  debug adbs.to_s.inspect
                  resp.body = adbs.to_s
              else if command =~ /([\d]*)\.(.*)$/ #1232.mp3
                      debug "sending #{@music[ $1.to_i ]}"
                      resp.body = open( @music[ $1.to_i ] )
                  else
                      debug "unimplemented request #{req.path}"
                  end
          end
      end
  private

      def query( sql )
          out = String.new
          $stderr.puts "SQL QUERY: #{sql}"
          out += line while (line = $stdin.gets) && (line.chop != '**** END SQL ****')
          out
      end
end

def debug( string )
  f = open('/tmp/test.ruby', File::WRONLY | File::APPEND | File::CREAT )
  f.puts( string )
  f.close
end

class Controller

    def initialize
        server = WEBrick::HTTPServer.new( { :Port=>8081 } )
        ['INT', 'TERM'].each { |signal|
            trap(signal) { 
                server.shutdown
            }
        }
        server.mount( '/login', LoginServlet )
        server.mount( '/update', UpdateServlet )
        server.mount( '/databases', DatabaseServlet )
        server.start
    end

end

require "#{ARGV[0]}/codes.rb"
Controller.new