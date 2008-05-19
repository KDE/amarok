#!/usr/bin/env ruby
#
# Export application icons from SVG
#
# Copyright (C) 2007 Harald Sitter <sitter.harald@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

@app             = "amarok"
@iconset         = "hi"
@exportpath      = "../../src/images"
bigsvg           = "../../src/images/amarok_icon.svg"
smallsvg         = "../../src/images/amarok_icon_small.svg"
resolutions      = [128,64,48,32]
smallresolutions = [22,16]


def export_icon( res, svg )
    `inkscape --without-gui --export-png="#{@exportpath}/#{@iconset}#{res}-app-#{@app}.png" --export-dpi=72 --export-background-opacity=0 --export-width=#{res} --export-height=#{res} "#{svg}" > /dev/null`
end


for res in resolutions do
    export_icon( res, bigsvg )
end

for res in smallresolutions do
    export_icon( res, smallsvg )
end

exit 0
