#!/usr/bin/env ruby

#A DAAP Server
# (c) 2006 Ian Monroe <ian@monroe.nu>
# License: GNU General Public License V2
$LOAD_PATH.push(ARGV[0])
puts "here it is: #{ARGV[0]}"
$LOAD_PATH.push(ARGV[1])
puts "here it is: #{ARGV[1]}"

require "codes.rb"
require 'mongrel'
require "#{ARGV[2]}" #debug.rb

require 'uri'
require 'pp'
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
                    log "type error! #{@value} #{CODE_TYPE[@name]} #{@name}"
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

class DatabaseServlet < Mongrel::HttpHandler
  include DebugMethods
  public

      def initItems
          @@sessionId = 42
          artists = Hash.new
          albums = Hash.new
          genre = Hash.new
          year = Hash.new
          device_paths = Hash.new
          indexes = [  { :dbresult=> query( 'select * from album' ),  :indexed => albums },
          { :dbresult=> query( 'select * from artist' ), :indexed => artists },
          { :dbresult=> query( 'select * from genre' )  , :indexed => genre },
          { :dbresult=> query( 'select * from year' )  , :indexed => year },
          { :dbresult=> query( 'select id, lastmountpoint from devices' ), :indexed => device_paths } ]
          indexes.each { |h|
              0.step( h[ :dbresult ].size, 2 ) { |i|
                  h[ :indexed ][ h[ :dbresult ][i].to_i ] = h[ :dbresult ][ i.to_i+1 ]
              }
          }

          columns =     [ "album, ", "artist, ", "genre, ", "year, ", "track, ", "title, ", "length, ", "samplerate, ", "url, ", "deviceid" ]
          puts "SQL QUERY: SELECT #{columns.to_s} FROM tags"
          @column_keys = [ :songalbum, :songartist, :songgenre, :songyear, :songtracknumber, :itemname, :songtime, :songsamplerate, :url,  :deviceid ]
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
              when 0..3
                  track << Element.new( METAS[ @column_keys[columnIt] ][ :code ], indexes[columnIt][ :indexed ][ line.to_i ] )
              when (4 ..( @column_keys.size-3 ) )
                  track << Element.new( METAS[ @column_keys[columnIt] ][ :code ], line )
              when columns.size - 2
                  url = line.reverse.chop.reverse
              when columns.size - 1
                  id += 1
                  device_id = line.to_i
                  if device_id == -1 then
                      @music[id] = url
                  else
                      url[0] = ''
                      @music[id] = "#{indexes.last[:indexed][ device_id ]}/#{url}"
                  end
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

      def process( request, response )
          if @items.nil? then
              initItems()
          end
          uri = URI::parse( request.params["REQUEST_URI"] )
          command = File.basename( uri.path )
          output = String.new
          case command
              #{"mupd"=>{"mstt"=>[200], "musr"=>[2]}}
              when "login" then
                  root =  Element.new( 'mlog' )
                  root << Element.new( 'mlid', @@sessionId )
                  root << Element.new( 'mstt',  200 ) #200, as in the HTTP OK code
                  write_resp( response, root.to_s )
                  @@sessionId += 1
              #{"mupd"=>{"mstt"=>[200], "musr"=>[2]}}
              when "update" then
                  root = Element.new( 'mupd' )
                  root << Element.new( 'mstt', 200 )
                  root << Element.new( 'musr', 2 )
                  write_resp( response, root.to_s )
              when 'server-info'
# SimpleDaapClient output from Banshee server
#               {"msrv"=>
#                 {"mpro"=>[131074],
#                 "msbr"=>[nil],
#                 "mslr"=>[nil],
#                 "msup"=>[nil],
#                 "msex"=>[nil],
#                 "msqy"=>[nil],
#                 "msau"=>[nil],
#                 "apro"=>[196610],
#                 "minm"=>["Banshee Music Share"],
#                 "msdc"=>[1],
#                 "mstt"=>[200],
#                 "msal"=>[nil],
#                 "msrs"=>[nil],
#                 "mstm"=>[1800],
#                 "mspi"=>[nil],
#                 "msix"=>[nil]}}
                  msrv = Element.new( 'msrv' )
                    msrv << Element.new( 'mpro', 0x20002 )
                    msrv << Element.new( 'msbr', 1 )
                    msrv << Element.new( 'mslr', 1 )
                    msrv << Element.new( 'msup', 1 )
                    msrv << Element.new( 'msex', 1 )
                    msrv << Element.new( 'msqy', 1 )
                    msrv << Element.new( 'msau', 0 )
                    msrv << Element.new( 'apro', 0x30002 )
                    msrv << Element.new( 'minm', "Amarok Music Share" )
                    msrv << Element.new( 'msdc', 1 )
                    msrv << Element.new( 'mstt', 200 )
                    msrv << Element.new( 'msal', 1 )
                    msrv << Element.new( 'msrs', 1 )
                    msrv << Element.new( 'mstm', 1800 )
                    msrv << Element.new( 'mspi', 1 )
                    msrv << Element.new( 'msix', 1 )
                  write_resp( response, msrv.to_s )
              when 'content-codes' then
                  write_resp( response, CONTENT_CODES ) #LAAAAAZY
              when 'containers' then
              # {"aply"=>
              #   {"muty"=>[nil],
              #    "mstt"=>[200],
              #    "mrco"=>[1],
              #    "mtco"=>[1],
              #    "mlcl"=>
              #     [{"mlit"=>
              #        [{"abpl"=>[nil],
              #          "miid"=>[1],
              #          "mper"=>[0],
              #          "minm"=>["Banshee Music Share"],
              #          "mimc"=>[2463]}]}]}}
                aply = Element.new( 'aply' )
                    aply << Element.new( 'muty', nil )
                    aply << Element.new( 'mstt', 200 )
                    aply << Element.new( 'mrco', 1 )
                    mlcl =  Element.new( 'mlcl')
                    aply << mlcl
                        mlit = Element.new( 'mlit' )
                        mlcl << mlit
                            mlit << Element.new( 'abpl', nil )
                            mlit << Element.new( 'miid', 1 )
                            mlit << Element.new( 'mper', 0 )
                            mlit << Element.new( 'minm', "Amarok Music Share" )
                            mlit << Element.new( 'mimc', @items.size )
                write_resp( response, aply.to_s )
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
                  avdb << Element.new( 'mstt', 200 )
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
                  write_resp( response, avdb.to_s )
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
                  requested = uri.query.nil? ? Array.new : Mongrel::HttpRequest.query_parse( uri.query )['meta'].split(',')
                  puts "#{request.params.inspect} #{requested.inspect} #{uri.to_s} #{request.params["REQUEST_URI"]}"
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
                  adbs << Element.new( 'mstt', 200 )
                  adbs << Element.new( 'mrco', @items.size )
                  adbs << Element.new( 'mtco', @items.size )
                  adbs << @items
                  write_resp( response, adbs.to_s( toDisplay ) )
              else if command =~ /([\d]*)\.(.*)$/ #1232.mp3
                      log "sending #{@music[ $1.to_i ]}"
                      file = @music[ $1.to_i ]
                      response.start(200) do |head,out|
                          response.send_status(File.size( file ))
                          response.header['Content-Type'] = "application/#{$2}"
                          response.send_header
                          response.send_file(file)
                      end
                      response.send_header
                      response.send_file( file )
                  else
                      response.start( 404 ) do | head, out |
                        out << "Command not implemented."
                      end
                      puts "#{command} not implemented"
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

      def write_resp( response, value )
          response.start do | head, out |
              head['DAAP-Server'] = 'amarok-kaylee'
              head['Content-Type'] = 'application/x-dmap-tagged'
              out << value
          end
      end
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
                server = Mongrel::HttpServer.new('0.0.0.0', port)
                no_server = false
            rescue Errno::EADDRINUSE
                if port == 3700 then
                    fatal( "No ports between 3688 and 3700 are open." )
                end
                port += 1
            end
        end
        ds = DatabaseServlet.new
        server.register('/', ds )
        server.register('daap', ds )
        puts "SERVER STARTING: #{port}"
        server.run.join
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
