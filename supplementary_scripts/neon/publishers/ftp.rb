#!/usr/bin/env ruby
#
# FTP publisher module for the Neon framework
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

require 'net/ftp'

class PublishFtp
    def initialize()

        @conf = Config::read(CONFIG)
        section = "pub-ftp"

        unless @conf.section?(section)
            CreateConfig(section)
        end

        host   = @conf.value(section, "host")
        dir    = @conf.value(section, "dir")
        user   = @conf.value(section, "user")
        passwd = @conf.value(section, "passwd")
        Upload(host, dir, user, passwd)
    end

    def CreateConfig(section)
        #add data section
        @conf.add_section(section)

        #aggregate data
        puts "FTP - Host:"
        host = gets
        puts "FTP - Directory:"
        dir = gets
        puts "FTP - User:"
        user = gets
        puts "FTP - Password:"
        passwd = gets

        #write data to config
        @conf.add_value(section, "host", host.chomp())
        @conf.add_value(section, "dir", dir.chomp())
        @conf.add_value(section, "user", user.chomp())
        @conf.add_value(section, "passwd", passwd.chomp())
        #write config to file
        @conf.save(CONFIG)
    end

    def Upload(host, dir, user, passwd)
        Neon.new.BaseDir()

        ftp = Net::FTP.new(host)
        ftp.debug_mode=true
        ftp.login(user, passwd)
        ftp.chdir(dir)

        Dir.foreach("."){|file|
            if file.include?(".tar.bz2")
                ftp.putbinaryfile(file, File.basename(file))
            end
        }
        print ftp.list('all')

        ftp.close
    end

end
