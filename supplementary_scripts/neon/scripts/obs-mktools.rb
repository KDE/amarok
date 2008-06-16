#!/usr/bin/env ruby
#
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

require 'fileutils'

OBSPROJECT = "home:apachelogger"

tmpdir   = "tmp-tools-obs"
specfile = "distros/obs/tools.spec"
thing    = "tools"
oscthing = "amarok-nightly-#{thing}"
thingdir = tmpdir + "/" + oscthing

Dir.chdir("../")

unless File.exists?(tmpdir)
    system("osc co #{OBSPROJECT} amarok-nightly-tools")
    FileUtils.mv(OBSPROJECT, tmpdir)
end

FileUtils.cp_r(thing, thingdir)
FileUtils.cp_r(specfile, thingdir + "/amarok-nightly-#{thing}.spec")

Dir.chdir(thingdir)
FileUtils.mv(thing, oscthing)
system("tar -cf #{oscthing}.tar #{oscthing}")
if File.exists?("#{oscthing}.tar.bz2")
    FileUtils.rm_r("#{oscthing}.tar.bz2")
end
system("bzip2 #{oscthing}.tar")
FileUtils.rm_r(oscthing)

system("osc addremove")
system("osc ci -m 'update'")
