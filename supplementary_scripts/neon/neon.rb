#!/usr/bin/env ruby
#
# Neon Framework - Amarok Nightly Builds
#
# Copyright (C) 2008 Harald Sitter <harald@getamarok.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

DATE        = Time.now.utc.strftime("%Y%m%d")
DAYOFMONTH  = Time.now.utc.mday()
DAYOFWEEK   = Time.now.utc.wday()
REV         = "1"
NEONPATH    = Dir.pwd()
ROOTPATH    = "#{ENV['HOME']}/amarok-nightly"
BASEPATH    = "#{ROOTPATH}/#{DATE}"
APPVERSION  = "2.0-SVN-Neon"
CONFIG      = "#{ENV['HOME']}/.neonrc"

require 'fileutils'
require 'libneon.rb'
require 'fetcher.rb'
require 'publisher.rb'
require 'distro.rb'

#make main module accessible
include Neon
#make main class accessible
@neon = Neon::Neon.new()
#make configurations accessible
@conf = Config::read(CONFIG)

###############################
# Fetch Source

if DAYOFMONTH == 1 or $*[0] == "all" or $*[0] == "qt"
  QtCopy()
end

if DAYOFWEEK == 0 or $*[0] == "all" or $*[0] == "deps"
  Strigi()
  TagLib()
  KdeLibs()
  KdeBaseRuntime()
end

Amarok()

##############################
# Publish

# PublishFtp.new()
#
# PublishFile.new()

##############################
# Distribution Uploads

SVNPACKAGES = @packages

UploadUbuntu.new()

###############################
# Cleaning man

puts "Mr. Clean is not yet completely(?) implemented, sorry :-("

FileUtils.rm_r(ROOTPATH)
