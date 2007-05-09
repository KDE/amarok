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
 # Small tool for reverting all instances of Amarok::icon("foo") back 
 # to the original oxygen icon names.
 #
 ###########################################################################


file = File.new("src/iconloader.cpp", File::RDONLY)
str = file.read 
name_table = Hash.new

str.each_line do |line|
  if line.include?('iconMap["')
    reg = /(".*?")(.*)(".*?")/
    a = reg.match(line)[1]
    b = reg.match(line)[3]
    puts "#{a}  #{b}" 
  end
end

