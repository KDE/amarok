#!/usr/bin/env ruby
#
#
# Ruby database backup script
# (c) 2005 Seb Ruiz <me@sebruiz.net>
# Released under the GPL v2 license


def getFilename( input )
    puts input
    date = `date +%Y%m%d`
    i = 1
    input = input + "." + date
    original = input

    # don't overwrite a previously written backup
    loop do
        file = File.dirname( File.expand_path( __FILE__ ) ) + "/" + input

        if not FileTest.exist?( file )
            return input
        end

        i = i + 1
        newName = original + "." + i
        input = newname
    end
end

if $*.empty?() or $*[0] == "--help"
    puts( "Usage: backupDatabase.rb saveLocation" )
    puts()
    puts( "Backup your amaroK database!" )
    exit( 1 )
end

destination = $*[0] + "/"

unless FileTest.directory?( destination )
    error = "Error: Save destination must be a directory"
    `dcop amarok playlist popupMessage '#{error}'`
    exit( 1 )
end

unless FileTest.writable_real?( destination )
    error = "Error: Destination directory not writable."
    `dcop amarok playlist popupMessage '#{error}'`
    exit( 1 )
end

filename = ""
database = `dcop amarok script readConfig DatabaseEngine`.chomp!()

case database

    when "0" # sqlite
        filename = "collection.db"
        filename = getFilename( filename )
        dest = destination + "/" + filename
        puts dest
        `cp ~/.kde/share/apps/amarok/collection.db #{dest}`

    when "1" # mysql
        filename = "amarokdb.mysql"
        filename = getFilename( filename )
        dest = destination + "/" + filename
        puts dest
        db   = `dcop amarok script readConfig MySqlDbName`.chomp!()
        user = `dcop amarok script readConfig MySqlUser`.chomp!()
        pass = `dcop amarok script readConfig MySqlPassword`.chomp!()
        `mysqldump -u #{user} -p#{pass} #{db} > #{dest}`

    when "2" # postgres
        error = "Sorry, postgresql database backups have not been implemented"
        `dcop amarok playlist popupMessage #{error}`
        exit( 1 )
end

message = "Database backup saved to: #{destination}/#{filename}"
`dcop amarok playlist popupMessage '#{message}'`

