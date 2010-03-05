#!/usr/bin/ruby
###########################################################################
#   This script puts a DEBUG_BLOCK in every method of a given C++ file.   #
#   Use with caution.                                                     #
#                                                                         #
#   Copyright                                                             #
#   (C) 2008 Casey Link <unnamedrambler@gmail.com>                        #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         #
###########################################################################

if ARGV.size < 1
  print "USAGE: #{__FILE__} <filename> \n"
  exit
end

regex = Regexp.new(/^.+::(.+)\(.*\)\s*(?:const)*\s*(?::\s*(?:\s*.+\(.*\))*)*\s*\{\s*(?:DEBUG_BLOCK)*/)
f = File.new(ARGV[0])
string = f.read
news = string.gsub(regex){ |s|
  if not s.include? "DEBUG_BLOCK"
    "#{$&}\n\tDEBUG_BLOCK\n"
  else
    $&
  end
}
print news

