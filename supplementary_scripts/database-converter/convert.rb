#!/usr/bin/ruby -w
# == Synopsis
#   Transfers statistics from Amarok 1.4 to Amarok 2.0 database.
#
# == Usage
#   convert [options] source_file
#
#   For help use: convert -h
#
# == Options
#   -h, --help          Displays help message
#   -q, --quiet         Output as little as possible, overrides verbose
#   -v, --verbose       Verbose output
#   TODO - add additional options
#
# == Author
#   Seb Ruiz
#
# == Copyright
#   Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>. Licensed under the GPL v2+ License


require "time"
require "optparse"
require "ostruct"
# require "mysql"
require "rdoc/usage"
require "sqlite3"

class Converter
    VERSION = '0.0.1'

    attr_reader :options

    def initialize( arguments, stdin )
        @arguments = arguments
        @stdin     = stdin

        # Set defaults
        @options = OpenStruct.new
        @options.verbose    = false
        @options.quiet      = false
        @options.safe_mode  = false
        @options.mysql_user = "amarok"
        @options.mysql_pass = "amarok"
        @options.mysql_host = "localhost"
        @options.mysql_db   = "amarok"
    end

    # Parse options, check arguments, then process the command
    def run
        if parsed_options?
            start = Time.now
            transferStatistics
            finish = Time.now
            
            puts "\nTime elapsed: #{finish - start}" if @options.verbose
        else
            output_usage
        end
    end

    protected

    def parsed_options?
        # Specify options
        opts = OptionParser.new
        opts.on('-h', '--help')       { output_help }
        opts.on('-v', '--verbose')    { @options.verbose = true }
        opts.on('-q', '--quiet')      { @options.quiet = true }
        opts.on('-s', '--safe-mode')  { @options.safe_mode = true }
        # TO DO - add additional options

        opts.parse!(@arguments)

        true
    rescue
        false
    end

    def output_options
        puts "Options:\n"
        @options.marshal_dump.each do |name, val|
            puts "  #{name} = #{val}"
        end
    end

    def output_help
        output_version
        RDoc::usage() #exits app
    end

    def output_usage
        RDoc::usage('usage') # gets usage from comments above
    end

    def output_version
        puts "#{File.basename(__FILE__)} version #{VERSION}"
    end

#     def establishMysqlConnection( server, username, password, database, verbose )
#         # connect to the MySQL server
#         conn = Mysql.real_connect( server, username, password, database )
#         # get server version string and display it
#         puts "Mysql server version: " + $mysqlConnection.get_server_info if verbose
#         return conn
#     rescue Mysql::Error => e
#         if verbose
#             puts "Error code: #{e.errno}"
#             puts "Error message: #{e.error}"
#             puts "Error SQLSTATE: #{e.sqlstate}" if e.respond_to?("sqlstate")
#         elsif
#             puts "Could not connect to MySQL database"
#         end
#         return nil
#     end

    def transferStatistics
        old_db = SQLite3::Database.new( "collection.db" )
        new_db = SQLite3::Database.new( "collection2.db" )

        old_db.results_as_hash = true
        new_db.results_as_hash = true

        update_count = 0;

        # de-dynamic collection
        devices_row = old_db.execute( "SELECT id, lastmountpoint FROM devices" )

        statistics_row = old_db.execute( "SELECT deviceid, url, createdate, percentage, rating, playcounter FROM statistics" )

        puts "Fetched #{statistics_row.size} rows"

        statistics_row.each do | tag |
            url      = tag["url"]
            deviceid = tag["deviceid"]

            devices_row.each do | device |
                if deviceid == device["id"]
                    url = "." + device["lastmountpoint"] + url.slice(1..url.length)
                    url.sub!( "\/\/", "\/" ) #filter out multiple forward slashes
                    break
                end
            end

            urlid = new_db.get_first_value( "SELECT id FROM urls WHERE rpath=?", url )

            updates = new_db.execute( "UPDATE statistics SET createdate=?, score=?, rating=?, playcount=? WHERE url=?",
                                       tag["createdate"], tag["percentage"], tag["rating"], tag["playcounter"], urlid );

            update_count += updates.size

            puts "Transferred statistics for: " + url if @options.verbose
        end
        puts
        puts "Updated #{update_count} statistics"
    end

end

converter = Converter.new(ARGV, STDIN)
converter.run



