# Generic ruby library for KDE extragear/playground releases
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

require 'lib/l10ncore'
require 'lib/l10nstat'

include L10nCore

def fetch_l10n
    src_dir
    @name = NAME.split("-").join #strip hyphens (required for kipi-plugins)
    ld    = "l10n"
    pd    = "po"
    Dir.mkdir pd

    l10nlangs = %x[svn cat #{@repo}/l10n-kde4/subdirs].split("\n")
    @l10n     = []

    for lang in l10nlangs
        next if lang == "x-test"

        pofilename = "l10n-kde4/#{lang}/messages/#{COMPONENT}-#{SECTION}"
        rm_rf ld
        next if %x[svn ls #{@repo}/#{pofilename}].empty?
        # do a complete checkout because it is a) faster b) more flexible
        system("svn co #{@repo}/#{pofilename} #{ld}")
        exit_checker($?,pofilename)

        files = Dir.glob("l10n/#{@name.chop}*.po") # chop last char because of kipiplugin(s)
        next if files == []

        dest = pd + "/#{lang}"
        Dir.mkdir dest
        puts("Copying #{lang}\'s #{@name}.po(s) over ...")
        mv( files, dest )
        mv( ld + "/.svn", dest )

        # create lang's cmake files
        cmakefile = File.new( "#{dest}/CMakeLists.txt", File::CREAT | File::RDWR | File::TRUNC )
        cmakefile << "file(GLOB _po_files *.po)\n"
        cmakefile << "GETTEXT_PROCESS_PO_FILES(#{lang} ALL INSTALL_DESTINATION ${LOCALE_INSTALL_DIR} ${_po_files} )\n"
        cmakefile.close

        # add to SVN in case we are tagging
        %x[svn add #{dest}/CMakeLists.txt]
        @l10n += [lang]

        puts "done."
    end

    # the statistics depend on @l10n, so invoking it only within fetch_l10n makes most sense
    l10nstat unless $options[:stat] == false or @l10n.empty?

    if not @l10n.empty? # make sure we actually fetched languages
        # create po's cmake file
        cmake_creator(pd,true)

        # change cmake file
        cmake_add_sub(pd)
    else
        rm_rf pd
    end

    rm_rf ld
end
