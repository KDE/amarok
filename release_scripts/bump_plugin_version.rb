#!/usr/bin/env ruby
#
# This is a convenience script for bumping amaroK's plugin framework version
# in the various engine desktop files and in pluginmanager.h.
#
# The script should be run once before each release, in order to ensure that
# no old and perhaps incompatible engines are getting loaded. After running, don't
# forget to commit to svn. The script must be started from the amarok/ folder.
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# License: GNU General Public License V2


# Make sure the current working directory is amarok
if not Dir::getwd().split( "/" ).last() == "amarok"
    print "ERROR: This script must be started from the amarok/ folder. Aborting.\n\n"
    exit()
end


# Bump FrameworkVersion in pluginmanager.h
file = File.new( "src/pluginmanager.h", File::RDWR )
str = file.read()
file.rewind()
file.truncate( 0 )
temp = str.scan( /static const int FrameworkVersion = [0-9]*;/ )
@version = temp.join().scan( /[0-9]*/ ).join().to_i()
@version = @version + 1

print "Bumping the plugin framework version to: #{@version}"

str.sub!( /static const int FrameworkVersion = [0-9]*;/, "static const int FrameworkVersion = #{@version};" )
file << str
file.close()


# Bump engine desktop files
print "\n\n"
Dir.chdir( "src" )
Dir.foreach( "engine" ) do |x|
    next if x[0, 1] == "."
    if FileTest.directory?( "engine/#{x}" )
        print x + "\n"
        file = File.new( Dir["engine/#{x}/*.desktop"].join(), File::RDWR )
        str = file.read()
        file.rewind()
        file.truncate( 0 )
        str.sub!( /X-KDE-amaroK-framework-version=[0-9]*/, "X-KDE-amaroK-framework-version=#{@version}" )
        file << str
        file.close()
    end
end


print "\n"
print "\n"
print "Done :) Now commit the source to SVN."
print "\n\n"
