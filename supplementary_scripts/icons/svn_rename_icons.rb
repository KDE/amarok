#!/usr/bin/env ruby
#
# Rename Amarok's icons in src/images/icons
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

sizes = [128,64,48,32,22,16]

Dir.chdir("../../src/images/icons")

if ARGV[1].split(-).reverse[0] == amarok
    attach = ""
else
    attach = "-amarok"
end

for size in sizes do
    system("svn", "mv", "hi#{size}-action-#{ARGV[0]}.png", "hi#{size}-action-#{ARGV[1]}#{attach}.png")
    if $? != 0
        system("mv", "hi#{size}-action-#{ARGV[0]}.png", "hi#{size}-action-#{ARGV[1]}-amarok.png")
    end
end

exit 0
