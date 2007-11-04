#!/usr/bin/env ruby

require 'find'

class DirWalker

    def initialize()
    
        puts "Fired up and ready to go! \n\n"
    end

    def ParseLevel( rootFolder )

        level = 0;
        Find.find( rootFolder ) do |path|

            if FileTest.directory?(path)
                puts "// DIRECTORY: " << path << ", level: " << level.to_s << "\n"

                if level > 1
                    Find.prune
                    puts "prune"
		end
                
                if level == 1
    
			#We only want to handle one level at a time
			puts "... inserting!"
			
			system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "insertAlbum", "Scripted Files", path, "A directory" )
		end
                    
                level = level +1
               next
            else #we have a file
                if level == 1
                	puts "// FILE: " << path << "\n"
		end 
            end

        end
    end
    
end



if $*.empty?()

	# create new browser
	`qdbus org.kde.amarok /ScriptableServiceManager createDynamicService "Scripted Files" "files" "A simple demo of a scriptable service that parses  a directory tree and adds music files to a service" "/home/amarok-dev/amarok/src/servicebrowser/scriptableservice/file_browser_service_script.rb"`

else
	case $*[0]
		when "--populate_root"
			DirWalker.new.ParseLevel( "/home/amarok-dev/music/Brad Sucks" )
	end
end




