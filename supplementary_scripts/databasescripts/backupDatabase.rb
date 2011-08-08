#!/usr/bin/env ruby
#
#
# Ruby database backup script
# (c) 2005-2007 Seb Ruiz <ruiz@kde.org>
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
    puts( "Usage: backupDatabase.rb saveDirectory" )
    puts()
    puts( "Backup your Amarok database!" )
    exit( 1 )
end

destination = $*[0] + "/"

unless FileTest.directory?( destination )
    system("dcop", "amarok", "playlist", "popupMessage", "Error: Save destination must be a directory")
    exit( 1 )
end

unless FileTest.writable_real?( destination )
    system("dcop", "amarok", "playlist", "popupMessage", "Error: Destination directory not writable.")
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
        pass = `dcop amarok script readConfig MySqlPassword2`.chomp!()
        host = `dcop amarok script readConfig MySqlHost`.chomp!()
        port = `dcop amarok script readConfig MySqlPort`.chomp!()
        system("mysqldump", "-u", user, "-p"+pass, "-h", host, "-P", port, db, "-r", dest);

    when "2" # postgres
        filename = "amarokdb.psql"
        filename = getFilename( filename )
        dest = destination + "/" + filename
        puts dest
        db   = `dcop amarok script readConfig PostgresqlDbName`.chomp!()
        user = `dcop amarok script readConfig PostgresqlUser`.chomp!()
        pass = `dcop amarok script readConfig PostgresqlPassword`.chomp!()
        host = `dcop amarok script readConfig PostgresqlHost`.chomp!()
        port = `dcop amarok script readConfig PostgresqlPort`.chomp!()
        system("pg_dump", "-U", user, "-W", pass, "-h", host, "-p", port, db, "-f", dest);
end

system("dcop", "amarok", "playlist", "popupMessage", "Database backup saved to: #{destination}/#{filename}")
