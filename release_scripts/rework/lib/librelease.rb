#!/usr/bin/env ruby
#
# Generic ruby library for KDE extragear/playground releases
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

def InformationQuery()
  def CheckoutLocation(dlg)
    location = dlg.combobox("Select checkout\\'s place:", "Trunk Stable Tag")
    puts location #DEBUG
    if location == "Stable"
      @useStable = true
    elsif location == "Tag"
      @tag = dlg.inputbox("Enter the tag name:")
      puts @tag #DEBUG
    end
  end

  def ReleaseVersion(dlg)
    if @tag and not @tag.empty?()
      @version = @tag
    else
      @version = dlg.inputbox("Enter the release version:")
    end
    puts @version #DEBUG
  end

  def SvnProtcol(dlg)
    @protocol = dlg.radiolist("Do you use svn+ssh, https or anonsvn :",["svn+ssh","https","anonsvn"],1)
    puts @protocol #DEBUG
  end

  def SvnUsername(dlg)
    if @protocol == "anonsvn"
      @protocol = "svn"
      @user = "anonsvn"
    else
      @user = dlg.inputbox("Your SVN user:")
      @user += "@svn"
    end
    puts @user #DEBUG
  end

  dlg = KDialog.new("#{NAME} release script","cookie")

  CheckoutLocation(dlg)
  ReleaseVersion(dlg)
  SvnProtcol(dlg)
  SvnUsername(dlg)
end


def FetchSource()
  FileUtils.rm_rf( @folder )
  FileUtils.rm_rf( "#{@folder}.tar.gz" )

  branch = "trunk"

  if @useStable
    branch = "branches/stable"
  elsif @tag and not @tag.empty?()
    branch = "tags/#{NAME}/#{@tag}"
  end

  @repo = "#{@protocol}://#{@user}.kde.org/home/kde/#{branch}"
  puts @repo #DEBUG

  puts "Fetching source from #{branch}...\n\n"
  # TODO: ruby-svn
  `svn co #{@repo}/#{COMPONENT}/#{SECTION}/#{NAME} #{@folder}`
end


def FetchTranslations()
  Dir.chdir(@folder)
  puts "\n"
  puts "**** l10n ****"
  puts "\n"

  # TODO: ruby-svn
  i18nlangs = `svn cat #{@repo}/l10n-kde4/subdirs`
  subdirs   = false
  Dir.mkdir("l10n")
  Dir.mkdir("po")

  for lang in i18nlangs
    lang.chomp!()
    pofilename = "l10n-kde4/#{lang}/messages/#{COMPONENT}-#{SECTION}/#{NAME}.po"
    # TODO: ruby-svn
    `svn cat #{@repo}/#{pofilename} 2> /dev/null | tee l10n/#{NAME}.po`
    next if FileTest.size( "l10n/#{NAME}.po" ) == 0

    dest = "po/#{lang}"
    Dir.mkdir( dest )
    puts "Copying #{lang}'s #{NAME}.po over ..."
    FileUtils.mv( "l10n/#{NAME}.po", dest )
    puts "done.\n"

    # create lang's cmake files
    cmakefile = File.new( "#{dest}/CMakeLists.txt", File::CREAT | File::RDWR | File::TRUNC )
    cmakefile << "file(GLOB _po_files *.po)\n"
    cmakefile << "GETTEXT_PROCESS_PO_FILES(${CURRENT_LANG} ALL INSTALL_DESTINATION ${LOCALE_INSTALL_DIR} ${_po_files} )\n"
    cmakefile.close()

    subdirs = true
  end

  if subdirs
    # Remove x-test language
    FileUtils.rm_rf( "po/x-test" )

    # create po's cmake file
    cmakefile = File.new( "po/CMakeLists.txt", File::CREAT | File::RDWR | File::TRUNC )
    Dir.foreach( "po" ) {|lang|
      unless lang == ".." or lang == "." or lang == "CMakeLists.txt"
        cmakefile << "add_subdirectory(#{lang})\n"
      end
    }
    cmakefile.close()

    # adapt cmake file
    cmakefile = File.new( "CMakeLists.txt", File::APPEND | File::RDWR )
    cmakefile << "find_package(Gettext)\n"
    cmakefile << "if(Gettext_FOUND AND GETTEXT_MSGMERGE_EXECUTABLE AND GETTEXT_MSGFMT_EXECUTABLE)\n"
    cmakefile << "macro_optional_add_subdirectory(po)\n"
    cmakefile << "MESSAGE(STATUS \"Gettext is missing or not properly installed. The translations will not be compiled.\")\n"
    cmakefile << "endif(Gettext_FOUND AND GETTEXT_MSGMERGE_EXECUTABLE AND GETTEXT_MSGFMT_EXECUTABLE)\n"
    cmakefile.close()
  else
    FileUtils.rm_rf( "po" )
  end

  FileUtils.rm_rf( "l10n" )
end


def CreateTar()
  `find -name ".svn" | xargs rm -rf`
  `tar -cf #{@folder}.tar #{@folder}`
  `bzip2 #{@folder}.tar`
  FileUtils.rm_rf(@folder)
end
