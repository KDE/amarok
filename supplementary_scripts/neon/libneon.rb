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

DATE           = Time.now.utc.strftime("%Y%m%d")
DAYOFMONTH     = Time.now.utc.mday()
DAYOFWEEK      = Time.now.utc.wday()
REV            = "1"
NEONPATH       = Dir.pwd()
ROOTPATH       = "#{NEONPATH}/root"
TMPPATH        = "#{ROOTPATH}/tmp"
BASEPATH       = "#{TMPPATH}/#{DATE}"
SVNPATH        = "#{ROOTPATH}/svn"
PATHS          = [ROOTPATH,TMPPATH,BASEPATH,SVNPATH]
AMAROKVERSION  = "2.0-SVN-Neon"
CONFIG         = "#{ENV['HOME']}/.neonrc"

require 'fileutils'
require 'fetcher.rb'
require 'publisher.rb'
require 'distro.rb'
require 'config.rb'

module Neon
    class Neon
        def initialize()
            # create today's basepath
            for dir in PATHS
                initDir(dir)
            end

            # create the configuration file if it doesn't exist
            unless File.exist?(CONFIG)
                FileUtils.touch(CONFIG)
                FileUtils.chmod(0600, CONFIG)
            end
        end
    end

    def initDir(dir)
        unless File.exist?(dir)
            FileUtils.mkdir_p(dir)
        end
    end

    def thisMethod()
        caller[0][/`([^']*)'/, 1]
    end

    def baseDir()
        Dir.chdir(BASEPATH)
    end

    def svnDir()
        Dir.chdir(SVNPATH)
    end

    def mrClean()
        puts("Mr. Clean is not yet completely(?) implemented, sorry :-(")
        FileUtils.rm_rf(Dir.glob(BASEPATH))
    end
end
