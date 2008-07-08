#!/usr/bin/env ruby
#
# Neon Framework - Amarok Nightly Builds
#
# Copyright (C) 2008 Harald Sitter <harald@getamarok.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License or (at your option) version 3 or any later version
# accepted by the membership of KDE e.V. (or its successor approved
# by the membership of KDE e.V.), which shall act as a proxy 
# defined in Section 14 of version 3 of the license.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

require 'libneon.rb'

#make main module accessible
include Neon
#make main class accessible
@neon = Neon::Neon.new()
#make configurations accessible
@conf = Config::read(CONFIG)

# TODO: needs new position
#mrClean()

###############################
# Fetch Source

if $*[0] == "all" or $*[0] == "qt"
    fetcQtCopy()
end

if DAYOFWEEK == 0 or $*[0] == "all" or $*[0] == "deps"
    fetchKdeSupportMinimal()
    fetchKdeLibs()
    fetchKdeBaseMinimal()
end

fetchAmarok()

##############################
# Publish

# PublishFtp.new()
#
# PublishFile.new()

##############################
# Distribution Uploads

SVNPACKAGES = @packages

UploadUbuntu.new("amarok")
UploadObs.new()


###############################
# Cleaning man

mrClean()
