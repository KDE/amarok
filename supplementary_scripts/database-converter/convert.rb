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
require "mysql"
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
        @options.source     = "mysql"
        @options.username   = ""
        @options.password   = ""
        @options.hostname   = ""
        @options.database   = ""
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
        opts.on("-u [USERNAME]", "Database username (mysql/psql)") do |user|
            @options.username = user 
        end
        opts.on("-p [PASSWORD]", "Database password (mysql/psql)") do |pass|
            @options.password = pass 
        end
        opts.on("-h [HOSTNAME]", "Database hostname (mysql/psql)") do |host|
            @options.hostname = host 
        end
        opts.on("-d [DATABASE]", "Database name (mysql/psql)") do |name|
            @options.database = name 
        end

        opts.parse!(@arguments)

        true
    rescue => e
        puts "#{e}"
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

    # connect to the MySQL server
    # get server version string and display it
 
    def getMysqlConnection()
        conn = Mysql.real_connect( @options.hostname, @options.username, @options.password, @options.database )
        puts "Mysql server version: " + conn.get_server_info if @options.verbose
        return conn
    rescue Mysql::Error => e
        if @options.verbose
            puts "Error message: #{e.error}"
        elsif
            puts "Could not connect to Amarok 1.4 MySQL database"
        end
    end

    def getAmarok2Collection()
        filename = "collection2.db1"
        if not File.exist?( filename )
            raise "File #{filename} does not exist"
        elsif not File.writable?( filename )
            raise "File #{filename} is not writable"
        end
        return SQLite3::Database.new( "collection2.db" );
    rescue => e
        puts "Error: #{e}"
    end

    def transferStatistics
        conn = getMysqlConnection()
        return if conn.nil?

        amarok2_collection = getAmarok2Collection()
        return if amarok2_collection.nil?

        amarok2_collection.results_as_hash = true

        updateCount = 0;
        errorCount = 0;

        # de-dynamic collection
        devices_row    = conn.query( "SELECT id, lastmountpoint FROM devices" )
        statistics_row = conn.query( "SELECT deviceid, url, createdate, accessdate, percentage, rating, playcounter FROM statistics" )

        puts "Fetched #{statistics_row.num_rows} rows"

        staleEntries = Array.new

        statistics_row.each_hash do | tag |
            begin
            url      = tag["url"]
            deviceid = tag["deviceid"]

            devices_row.each_hash do | device |
                if deviceid == device["id"]
                    url = "." + device["lastmountpoint"] + url.slice(1..url.length)
                    url.sub!( "\/\/", "\/" ) #filter out multiple forward slashes
                    break
                end
            end

            urlid = amarok2_collection.get_first_value( "SELECT id FROM urls WHERE rpath=?", url )
            if urlid.nil?
                staleEntries << url
                next
            end
          
            #if not @options.safe_mode
            query = "INSERT INTO statistics (url, createdate, accessdate, score, rating, playcount ) VALUES ( " +
                    "#{urlid}, #{tag["createdate"]}, #{tag["accessdate"]}, #{tag["percentage"]}, #{tag["rating"]}, #{tag["playcounter"]} );"

            updates = amarok2_collection.execute( query );
            #else
            #    puts "Safe mode!"
            #end
            
            updateCount += 1

            #puts "UPDATE statistics SET createdate=#{tag["createdate"]}, score=#{tag["percentage"]}, rating=#{tag["rating"]}, " +
            #     " playcount=#{tag["playcounter"]} WHERE url=#{urlid}" if @options.verbose
            rescue SQLite3::SQLException => e
                if @options.verbose
                    puts "Insertion error: #{e}"
                    #puts query
                end
                errorCount += 1
            end
        end
        puts
        puts "#{updateCount} statistic updateCount"
        puts "#{errorCount} errorCount encountered"
        puts "#{staleEntries.size} stale entries ignored"

        if @options.verbose
            puts "Stale entries: "
            staleEntries.each {|e| print "    ", e, "\n" }
        end

        devices_row.free
        statistics_row.free
    rescue Mysql::Error => e
        if @options.verbose
            puts "Error message: #{e.error}"
        elsif
            puts "Could not connect to MySQL database"
        end
    ensure
        conn.close if conn
    end

end

converter = Converter.new(ARGV, STDIN)
converter.run



