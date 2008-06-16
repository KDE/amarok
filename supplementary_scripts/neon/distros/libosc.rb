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

OSCPATH    = ROOTPATH + "/osc"
OBSPROJECT = "home:apachelogger"

module Osc
    def oscStarter()
        unless File.exist?(OSCPATH)
            system("osc co #{OBSPROJECT}")
            FileUtils.mv(OBSPROJECT, OSCPATH)
        else
            oscUpdate()
        end
    end

    def oscDir()
        Dir.chdir(OSCPATH)
    end

    def oscUpdateEval()
        oscDir()

        count = 0
        %x[osc up]
        while $? != 0 # TODO: libfetch.rb -> combine in libneon.rb
            unless count >= 20
                puts("retrying...")
                %x[#{cmd}]
                count += 1
            else
                puts("Error: osc checkout didn't exit properly in 20 tries, aborting.")
                # TODO: currently avoids mr. clean!
                exit 0
            end
        end
    end

    def oscUpdate()
        puts("#{thisMethod()}() started")
        system("osc addremove *")
        oscUpdateEval()
    end

    def oscCommit()
        oscDir()
        puts "#{thisMethod()}() started in #{@dir}"
        Dir.chdir(@dir)
        system("osc addremove")
        system("osc ci -m 'update'")
        @oscrev = %x[osc info].split("\n")[3].split(" ")[1]
    end
end
