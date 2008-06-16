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

module LibFetch
    def getTarball(comp)
        puts "#{thisMethod()}() started with component: #{comp}"

        baseDir()
        ftp = Net::FTP.new('ftp.kde.org')
        ftp.login
        files = ftp.chdir('pub/kde/snapshots')
        files = ftp.list(comp + "*.tar.bz2")
        file  = files[0].to_s.split(' ')[8]
        ftp.getbinaryfile(comp + ".tar.bz2", file, 1024)
        ftp.close

        rev = file.chomp(".tar.bz2").reverse.chomp("-" + comp.reverse).reverse
        if comp == "qt-copy"
            comp = "qt"
        end
        @dir = comp + "-" + rev
        system("tar -xf #{file}")
        FileUtils.rm_f(file)
        FileUtils.mv(file.chomp(".tar.bz2"), @dir)
        varMagic(comp, rev)
    end

    def checkOutEval(comp, path, dir, recursive=nil)
        unless recursive == "no"
            cmd = "svn"
        else
            cmd = "svn -N"
        end

        unless File.exists?(dir)
            cmd += " co svn+ssh://sitter@svn.kde.org/home/kde/trunk/#{path} #{SVNPATH}/#{dir}"
            puts "Checking out #{path} to #{SVNPATH}/#{dir}"
        else
            cmd += " up #{SVNPATH}/#{dir}"
            puts "Updating #{SVNPATH}/#{dir}"
        end

        count = 0
        %x[#{cmd}]
        while $? != 0
            unless count >= 20
                puts "retrying..."
                %x[#{cmd}]
                count += 1
            else
                puts "Error: svn checkout didn't exit properly in 20 tries, aborting."
                # TODO: currently avoids mr. clean!
                exit 0
            end
        end
    end

    def checkOut(comp, path, dir=nil, recursive=nil)
        puts "#{thisMethod()}() started with component: #{comp}"
        svnDir()
        unless dir
            dir=comp
        end
        checkOutEval(comp, path, dir, "#{recursive}")
        rev = %x[svn info #{dir}].split("\n")[4].split(" ")[1]
        @dir = SVNPATH + "/" + dir
        varMagic(comp, rev)
    end

    def varMagic(comp, rev)
        if @packages.nil?
            @packages = {comp => rev}
        else
            @packages[comp] = [rev]
        end
    end

    def createTar(comp, dir=nil)
        if @cofailed
            return
        end
        unless dir
            dir = "amarok-nightly-" + comp
        end
        puts "#{thisMethod()}() started with component: #{dir}"
        baseDir()
        FileUtils.cp_r(@dir, dir)
        system("find #{dir} -name '.svn' | xargs rm -rf")
        system("tar -cf #{dir}.tar #{dir}")
        system("bzip2 #{dir}.tar")
    end
end
