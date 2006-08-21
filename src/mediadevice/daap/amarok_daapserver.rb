#!/usr/bin/env ruby

#A DAAP Server
# (c) 2006 Ian Monroe <ian@monroe.nu>
# License: GNU General Public License V2

require "#{ARGV[0]}" #codes.rb
require "#{ARGV[1]}" #debug.rb
require 'webrick'
#require 'rubygems'
#require 'ruby-prof'

$app_name = "Daap"
$debug_prefix = "Server"

class Element
    attr_accessor :name

    public 
        def initialize(name, value = Array.new)
            @name, @value = name, value
        end
        
        def to_s( codes = nil )
            if @value.nil? then
                log @name + ' is null'
                @name + Element.long_convert( 0 )
            else
                content = valueToString( codes )
                @name + Element.long_convert(content.length) + content
            end
        end
        
        def collection?
            @value.class == Array
        end
        
        def <<( child )
            @value << child
        end

        def size
            @value.size
        end

        def Element.char_convert( v ) 
            packing( v, 'c' )
        end

        def Element.short_convert( v )
            packing( v, 'n' )
        end

        def Element.long_convert( v )
            packing( v, 'N' )
        end

        def Element.longlong_convert( v )
            v = v.to_i  if( v.is_a?(String) )
            a = Array.new
            a[0] = v >> 32
            b = Array.new
            b[0] = v & 0xffffffff
            a.pack('N') + b.pack('N')
        end
 
    protected
        def valueToString( codes )
            case CODE_TYPE[@name]
                when :string then
                    @value
                when :long then 
                    Element.long_convert( @value )
                when :container then
                    values = String.new
                    @value.each do |i|
                        values += i.to_s( codes )
                    end
                    values
                when :char then
                    Element.char_convert( @value )
                when :short then
                    Element.short_convert( @value )
                when :longlong then
                    Element.longlong_convert( @value )
                when :date then
                    Element.long_convert( @value )
                when :version then
                    Element.short_convert( @value )
                else
                    log "type error! #{@value} #{CODE_TYPE[@name]}"
            end
        end

        def Element.packing( v, packer )
            v = v.to_i  if( v.is_a?(String) )
            a = Array.new
            a[0] = v
            a.pack(packer)
        end

end

class Mlit < Element
	attr_accessor :songformat, :id
    def to_s( codes )
        values = String.new
        @value.each { |i|
            values += i.to_s( codes ) if codes.member?( i.name )
        }
        'mlit' + Element.long_convert(values.length) + values
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
        log resp.body.dump
        @@sessionId += 1
    end
end

#{"mupd"=>{"mstt"=>[200], "musr"=>[2]}}
class UpdateServlet < WEBrick::HTTPServlet::AbstractServlet
    include DebugMethods
    
    debugMethod(:do_GET)
    def do_GET( req, resp )
        root = Element.new( 'mupd' )
        root << Element.new( 'mstt', WEBrick::HTTPStatus::OK.code )
        root << Element.new( 'musr', 2 )
        resp.body = root.to_s
        log resp.body.dump
    end

end

class DatabaseServlet < WEBrick::HTTPServlet::AbstractServlet
  include DebugMethods
  public
      @@instance = nil
      def self.get_instance( config, *options )
          @@instance = @@instance || self.new
      end

      def initialize
          artists = Hash.new
          albums = Hash.new
          genre = Hash.new
          device_paths = Hash.new
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
          puts "SQL QUERY: SELECT #{columns.to_s} FROM tags"
          @column_keys = [ :songalbum, :songartist, :songgenre,  :songtracknumber, :itemname, :songyear, :songtime, :songsamplerate, :url,  :deviceid ]
          #TODO composer :songcomposer
          @music = Array.new
          @items = Element.new( 'mlcl' )
          id = 0
          columnIt = 0
          track = Mlit.new( 'mlit' )
          url = String.new
          while ( line = $stdin.gets ) && ( line.chop! != '**** END SQL ****' )
              puts "#{columnIt} - #{line}" if id < 10
              case columnIt
              when 0..2
                  track << Element.new( METAS[ @column_keys[columnIt] ][ :code ], indexes[columnIt][ :indexed ][ line.to_i ] )
              when (3 ..( @column_keys.size-3 ) )
                  track << Element.new( METAS[ @column_keys[columnIt] ][ :code ], line )
              when columns.size - 2
                  url = line.reverse.chop.reverse
              when columns.size - 1
                  device_id = line.to_i
                  if device_id == -1 then
                      @music[id] = url
                  else
                      url[0] = ''
                      @music[id] = "#{indexes[3][:indexed][ device_id ]}/#{url}"
                  end
                  id += 1
                  track << Element.new( 'miid', id )
                  track << Element.new( 'asfm', File::extname( url ).reverse.chop.reverse )
                  @items << track
                  columnIt = -1
                  track = Mlit.new( 'mlit' )
                  url = String.new
              end
              columnIt += 1
          end
          @column_keys.push( :itemid )
          @column_keys.push( :songformat )
      end
      debugMethod(:new)

      def do_GET( req, resp )
          if @items.nil? then
              initItems()
          end
      
          command = File.basename( req.path )
          case command
              when "login" then
                  root =  Element.new( 'mlog' )
                  root << Element.new( 'mlid', @@sessionId )
                  root << Element.new( 'mstt',  WEBrick::HTTPStatus::OK.code )
                  resp.body = root.to_s
                  log resp.body.dump
                  @@sessionId += 1
              when "update" then
                  root = Element.new( 'mupd' )
                  root << Element.new( 'mstt', WEBrick::HTTPStatus::OK.code )
                  root << Element.new( 'musr', 2 )
                  resp.body = root.to_s
                  log resp.body.dump
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
                              toDisplay.push( METAS[ index ][:code] )
                          else
                              log "not being displayed #{index.to_s}"
                          end
                      end
                  }
                  adbs = Element.new( 'adbs' )
                  adbs << Element.new( 'muty', nil )
                  adbs << Element.new( 'mstt', WEBrick::HTTPStatus::OK.code )
                  adbs << Element.new( 'mrco', @items.size )
                  adbs << Element.new( 'mtco', @items.size )
                  adbs << @items
                  resp.body = adbs.to_s( toDisplay )
              else if command =~ /([\d]*)\.(.*)$/ #1232.mp3
                      log "sending #{@music[ $1.to_i ]}"
                      resp.body = open( @music[ $1.to_i ] )
                  else
                      log "unimplemented request #{req.path}"
                  end
          end
      end
      debugMethod(:do_GET)

  private
      def query( sql )
          out = Array.new
          puts "SQL QUERY: #{sql}"
          while ( line = $stdin.gets) && (line.chop! != '**** END SQL ****' )
              out.push( line )
          end
          out
      end
      debugMethod(:query)
end

def log( string )
  f = open('/tmp/test.ruby', File::WRONLY | File::APPEND | File::CREAT )
  f.puts( string )
  f.close
end

class Controller

    def initialize
        port = 3689
        no_server = true
        while no_server 
            begin
                server = WEBrick::HTTPServer.new( { :ServerName=>'127.0.0.1', :Port=>port } )
                no_server = false
            rescue Errno::EAFNOSUPPORT
                if port == 3700 then
                    fatal( "No ports between 3688 and 3700 are open." )
                end
                port += 1
            end
        end
        ['INT', 'TERM'].each { |signal|
            trap(signal) { 
                server.shutdown
            }
        }
       # server.mount( '/login', LoginServlet )
       # server.mount( '/update', UpdateServlet )
       # server.mount( '/databases', DatabaseServlet )
        server.mount( '/', DatabaseServlet )
        puts "SERVER STARTING: #{port}"
        server.start
    end

end

$stdout.sync = true
$stderr.sync = true

#RubyProf.start
Controller.new

#result = RubyProf.stop
#printer = RubyProf::GraphHtmlPrinter.new(result)
#f = open('/tmp/test.html', File::WRONLY | File::CREAT )
#printer.print( f, 3 )