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

require 'fileutils'
require './lib/librelease.rb'

# ask for checkout location
# CheckoutLocation()

# Information()

# ask for version number
# ReleaseVersion()

# 
branch   = "trunk"
name     = "amarok"
folder   = "#{name}-#{version}"

user     = "sitter"
protocol = "svn+ssh"
version  = "2.0.0"

repo     = "#{protocol}://#{user}@svn.kde.org/home/kde/#{branch}"

# fetch sources
FetchSource( folder, name, repo )


FetchTranslations( folder, name, repo )


# create the tar archive
CreateTar( folder )
