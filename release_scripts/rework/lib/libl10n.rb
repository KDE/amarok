#!/usr/bin/env ruby
#
# Generic ruby library for KDE extragear/playground releases
#
# Copyright (C) 2007-2008 Harald Sitter <harald@getamarok.com>
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

@dlg = KDialog.new("#{NAME} release script","cookie")

def FetchTranslations()
  bar  = @dlg.progressbar("preparing l10n processing",1)
  Dir.chdir(BASEPATH + "/" + @folder)
  Dir.mkdir("l10n")
  Dir.mkdir("po")

  l10nlangs     = `svn cat #{@repo}/l10n-kde4/subdirs`.chomp!()
  @translations = []
  subdirs       = false

  bar.maxvalue = l10nlangs.count("\n")
  step         = 0

  for lang in l10nlangs
    lang.chomp!()
    bar.label    = "processing po/#{lang}"
    bar.progress = step
    step        += 1

    pofilename = "l10n-kde4/#{lang}/messages/#{COMPONENT}-#{SECTION}"
    # TODO: ruby-svn
    FileUtils.rm_rf("l10n")
    `svn co #{@repo}/#{pofilename} l10n 2> /dev/null`
    next if not FileTest.exists?( "l10n/#{NAME}.po" )

    dest = "po/#{lang}"
    Dir.mkdir( dest )
    puts "Copying #{lang}'s #{NAME}.po over ..."
    FileUtils.mv( "l10n/#{NAME}.po", dest )
    FileUtils.mv( "l10n/.svn", dest )
    puts "done.\n"

    # create lang's cmake files
    cmakefile = File.new( "#{dest}/CMakeLists.txt", File::CREAT | File::RDWR | File::TRUNC )
    cmakefile << "file(GLOB _po_files *.po)\n"
    cmakefile << "GETTEXT_PROCESS_PO_FILES(${CURRENT_LANG} ALL INSTALL_DESTINATION ${LOCALE_INSTALL_DIR} ${_po_files} )\n"
    cmakefile.close()

    # add to SVN in case we are tagging
    `svn add #{dest}/CMakeLists.txt`
    @translations += [lang]

    subdirs = true
  end
  bar.close

  if subdirs
    # create po's cmake file
    cmakefile = File.new( "po/CMakeLists.txt", File::CREAT | File::RDWR | File::TRUNC )
    cmakefile << "find_package(Gettext REQUIRED)\n"
    cmakefile << "if (NOT GETTEXT_MSGMERGE_EXECUTABLE)\n"
    cmakefile << "MESSAGE(FATAL_ERROR \"Please install msgmerge binary\")\n"
    cmakefile << "endif (NOT GETTEXT_MSGMERGE_EXECUTABLE)\n"
    cmakefile << "if (NOT GETTEXT_MSGFMT_EXECUTABLE)\n"
    cmakefile << "MESSAGE(FATAL_ERROR \"Please install msgmerge binary\")\n"
    cmakefile << "endif (NOT GETTEXT_MSGFMT_EXECUTABLE)\n"
    Dir.foreach( "po" ) {|lang|
      unless lang == ".." or lang == "." or lang == "CMakeLists.txt"
        cmakefile << "add_subdirectory(#{lang})\n"
      end
    }
    cmakefile.close()

    # adapt cmake file
    cmakefile = File.new( "CMakeLists.txt", File::APPEND | File::RDWR )
    cmakefile << "include(MacroOptionalAddSubdirectory)\n"
    cmakefile << "macro_optional_add_subdirectory( po )\n"
    cmakefile.close()
  else
    FileUtils.rm_rf( "po" )
  end

  FileUtils.rm_rf( "l10n" )
end


def FetchDocumentation()
  bar  = @dlg.progressbar("preparing l10n processing",1)
  Dir.chdir(BASEPATH + "/" + @folder)
  Dir.mkdir("doc")
  Dir.chdir("doc")

  l10nlangs = `svn cat #{@repo}/l10n-kde4/subdirs`.chomp!()
  subdirs   = false

  bar.maxvalue = l10nlangs.count("\n")
  step         = 0

  `svn co #{@repo}/#{COMPONENT}/#{SECTION}/doc/#{NAME}`

  Dir.chdir(BASEPATH + "/" + @folder)
  Dir.mkdir( "l10n" )
  Dir.chdir( "l10n" )

  # docs
  for lang in l10nlangs
    lang.chomp!()

    bar.label    = "processing #{lang}'s #{NAME} documentation"
    bar.progress = step
    step        += 1

    FileUtils.rm_rf( "../doc/#{lang}" )
    FileUtils.rm_rf( NAME )
    docdirname = "l10n-kde4/#{lang}/docs/extragear-multimedia/amarok"
    `svn co -q #{@repo}/#{docdirname} > /dev/null 2>&1`
    next unless FileTest.exists?( "amarok" )
    puts "Copying #{lang}'s #{NAME} documentation over... "
    `cp -R amarok/ ../doc/#{lang}`

    cmakefile = File.new( "../doc/#{lang}/CMakeLists.txt", File::CREAT | File::RDWR | File::TRUNC )
    cmakefile << "kde4_create_handbook(index.docbook INSTALL_DESTINATION \${HTML_INSTALL_DIR}/\${CURRENT_LANG}/ SUBDIR #{NAME} )\n"
    cmakefile.close()

    puts( "done.\n" )

    subdirs = true
  end
  bar.close

  Dir.chdir(BASEPATH + "/" + @folder)

  if subdirs
    # create doc's cmake file
    cmakefile = File.new( "doc/CMakeLists.txt", File::CREAT | File::RDWR | File::TRUNC )
    Dir.foreach( "doc" ) {|lang|
      unless lang == ".." or lang == "." or lang == "CMakeLists.txt"
        cmakefile << "add_subdirectory(#{lang})\n"
      end
    }
    cmakefile.close()

    # adapt cmake file
    cmakefile = File.new( "CMakeLists.txt", File::APPEND | File::RDWR )
    unless File.exists?("po")
      cmakefile << "include(MacroOptionalAddSubdirectory)\n"
    end
    cmakefile << "macro_optional_add_subdirectory( doc )\n"
    cmakefile.close()
  else
    FileUtils.rm_rf( "doc" )
  end

  FileUtils.rm_rf( "l10n" )
end
