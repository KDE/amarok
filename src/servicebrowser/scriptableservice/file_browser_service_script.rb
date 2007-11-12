#!/usr/bin/env ruby

require 'find'

class DirWalker

    def initialize()
    
        puts "Fired up and ready to go! \n\n"
    end

    def ParseLevel( rootFolder, parent )

        level = 0;
        Find.find( rootFolder ) do |path|

            if FileTest.directory?(path)
                puts "// DIRECTORY: " << path << ", level: " << level.to_s << "\n"


                
                if level == 1
			#We only want to handle one level at a time
			puts "... inserting!"
                system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "insertAlbum", "Scripted Files", File.basename(path), "A directory", path )
                        Find.prune
		else 
                    level = level +1
                end

                next

            else #we have a file
                if level == 1
                	puts "// FILE: " << path << "\n"
                        puts "... inserting!"
                system("qdbus", "org.kde.amarok", "/ScriptableServiceManager", "insertTrack", "Scripted Files", File.basename(path), "A track", path, parent )
		end 
            end

        end
    end
    
end


puts " >>>>>>>>>>>>>> Radio station script called with: '" + $*[0].to_s() + "', '"  +$*[1].to_s()+ "', '" + $*[2].to_s() + "'"

if $*.empty?()

	# create new browser
	`qdbus org.kde.amarok /ScriptableServiceManager createDynamicService "Scripted Files" "files" "A simple demo of a scriptable service that parses  a directory tree and adds music files to a service" "/home/amarok-dev/amarok/src/servicebrowser/scriptableservice/file_browser_service_script.rb"`

else
    case $*[0].strip()
        when "--populate"
                if $*[1].strip() == "-1"
                            puts " Populating main level..."
			                DirWalker.new.ParseLevel( "/home/amarok-dev/media/jamendo/", -1 )
                            puts "... done"
                        else
                            puts " Populating " + $*[2].to_s() + "..."
                            DirWalker.new.ParseLevel( $*[2].to_s(), $*[1] )
                            puts "... done"
                        end;
	end
end




