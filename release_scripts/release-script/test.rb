#!/usr/bin/env ruby
#
# Copyright (C) 2009 Harald Sitter <apachelogger@ubuntu.com>
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

ALL  = "-b trunk -v 0.1 -p anonsvn -t"
OPTS = ["-l","-s","-d","-a","-r"]

for app in Dir.glob("release_*.rb")
    for opt in OPTS
        for enhancer in OPTS
            opt += " #{enhancer}" unless opt == enhancer
            puts 
            std = %x[ruby #{app} #{ALL} #{opt} > /dev/stdout 2>&1]
            if $? != 0
                puts "ERRRORRRR: #{app} #{ALL} #{opt}"
                puts "\n\n\n"
                puts std
            end
        end
    end
end
