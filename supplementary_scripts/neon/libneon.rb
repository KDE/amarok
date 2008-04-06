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

  module Neon
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
  end

  def ThisMethod()
    caller[0][/`([^']*)'/, 1]
  end

  def BaseDir()
    Dir.chdir(BASEPATH)
  end

  def GetTarball(comp)
    puts "#{ThisMethod()} started with component: #{comp}"
    BaseDir()
    ftp = Net::FTP.new('ftp.kde.org')
    ftp.login
    files = ftp.chdir('pub/kde/snapshots')
    files = ftp.list(comp + "*.tar.bz2")
    file  = files[0].to_s.split(' ')[8]
    ftp.getbinaryfile(comp + ".tar.bz2", file, 1024)
    ftp.close
    rev = file.chomp(".tar.bz2").reverse.chomp("-" + comp.reverse).reverse
    if comp == "qt-copy"
      comp = "qt"
    end
    @dir = "amarok-nightly-" + comp + "-" + rev
    `tar -xf #{file}`
    FileUtils.rm_f(file)
    FileUtils.mv(file.chomp(".tar.bz2"), @dir)
    VarMagic(comp, rev)
  end

  def CheckOut(comp, path, dir=nil, recursive=nil)
    puts "#{ThisMethod()} started with component: #{comp}"
    BaseDir()
    unless recursive == "no"
      cmd = "svn co"
    else
      cmd = "svn co -N"
    end
    unless dir
      dir = "amarok-nightly-" + comp
    end
    `#{cmd} svn://anonsvn.kde.org/home/kde/trunk/#{path} #{dir}`
    count = 0
    while $? != 0
      unless count >= 20
        `svn co svn://anonsvn.kde.org/home/kde/trunk/#{path} #{dir}`
        count += 1
      else
        puts "Neon::CheckOut svn co didn't exit properly in 20 tries, aborting checkout of #{comp}."
        return
      end
    end
    rev = `svn info #{dir}`.split("\n")[4].split(" ")[1]
    @dir = dir + "-" + rev
    FileUtils.mv(dir, @dir)
    VarMagic(comp, rev)
  end

  def VarMagic(comp, rev)
    if @packages.nil?
      @packages = {comp => rev}
    else
      @packages[comp] = [rev]
    end
  end

  def CreateTar(comp, dir=nil)
    puts "#{ThisMethod()} started with component: #{comp}"
    `find '#{@dir}' -name '.svn' | xargs rm -rf`
    `tar -cf #{@dir}.tar #{@dir}`
    `bzip2 #{@dir}.tar`
  end

end


