#!/usr/bin/env ruby
#
# Copyright (C) 2008 Harald Sitter <harald@getamarok.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

def QtCopy()
  GetTarball("qt-copy")
end

def Strigi()
  comp = "strigi"
  CheckOut(comp, "kdesupport/strigi")
  CreateTar(comp)
end

def TagLib()
  comp = "taglib"
  CheckOut(comp, "kdesupport/taglib")
  CreateTar(comp)
end

def KdeLibs()
  comp = "kdelibs"
  GetTarball(comp)

  # Change ksycoca file name
  file = File.new( "#{@dir}/kdecore/sycoca/ksycoca.h", File::RDWR )
  str = file.read()
  file.rewind()
  file.truncate( 0 )
  str.sub!( /#define KSYCOCA_FILENAME \".*\"/, "#define KSYCOCA_FILENAME \"neonsycoca\"" )
  file << str
  file.close()

  CreateTar(comp)
end


def KdeBaseRuntime()
  comp = "kdebase-runtime"

  CheckOut(comp, "KDE/kdebase/runtime", nil, "no")
  `svn up #{@dir}/cmake`
  `svn up #{@dir}/phonon`
  `svn up #{@dir}/kstyles`

  #create CMakeLists.txt
  cmakefile = File.new( "#{@dir}/CMakeLists.txt", File::CREAT | File::RDWR | File::TRUNC )
  cmakefile << "file(GLOB _po_files *.po)\n"
  cmakefile << "set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )\n"
  cmakefile << "find_package(KDE4 REQUIRED)\n"
  cmakefile << "find_package(Strigi REQUIRED)\n"
  cmakefile << "include(KDE4Defaults)\n"
  cmakefile << "include(MacroLibrary)\n"
  cmakefile << "include(MacroOptionalFindPackage)\n"
  cmakefile << "include(MacroOptionalAddSubdirectory)\n"
  cmakefile << "include(CheckFunctionExists)\n"
  cmakefile << "include(CheckIncludeFiles)\n"
  cmakefile << "check_include_files(sys/wait.h HAVE_SYS_WAIT_H)\n"
  cmakefile << "check_include_files(sys/time.h HAVE_SYS_TIME_H)\n"
  cmakefile << "configure_file (config-runtime.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-runtime.h )\n"
  cmakefile << "add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})\n"
  cmakefile << "include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES})\n"

  #define subdirectories to use
  cmakefile << "add_subdirectory(cmake)\n"
  cmakefile << "add_subdirectory(phonon)\n"
  cmakefile << "add_subdirectory(kstyles)\n"
  cmakefile.close()

  CreateTar(comp)
end


def Amarok()
  comp = "amarok"

  CheckOut(comp, "extragear/multimedia/amarok", "amarok-nightly")

  # Change version
  file = File.new( "#{@dir}/src/Amarok.h", File::RDWR )
  str = file.read()
  file.rewind()
  file.truncate( 0 )
  str.sub!( /APP_VERSION \".*\"/, "APP_VERSION \"#{APPVERSION}\"" )
  file << str
  file.close()

  CreateTar(comp)

end
