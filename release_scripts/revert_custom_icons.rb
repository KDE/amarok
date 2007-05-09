 ###########################################################################
 #   Copyright (C) 2007 by Mark Kretschmann <markey@web.de>                #
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
 #   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          #
 ###########################################################################
 #
 #   Small tool for reverting all instances of Amarok::icon("foo") back 
 #   to the original oxygen icon names.
 #
 ###########################################################################

require 'strscan'

def search_cpp(folder)
  Dir.foreach(folder) do |x|
    next if x[0, 1] == "."
    search_cpp("#{folder}/#{x}") if FileTest.directory?("#{folder}/#{x}")
    @cpp_files += Dir["#{folder}/#{x}/*.cpp"]
  end
end

def fix_file(path)
  file = File.new(path, File::RDWR)
  str = file.read
  str_output = str.dup
  scanner = StringScanner.new(str)

  loop do
    scanner.scan(/(.*?)(Amarok::icon\( *?)(".*?")( *?\))/m)
    break if scanner[3].nil? 
    name = scanner[3]
    whole_match = scanner[2] + scanner[3] + scanner[4]
    new_name = @name_table[name]
    new_name = name if new_name == nil
    str_output.sub!(whole_match, new_name)
  end
  file.rewind
  file.truncate(0)
  file << str_output
end

# Make sure the current working directory is amarok
unless Dir::getwd().split( "/" ).last() == "amarok"
    print "ERROR: This script must be started from the amarok/ folder. Aborting.\n\n"
    exit(1)
end

file = File.new("src/iconloader.cpp", File::RDONLY)
str = file.read 
@name_table = Hash.new

str.each_line do |line|
  if line.include?('iconMap["')
    reg = /(".*?")(.*)(".*?")/
    a = reg.match(line)[1]
    b = reg.match(line)[3]
    @name_table[a] = b
  end
end

@cpp_files = []
search_cpp("src")

@cpp_files.each do |x|
  puts("Processing " + x)
  fix_file(x)
end

puts
puts("Done.")

