#!/usr/bin/env ruby
#
# Move icons from subdirectories to one and rename them accordingly
# Assuming Directory structure:
#  ./16x16/
#  ./16x16/actions/
#  ./16x16/actions/amarok-icon-blue.png
#  ./16x16/apps/
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

require 'fileutils'

output = "./icons/"
sizes  = ["128","48","32","22","16"]

unless File.exists?(output)
    FileUtils.mkdir_p(output)
end

for size in sizes
    path = "./" + size + "x" + size + "/"
    for dir in Dir.entries(path) - ['.', '..']
        path += dir + "/"
        for icon in Dir.entries(path) - ['.', '..']
            unless icon.include?("hi" + size + "-" + dir)
                iconnew = "hi" + size + "-" + dir + "-" + icon
                puts("COPYRENAME: cp #{path + icon} TO #{output + iconnew}")
                FileUtils.cp(path + icon, output + iconnew)
                icon = iconnew
            else
                puts("COPY: cp #{path + icon} TO #{output}")
                FileUtils.cp(path + icon, output)
            end
        end
    end
end
