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

DEBPATH     = NEONPATH + "/distros/ubuntu"
DEBBASEPATH = ROOTPATH + "/#{DATE}-ubuntu"
LPPATH      = "http://ppa.launchpad.net/amarok-nightly/ubuntu/pool/main/a/amarok-nightly"
PACKAGES    = ["qt","strigi","kdelibs","kdebase-runtime","amarok"]

class UploadUbuntu
  def initialize()
    def BaseDir()
      Dir.chdir(DEBBASEPATH)
    end

    def CreateNUpload(package)
      `cp -rf #{DEBPATH}/#{package}-debian ./debian`
    
      `dch -b -v "#{DATE}-0amarok#{REV}" "Nightly Build"`
      `dpkg-buildpackage -S -sa -rfakeroot -k"Amarok Nightly Builds"`
      `dput amarok-nightly ../amarok-nightly-#{package}_#{DATE}-0amarok#{REV}_source.changes`

      `cp debian/changelog #{DEBPATH}/#{package}-debian/`
    end
    
    def Slap(package)
      url = "#{LPPATH}-#{package}/amarok-nightly-#{package}_#{DATE}-0ubuntu0amarok#{REV}_i386.deb`"
      `wget '#{url}'`
	while $? != "0"
	  sleep 600
	  `wget '#{url}'`
	end
	sleep 600
    end

    Dir.chdir(ROOTPATH)
#     Dir.mkdir(DEBBASEPATH)
#     FileUtils.cp_r("#{BASEPATH}/.", DEBBASEPATH)

    for package in PACKAGES
      if package != "amarok"
	dir = "amarok-nightly-#{package}-#{DATE}"
      else
	dir = "amarok-nightly-#{DATE}"
      end
      
      BaseDir()
      Dir.chdir(dir)
    
      CreateNUpload(package)
      Slap(package)
    end
  end
end
