#!/usr/bin/env ruby
#
# This is a convenience script for bumping Amarok's plugin framework version
# in the various engine desktop files and in pluginmanager.h.
#
# The script should be run once before each release, in order to ensure that
# no old and perhaps incompatible engines are getting loaded. After running, don't
# forget to commit to svn. The script must be started from the amarok/ folder.
#
# (c) 2005-2010 Mark Kretschmann <kretschmann@kde.org>
# License: GNU General Public License V2


def bump_desktop_files
    files = Dir["**/*.desktop"]

    files.each do |path|
        file = File.new(path, File::RDWR)
        str = file.read
        unless str[/X-KDE-Amarok-framework-version=[0-9]*/].nil?
            puts path
            str.sub!( /X-KDE-Amarok-framework-version=[0-9]*/, "X-KDE-Amarok-framework-version=#{@version}" )
            file.rewind
            file.truncate(0)
            file << str
        end
        file.close
    end
end


# Make sure the current working directory is amarok
if not Dir::getwd().split( "/" ).last() == "amarok"
    print "ERROR: This script must be started from the amarok/ folder. Aborting.\n\n"
    exit()
end


# Bump PluginFrameworkVersion in Amarok.h
file = File.new( "src/core/plugins/Plugin.h", File::RDWR )
str = file.read()
file.rewind()
file.truncate( 0 )
temp = str.scan( /static const int PluginFrameworkVersion = [0-9]*;/ )
@version = temp.join().scan( /[0-9]*/ ).join().to_i()
@version = @version + 1

print "Bumping the plugin framework version to: #{@version}"

str.sub!( /static const int PluginFrameworkVersion = [0-9]*;/, "static const int PluginFrameworkVersion = #{@version};" )
file << str
file.close()


# Bump plugin desktop files
puts
puts
bump_desktop_files


puts
puts
print "Done :) Now commit the source to git."
puts
puts

