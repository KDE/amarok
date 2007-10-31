#!/usr/bin/env ruby
#
# Generic ruby library for Amarok releases
#
# Copyright (C) 2007 Harald Sitter <harald@getamarok.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

require './lib/libkdialog.rb'
require 'fileutils'

@dlg = KDialog.new( "Release script" )


def Information()
def CheckoutLocation()
  location = @dlg.combobox( "Select checkout\\'s place:", "Trunk Stable Tag" ).chomp!()
  if location == "Stable"
  puts "2"
    @useStable = true
  elsif location == "Tag"
  puts "1"
    useTag = @dlg.inputbox("Enter the tag name:").chomp!()
  end
end

def ReleaseVersion( useTag )
  if useTag and not useTag.empty?()
    version = tag
  else
    version  = @dlg.inputbox("Enter the release version:").chomp!()
  end
end

CheckoutLocation()
# ReleaseVersion()
end


def FetchSource( folder, name, repo )
    `svn co #{repo}/extragear/multimedia/#{name} #{folder}`
end


def FetchTranslations( folder, name, repo )
    Dir.chdir(folder)
    puts "\n"
    puts "**** l10n ****"
    puts "\n"

    i18nlangs = `svn cat #{repo}/l10n-kde4/subdirs`
    subdirs   = false
    Dir.mkdir( "l10n" )
    Dir.mkdir( "po" )

    for lang in i18nlangs
        lang.chomp!()
        pofilename = "l10n-kde4/#{lang}/messages/extragear-multimedia/#{name}.po"
        `svn cat #{repo}/#{pofilename} 2> /dev/null | tee l10n/#{name}.po`
        next if FileTest.size( "l10n/#{name}.po" ) == 0

        dest = "po/#{lang}"
        Dir.mkdir( dest )
        puts "Copying #{lang}'s #{name}.po over ..."
        FileUtils.mv( "l10n/#{name}.po", dest )
        puts "done.\n" 

        # create lang's cmake files
        cmakefile = File.new( "#{dest}/CMakeLists.txt", File::CREAT | File::RDWR | File::TRUNC )
            cmakefile << "set(CURRENT_LANG #{lang})\n"
            cmakefile << "kde4_create_po_files()\n"
            cmakefile << "kde4_install_po_files(${CURRENT_LANG})\n"
        cmakefile.close()

        subdirs = true
    end

    if subdirs
         # Remove x-test language
        FileUtils.rm_rf( "po/x-test" )

        # create po's cmake file
        cmakefile = File.new( "po/CMakeLists.txt", File::CREAT | File::RDWR | File::TRUNC )
            Dir.foreach("po") {|lang|
            unless lang == ".." or lang == "." or lang == "CMakeLists.txt"
                cmakefile << "add_subdirectory(#{lang})\n"
            end
            }
        cmakefile.close()

        # adapt cmake file
        cmakefile = File.new( "CMakeLists.txt", File::APPEND | File::RDWR )
            cmakefile << "find_package(Msgfmt)\n"
            cmakefile << "if(MSGFMT_FOUND)\n"
            cmakefile << "macro_optional_add_subdirectory( po )\n"
            cmakefile << "MESSAGE(STATUS \"msgfmt is missing. The translations will not be compiled.\")\n"
            cmakefile << "endif(MSGFMT_FOUND)\n"
        cmakefile.close()
    else
        FileUtils.rm_rf( "po" )
    end

    FileUtils.rm_rf( "l10n" )
end


def CreateTar( folder )
    `find -name ".svn" | xargs rm -rf`
    `tar -cf #{folder}.tar #{folder}`
    `bzip2 #{folder}.tar`
    FileUtils.rm_rf(folder)
end

