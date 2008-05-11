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

require 'libosc.rb'

RPMPATH  = NEONPATH + "/distros/osbs"
RPMPATH  = ROOTPATH + "/#{DATE}-osbs"
PACKAGES = ["qt","strigi","taglib","kdelibs","kdebase-runtime"]

class UploadOSBS
  def initialize()
    def BaseDir()
      Dir.chdir(RPMPATH)
    end

    Execute()
  end

  def Execute()
  end
end
