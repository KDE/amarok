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

require 'distros/libosc.rb'

include Osc

OBSPATH  = NEONPATH + "/distros/obs"
PACKAGES = ["qt","kdesupport","kdelibs","kdebase","amarok"]
REPOPATH = "http://download.opensuse.org/repositories/home:/apachelogger/openSUSE_10.3/i586"

class UploadObs
    def initialize()
        oscStarter()
        puts("OBS: initiated.")
        execute()
    end

    def setVersion(package)
        @rev         = SVNPACKAGES[package]
        @rpmversion  = "#{DATE}.#{@rev}"
    end

    def copyFiles(package)
        if package == "amarok"
            @dir = "amarok-nightly"
        else
            @dir = "amarok-nightly-#{package}"
        end

        puts("OBS: Copying tar.bz2")
        oscDir()
        Dir.chdir(@dir)
        bz2 = "#{@dir}.tar.bz2"
        FileUtils.cp_r(BASEPATH + "/" + bz2, "./")

        puts("OBS: Copying spec")
        FileUtils.cp_r("#{OBSPATH}/#{@dir}.spec", "./")
    end

    def applyVersion(package)
        puts("OBS: Applying SVN revision #{@rev} as version")
        file = File.new("./#{@dir}.spec", File::RDWR)
        str = file.read()
        file.rewind()
        file.truncate( 0 )
        str.gsub!("0x1344224",@rpmversion)
        file << str
        file.close()
    end

    def checkAvailablilty(package)
        url = "#{REPOPATH}/amarok-nightly-#{package}-#{@rpmversion}-1.1.i586.rpm"

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

    def execute()
        Dir.chdir(ROOTPATH)

        for package in PACKAGES
            if SVNPACKAGES.has_key?(package)
                setVersion(package)

                copyFiles(package)

                applyVersion(package)

                puts("OBS: uploading #{package} checkout revision #{@rev}")
                oscCommit()

                unless package == "amarok"
                    checkAvailablilty(package)
                end
            end
        end
    end
end
