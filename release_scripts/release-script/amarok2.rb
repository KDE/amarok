#!/usr/bin/env ruby
#
# Generates a release tarball from KDE SVN
#
# Copyright (C) 2007-2009 Harald Sitter <apachelogger@ubuntu.com>
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

NAME      = "amarok"
COMPONENT = "extragear"
SECTION   = "multimedia"

# Amarok-only changes
# * Change application version to releaseversion
# * Remove unnecessary files
def custom
    # Change version
    src_dir
    file = File.new( "Version.h", File::RDWR )
    str = file.read
    file.rewind
    file.truncate( 0 )
    str.sub!( /APP_VERSION \".*\"/, "APP_VERSION \"#{@version}\"" )
    file << str
    file.close

    remover([
        "Amarok.kdev4","release_scripts","supplementary_scripts","HACKING",
        "VIS_PLAN",".krazy","src/.gitignore", "docs", ".gitignore"
    ])
    base_dir

# reference implementation of multi-tarballing
#    create_tar("mac",true) # second create_tar gets called by the starter helper
#    remover(["src/mac"])
end

$options = {:barrier=>70, :docs=>false, :tag=>true, :user=>"lydia", :branch=>"trunk", :protocol=>"svn+ssh"}

# get things started
require 'lib/starter'
