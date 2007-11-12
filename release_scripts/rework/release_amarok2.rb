#!/usr/bin/env ruby
#
# Generates an Amarok release tarball from KDE SVN
#
# Copyright (C) 2007 Harald Sitter <harald@getamarok.com>
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

NAME      = "amarok"
COMPONENT = "extragear"
SECTION   = "multimedia"

require 'fileutils'
require './lib/librelease.rb'

def Amarok()
  # Change version
  Dir.chdir("src")
  file = File.new( "#{NAME}.h", File::RDWR )
  str = file.read()
  file.rewind()
  file.truncate( 0 )
  str.sub!( /APP_version \".*\"/, "APP_version \"#{@version}\"" )
  file << str
  file.close()
  Dir.chdir("..") #amarok

  # Remove crap
  toberemoved = ["release_scripts","supplementary_scripts"]
  for object in toberemoved
    FileUtils.rm_rf(object)
  end

  Dir.chdir("..") #exec path
end

InformationQuery()

@folder = "#{NAME}-#{@version}" #create folder constant

FetchSource()

FetchTranslations()

Amarok()

CreateTar()
