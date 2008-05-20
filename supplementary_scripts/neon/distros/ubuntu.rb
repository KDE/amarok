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

DEBVERSION  = "hardy"
DEBPATH     = NEONPATH + "/distros/ubuntu"
DEBBASEPATH = ROOTPATH + "/#{DATE}-ubuntu"
LPPATH      = "http://ppa.launchpad.net/project-neon/ubuntu/pool/main/a/amarok-nightly"
PACKAGES    = ["qt","strigi","taglib","kdelibs","kdebase-runtime"]

debemail    = ENV['DEBEMAIL']
debfullname = ENV['DEBFULLNAME']

ENV['DEBEMAIL']    = "nightly@getamarok.com"
ENV['DEBFULLNAME'] = "Project Neon"

class UploadUbuntu
  def initialize()
    def BaseDir()
      Dir.chdir(DEBBASEPATH)
    end

    Execute()
  end

  def SetVersion(package)
    @rev = SVNPACKAGES[package]
    @debversion  = "#{DATE}+svn#{@rev}-0amarok#{REV}"
  end

  def AmarokUpload()
    # upload
    `dput amarok-nightly ../amarok-nightly_#{@debversion}_source.changes`
  end

  def SrcUpload(package)
    # upload
    `dput amarok-nightly ../amarok-nightly-#{package}_#{@debversion}_source.changes`
  end

  def CreateNUpload(package)
     FileUtils.cp_r("#{DEBPATH}/#{package}-debian", "./debian")

     # automatic copyright/license listing
     FileUtils.cp_r("#{DEBPATH}/copyright", "./debian/copyright")
    `grep -i -R copyright * | grep -v "/debian/" >> ./debian/copyright`
    `licensecheck * | grep -v "/debian/" >> ./debian/copyright`

    `dch -D "#{DEBVERSION}" -v "#{@debversion}" "Nightly Build"`
    `dpkg-buildpackage -S -sa -k"#{ENV['DEBFULLNAME']}"`

    if package == "amarok"
      AmarokUpload()
    else
      SrcUpload(package)
    end
  end

  def CheckAvailablilty(package)
    url = "#{LPPATH}-#{package}/amarok-nightly-#{package}_#{@debversion}_i386.deb"

    `wget '#{url}'`

    turn = 0
    while $? != 0
      sleep 60
      `wget '#{url}'`
       if turn > 360
         exit 1 #leave me alone
       end
       turn += 1
    end
  end

  def Execute()
    Dir.chdir(ROOTPATH)
    Dir.mkdir(DEBBASEPATH)
    FileUtils.cp_r("#{BASEPATH}/.", DEBBASEPATH)

    for package in PACKAGES
      if SVNPACKAGES.has_key?(package)
        SetVersion(package)
        puts "Ubuntu: uploading #{package} checkout revision #{@rev}"
        dir = "amarok-nightly-#{package}-#{@rev}"
        BaseDir()
        Dir.chdir(dir)

        CreateNUpload(package)
        CheckAvailablilty(package)
      end
    end

    BaseDir()
    SetVersion("amarok")
    Dir.chdir("amarok-nightly-#{@rev}")
    CreateNUpload("amarok")

  end

rescue
  ENV['DEBEMAIL']    = debemail
  ENV['DEBFULLNAME'] = debfullname

end
