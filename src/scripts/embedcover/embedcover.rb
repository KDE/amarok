#!/usr/bin/env ruby
#
# Amarok Script for embedding album cover images in MP3 files.
#
# (c) 2005-2006 Mark Kretschmann <markey@web.de>
# License: GNU General Public License V2


require 'md5'
require "uri"

$stdout.sync = true
MenuItemName = "EmbedCover DoIt!"


def cleanup()
  `dcop amarok script removeCustomMenuItem #{MenuItemName}`
end

def sql_escape( string )
  return string.gsub( /[']/, "''" )
end


trap( "SIGTERM" ) { cleanup() }

`dcop amarok script addCustomMenuItem #{MenuItemName}`


loop do
  message = gets().chomp()
  command = /[A-Za-z]*/.match( message ).to_s()

  case command
  when "configure"
    msg  = 'EmbedCover does not have configuration options. Simply select a track in the '
    msg += 'playlist, then start EmbedCover from the context-menu (right mouse click).'

    `dcop amarok playlist popupMessage "#{msg}"`

  when "customMenuClicked"
    if message.include?( MenuItemName )
      args = message.split()
      # Remove the command args
      3.times() { args.delete_at( 0 ) }

      # Iterate over all selected files
      args.each() do |arg|
        uri = URI.parse( arg )
        file = URI.unescape( uri.path() )

        puts( "Path: #{file}" )

        backend = File.dirname( File.expand_path( __FILE__ ) ) + "/addimage2mp3.rb"

        # In the database we store relative URLs (for dynamic collection), so we need to convert
        file_relative = `dcop amarok collection relativePath "#{file}"`.chomp

        # Query is two parts, first ID, then name
        artist_id = `dcop amarok collection query "SELECT DISTINCT artist FROM tags WHERE url = '#{sql_escape( file_relative )}'"`.chomp
        artist    = `dcop amarok collection query "SELECT DISTINCT artist.name FROM artist WHERE id = '#{artist_id}'"`.chomp

        album_id  = `dcop amarok collection query "SELECT DISTINCT album FROM tags WHERE url = '#{sql_escape( file_relative )}'"`.chomp
        album     = `dcop amarok collection query "SELECT DISTINCT album.name FROM album WHERE id = '#{album_id}'"`.chomp

        puts( "ArtistId : #{artist_id}" )
        puts( "Artist   : #{artist}" )
        puts( "AlbumId  : #{album_id}" )
        puts( "Album    : #{album}" )

        if artist_id.empty?() and album_id.empty?()
          `dcop amarok playlist popupMessage "EmbedCover Error: This track is not in your Collection."`
          next
        end

        md5sum = MD5.hexdigest( "#{artist.downcase()}#{album.downcase()}" )
        imagefolder = "#{ENV['HOME']}/.kde/share/apps/amarok/albumcovers/large/"
        image = "#{imagefolder}#{md5sum}"

        puts( "Imagepath: #{image}" )

        unless FileTest.exist?( image )
          # If there is no imported image, check if there is an image associated
          # in the music folder

          sql = "SELECT path FROM images WHERE artist LIKE #{sql_escape( artist )} AND album LIKE #{sql_escape( album )} ORDER BY path;"
          images = `dcop amarok collection query #{sql}`.split( "\n" )

          # FIXME select best image from array, like CollectionDB does
          image = images.first()

          if image == nil or not FileTest.exist?( image )
            `dcop amarok playlist popupMessage "EmbedCover: No image found for this track."`
            next
          end
        end

        output = `ruby #{backend} "#{image}" "#{file}"`

        if $?.success?()
          `dcop amarok playlist popupMessage "EmbedCover has successfully embedded the image."`
        else
          reg = Regexp.new( "Error:.*", Regexp::MULTILINE )
          errormsg = reg.match( output )

          `dcop amarok playlist popupMessage "EmbedCover #{errormsg}"`
        end
      end
    end
  end
end
