#!/usr/bin/env ruby

require 'find'

@service_name = "Demo File Browser"

class DirWalker

    def initialize()
        puts "Fired up and ready to go! \n\n"
    end

    def ParseLevel( rootFolder, level, parent )

        service_name = "Demo File Browser"

        Find.find( rootFolder ) do |path|

            if FileTest.directory?(path)

                if path == rootFolder
                    next
                end
                
                puts "Found directory: " + path
                if level != "0"

                    puts "Path name: " + service_name
                
                    #We only want to handle one level at a time
                    system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "insertItem", service_name, level, parent, File.basename( path ), "A directory", path.gsub( " ", "%20" ), "" )
                    Find.prune
                end

                next

            else #we have a file
                if level == "0"
                	puts "// FILE: " << File.basename(path).gsub( File.extname(path), "" ) << "\n"
                    puts "... inserting!"
                    system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "insertItem", service_name, level, parent, File.basename(path).gsub( File.extname(path), "" ), "A file", "", path )
		        end
            end
        end
    end
end



dir_walker = DirWalker.new

#this one should really be made configurable....
base_path = "/home/nhn/media/"


#for debug purposes only:
#dir_walker.ParseLevel( "/home/nhn/media/cc/Artemis/Gravity", "0", "1" )


loop do
    message = gets
    puts "script got message" + message
    args = message.chomp.split(" ")

    case args[0]
    
        when "configure"
            `qdbus org.kde.amarok /Playlist popupMessage "This script does not require any configuration."`
            
        when "init"

            #4 levels, genres, artists, albums and tracks
            levels = "4"
            short_description = "Simple demo scripted file browser"
            root_html = "A small scripted service that recursively parses a directory structure. Not intended for any real use, but just as a small demo ( Done on a day with no internet access and nothing better to test the scriptable service framework against"

            # init new browser
            system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "initService", @service_name, levels, short_description, root_html )

        when "populate"

            level = args[1]
            parent_id = args[2]
            path = args[3]

            if level == "3"
                dir_walker.ParseLevel( base_path, level, "-1" )
            else
                dir_walker.ParseLevel( path.gsub( "%20", " " ) , level, parent_id )
            end

            system("qdbus", "org.kde.amarok", "/ScriptableServiceManager",  "donePopulating", @service_name, level )
            
        end
    end
#end




