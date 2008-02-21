#!/usr/bin/env ruby
#
# File publisher module for the Neon framework
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

class PublishFile
  def initialize()

    @conf = Config::read(CONFIG)
    section = "pub-file"

    unless @conf.section?(section)
      CreateConfig(section)
    end

    dir = @conf.value(section, "dir")
    Move(dir)
  end

  def CreateConfig(section)
    #add data section
    @conf.add_section(section)

    #aggregate data
    puts "File - Directory:"
    dir = gets

    #write data to config
    @conf.add_value(section, "dir", dir.chomp())
    #write config to file
    @conf.save(CONFIG)
  end

  def Move(dir)
    dest = "#{dir}/#{DATE}"

    Neon.new.BaseDir()

    FileUtils.mkdir(dest)
    Dir.foreach("."){|file|
      if file.include?(".tar.bz2")
        FileUtils.mv(file, dest)
      end
    }
  end

end
