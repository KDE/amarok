#!/usr/bin/env ruby
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

# advanced file/dir manipulation
require 'fileutils'

# constant - unchangeable once it got a value
SVNPATH = "tags/qca/2.0.0"

# create temporary directories
if File.exists?("tmp")
    FileUtils.rm_r("tmp")
end
Dir.mkdir("tmp")
Dir.chdir("tmp")

# svn checkout
system("svn co svn://anonsvn.kde.org/home/kde/#{SVNPATH} qca")

# copy debian dir in
FileUtils.cp_r("../debian", "qca/")

Dir.chdir("qca")

# edit changelog and copy it back to the base debian dir
system("dch -i -D hardy")
FileUtils.cp("debian/changelog", "../../debian/changelog")

# build package and upload to ppa
system("dpkg-buildpackage -S -sa")
system("dput ppa ../*changes")

# go away
exit 0
