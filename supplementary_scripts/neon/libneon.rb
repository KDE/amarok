#!/usr/bin/env ruby
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

require 'config.rb'

  class Neon
  def initialize()
    #create today's basepath
    unless File.exist?(BASEPATH)
      FileUtils.mkdir_p(BASEPATH)
    end

    #create the configuration file if it doesn't exist
    unless File.exist?(CONFIG)
      FileUtils.touch(CONFIG)
      FileUtils.chmod(0600, CONFIG)
    end
  end

  def BaseDir()
    Dir.chdir(BASEPATH)
  end

  def CreateTar(dir)
    puts Dir.pwd()
    `find '#{dir}' -name '.svn' | xargs rm -rf`
    `tar -cf #{dir}.tar #{dir}`
    `bzip2 #{dir}.tar`
  end
end
