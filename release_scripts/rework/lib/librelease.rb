#!/usr/bin/env ruby
#
# Generic ruby library for KDE extragear/playground releases
#
# Copyright (C) 2007-2008 Harald Sitter <harald@getamarok.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

require 'fileutils'

# DOC TODO
def FetchSource()
  bar  = @dlg.progressbar("fetching source code",1)
  FileUtils.rm_rf( @folder )
  FileUtils.rm_rf( "#{@folder}.tar.bz2" )

  branch = "trunk"

  if @useStable
    branch = "branches/stable"
  elsif @tag and not @tag.empty?()
    branch = "tags/#{NAME}/#{@tag}"
  end

  @repo = "#{@protocol}://#{@user}.kde.org/home/kde/#{branch}"
#   @repo = "file:///home/kde/#{branch}"
  puts @repo #DEBUG

  puts "Fetching source from #{branch}...\n\n"
  # TODO: ruby-svn
  `svn co #{@repo}/#{COMPONENT}/#{SECTION}/#{NAME} #{@folder}`

  bar.progress = 1
  bar.close
end

# Removes all .svn directories, creates a tar.bz2 and removes the source folder.
# You probably want to run this command as one of the last actions, since for
# example tagging heavily depends on the presence of the .svn directories.
def CreateTar()
  bar  = @dlg.progressbar("creating tarball",4)
  `find ".svn" -name #{@folder} | xargs rm -rf`
  bar.progress = 1
  `tar -cf #{@folder}.tar #{@folder}`
  bar.progress = 2
  `bzip2 #{@folder}.tar`
  bar.progress = 3
  FileUtils.rm_rf(@folder)
  bar.close
end

# TODO
def CreateCheckSums()
  puts "MD5Sum: #{`md5sum #{@folder}.tar.bz2`}"
  puts "SHA1Sum: #{`sha1sum #{@folder}.tar.bz2`}"
end

# TODO
def CreateMailNotification()
end

# TODO
def CreateMailAnnouncement()
end

# TODO
def CreateChangeLogHtml()
end
