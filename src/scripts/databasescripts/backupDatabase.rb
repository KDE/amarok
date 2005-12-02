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

destination = $*[0]

unless FileTest.directory?( destination )
    puts( "Error: Save destination must be a directory" )
    exit( 1 )
end

unless FileTest.writable_real?( destination )
    puts( "Error: Destination directory not writable." )
    exit( 1 )
end

filename = ""
database = `dcop amarok script readConfig DatabaseEngine`
database.chomp!()

case database

    when "0" # sqlite
        filename = "collection.db"

    when "1" # mysql
        filename = "amarokdb.mysql"

    when "2" # postgres
        error = "Sorry, postgresql database backups have not been implemented"
        `dcop amarok playlist popupMessage #{error}`
        exit( 1 )

end

filename = getFilename( filename )

