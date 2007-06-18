#!/usr/bin/env ruby
#
# Amarok-Script for fixing VBR encoded mp3 files without XING header. Calculates the real
# track length and adds the XING header to the file. This script is a frontend for the
# mp3fix.rb tool.
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# License: GNU General Public License V2


require "uri"


MenuItemName = "MP3Fixer FixIt!"


def cleanup()
    system("dcop", "amarok", "script", "removeCustomMenuItem", MenuItemName)
    # use unlink ??!
    system("rm",  Dir.getwd() + "/mp3fixer_playlist.m3u")
end

class String
    def shellquote
        return "'" + self.gsub("'", "'\\\\''") + "'"
    end
end

trap( "SIGTERM" ) { cleanup() }

system("dcop", "amarok", "script", "addCustomMenuItem", MenuItemName)

loop do
    message = gets().chomp()
    command = /[A-Za-z]*/.match( message ).to_s()

    case command
        when "configure"
            msg  = 'Mp3Fixer does not have configuration options. Simply select a track in the '
            msg += 'playlist, then start Mp3Fixer from the context-menu (right mouse click).'

            system("dcop", "amarok", "playlist", "popupMessage", msg)

        when "customMenuClicked"
            if message.include?( MenuItemName )
                args = message.split()
                folders = []

                # Remove the command args
                3.times() { args.delete_at( 0 ) }

                # Iterate over all selected files
                args.each() do |arg|
                    uri = URI.parse( arg )
                    path = URI.unescape( uri.path() )
                    filename = path.split( "/" ).last()

                    puts( "Path: #{path}" )

                    mp3fix = File.dirname( File.expand_path( __FILE__ ) ) + "/mp3fix.rb"

                    system("dcop", "amarok", "playlist", "shortStatusMessage", "Mp3Fixer is analyzing the file '#{filename}'...")
                    output = `env ruby #{mp3fix.shellquote} #{path.shellquote}`

                    if $?.success?()
                        reg = Regexp.new( "MP3FIX REPAIR SUMMARY:.*", Regexp::MULTILINE )
                        report = reg.match( output ).to_s()
                        report.gsub!( "\n", "<BR/>" )
                        system("dcop", "amarok", "playlist", "popupMessage", report)

                        folders << File.dirname( path ) unless folders.include?( File.dirname( path ) )
                    else
                        reg = Regexp.new( "Error:.*", Regexp::MULTILINE )
                        errormsg = reg.match( output )

                        system("dcop", "amarok", "playlist", "popupMessage", "Mp3Fixer #{errormsg}")
                    end
                end

                # Touch all folders of the modified files, so that the scanner picks then up
                folders.each do |folder|
                    system("touch", folder)
                end
                system("dcop", "amarok", "collection", "scanCollectionChanges")

                # Refresh the playlist
                system("dcop", "amarok", "playlist", "saveM3u", Dir.getwd() + "/mp3fixer_playlist.m3u", "false")
                system("dcop", "amarok", "playlist", "clearPlaylist")
                system("dcop", "amarok", "playlist", "addMedia", Dir.getwd() + "/mp3fixer_playlist.m3u")
            end
    end
end

