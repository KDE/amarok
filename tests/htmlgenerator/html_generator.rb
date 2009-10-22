#!/usr/bin/env ruby
########################################################################################
# HTML generator for Amarok's unit tests.                                              #
# This program transforms test output from XML to HTML, using a XSL stylesheet.        #
#                                                                                      #
# Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            #
#                                                                                      #
# This program is free software; you can redistribute it and/or modify it under        #
# the terms of the GNU General Public License as published by the Free Software        #
# Foundation; either version 2 of the License, or (at your option) any later           #
# version.                                                                             #
#                                                                                      #
# This program is distributed in the hope that it will be useful, but WITHOUT ANY      #
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      #
# PARTICULAR PURPOSE. See the GNU General Public License for more details.             #
#                                                                                      #
# You should have received a copy of the GNU General Public License along with         #
# this program.  If not, see <http://www.gnu.org/licenses/>.                           #
########################################################################################

require 'xml/libxml'
require 'xml/libxslt'


XSL_FILE = 'AmarokTest.xsl'
OUTPUT_FILE = 'Amarok.html'

out = ""

files = Dir["**/*.xml"].sort

# Generate menu
out += "<h1>Unit Tests</h1><h3><ul>"
files.each do |path|
  out += "<li><a href='##{path}'>#{path.sub('.xml','')}</a></li>"
end

out += "</ul></h3><br/><br/>"

files.each do |path|
  begin
    xslt = XML::XSLT.new
    xslt.xsl = XSL_FILE
    xslt.xml = path 
    out += "<a name='#{path}'>"
    out += xslt.serve
    out += "<br/><br/><hr noshade><br/><br/>"
  rescue
    print "exception"
  end
end


# Save to file
File.new( OUTPUT_FILE, File::CREAT | File::RDWR | File::TRUNC ).write(out)


print out

