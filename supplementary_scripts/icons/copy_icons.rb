#!/usr/bin/env ruby
#
# Transforms icon filenames from an icon tarball to correct format for committing
#
# (c) 2006 Mark Kretschmann <markey@web.de>
# Licensed under GPL V2.


require 'fileutils'


def copy_icons( res )
    folder = "#{res}x#{res}/actions"

    Dir.foreach( folder ) do |file|
        next if file[0, 1] == "."
        name = File.basename( file, ".png" )
        FileUtils.cp( "#{folder}/#{file}", "hi#{res}-action-#{name}.png" )
    end
end



copy_icons( "16" )
copy_icons( "22" )
copy_icons( "32" )
copy_icons( "48" )
copy_icons( "64" )
