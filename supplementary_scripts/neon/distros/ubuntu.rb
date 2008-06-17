# Copyright (C) 2008 Harald Sitter <harald@getamarok.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License or (at your option) version 3 or any later version
# accepted by the membership of KDE e.V. (or its successor approved
# by the membership of KDE e.V.), which shall act as a proxy 
# defined in Section 14 of version 3 of the license.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

DEBVERSION   = "hardy"
DEBPATH      = NEONPATH + "/distros/ubuntu"
DEBBASEPATH  = TMPPATH + "/#{DATE}-ubuntu"
GENERICFILES = ["copyright","compat","changelog"]
ROKPACKAGES  = ["qt","kdesupport","kdelibs","kdebase"]
KDEPACKAGES  = ["qt","kdesupport","kdelibs","kdepimlibs","kdebase","kdesdk","kdemultimedia","kdegraphics","kdenetwork"]

ENV['DEBEMAIL']    = "nightly@getamarok.com"
ENV['DEBFULLNAME'] = "Project Neon"

class UploadUbuntu
    def initialize(part)
        @part = part

        if @part == "amarok"
            @packages = ROKPACKAGES
            @lppath   = "http://ppa.launchpad.net/project-neon/ubuntu/pool/main/a/amarok-nightly"
        else
            @packages = KDEPACKAGES
            @lppath   = "http://ppa.launchpad.net/project-neon/ubuntu/pool/main/k/kde-nightly"
        end

        def baseDir()
            Dir.chdir(DEBBASEPATH)
        end

        execute()
    end

    def setVersion(package)
        @rev = SVNPACKAGES[package]
        @debversion  = "#{DATE}+svn#{@rev}-0neon#{REV}"
    end

    def amarokUpload()
        system("dput project-neon ../#{@part}-nightly_#{@debversion}_source.changes")
    end

    def srcUpload(package)
        system("dput project-neon ../#{@part}-nightly-#{package}_#{@debversion}_source.changes")
    end

    def createDebianSrc(package)
        FileUtils.cp_r("#{DEBPATH}/#{package}-debian", "./debian")

        # abuse generic files
        for gfile in GENERICFILES
            unless File.exist?("./debian/#{gfile}")
                FileUtils.cp_r("#{DEBPATH}/#{gfile}", "./debian/#{gfile}")
            end
        end

        system("licensecheck --copyright * | grep -v \"/debian/\" >> ./debian/copyright")

        system("dch -D '#{DEBVERSION}' -v '#{@debversion}' 'Nightly Build'")
        unless package == "amarok"
            file = File.new("./debian/changelog", File::RDWR)
            str = file.read()
            file.rewind()
            file.truncate( 0 )
            str.gsub!(/-nightly /,"-nightly-#{package} ")
            file << str
            file.close()
        end

        if @part == "kde"
            changeToKde()
        end

        system("dpkg-buildpackage -S -sa -k'#{ENV['DEBFULLNAME']}'")
    end

    def checkAvailablilty(package)
        url = "#{@lppath}-#{package}/#{@part}-nightly-#{package}_#{@debversion}_i386.deb"

        %x[wget '#{url}']

        turn = 0
        while $? != 0
            sleep 60
            %x[wget '#{url}']
            if turn > 360
                exit 1 #leave me alone
            end
            turn += 1
        end
    end

    def changeToKde()
        for file in Dir.entries("./debian") - ['.', '..', '.svn']
            # TODO: file.kde should contain a list of gsub/sub rules rather than a compelte copy
            if File.exists?("./debian/#{file}.kde")
                File.mv("./debian/#{file}.kde", "./debian/#{file}")
            else
                cfile = File.new("./debian/" + file, File::RDWR)
                str = cfile.read()
                cfile.rewind()
                cfile.truncate( 0 )
                str.gsub!('amarok-nightly','kde-nightly')
                cfile << str
                cfile.close()
            end
        end
    end

    def execute()
        Dir.chdir(ROOTPATH)
        FileUtils.rm_rf(DEBBASEPATH)
        Dir.mkdir(DEBBASEPATH)
        FileUtils.cp_r("#{BASEPATH}/.", DEBBASEPATH)

        for package in @packages
            if SVNPACKAGES.has_key?(package)
                setVersion(package)
                puts("Ubuntu: uploading #{package} checkout revision #{@rev}")
                dir = "amarok-nightly-#{package}"
                baseDir()
                Dir.chdir(dir)

                createDebianSrc(package)
                srcUpload(package)
                checkAvailablilty(package)
            end
        end

        if @part == "amarok"
            baseDir()
            setVersion("amarok")
            Dir.chdir("amarok-nightly")
            createDebianSrc("amarok")
            amarokUpload()
        end
    end
end
