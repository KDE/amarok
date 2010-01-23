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

require 'libfetch.rb'

include LibFetch

def fetchQtCopy()
    getTarball("qt-copy")
    createTar("qt")
end

def fetchKdeSupportMinimal()
    comp = "kdesupport"

    checkOut(comp, "kdesupport", "kdesupport-minimal", "no")

    checkOutEval(comp, "kdesupport/automoc", "kdesupport-minimal/automoc")
    checkOutEval(comp, "kdesupport/strigi", "kdesupport-minimal/strigi")
    checkOutEval(comp, "kdesupport/taglib", "kdesupport-minimal/taglib")
    checkOutEval(comp, "kdesupport/phonon", "kdesupport-minimal/phonon")

    # create CMakeLists.txt
    File.delete("#{SVNPATH}/kdesupport-minimal/CMakeLists.txt")
    cmakefile = File.new( "#{SVNPATH}/kdesupport-minimal/CMakeLists.txt", File::CREAT | File::RDWR | File::TRUNC )
    cmakefile << "project(KDESupport)\n"
    cmakefile << "add_subdirectory(automoc)\n"
    cmakefile << "add_subdirectory(strigi)\n"
    cmakefile << "add_subdirectory(taglib)\n"
    cmakefile << "add_subdirectory(phonon)\n"
    cmakefile.close()

    createTar(comp)
end

def fetchKdeLibs()
    comp = "kdelibs"
    checkOut(comp, "KDE/kdelibs")
    createTar(comp)
end

def fetchKdeBaseMinimal()
    comp = "kdebase"

    checkOut(comp, "KDE/kdebase/runtime", "kdebase-minimal", "no")

    checkOutEval(comp, "KDE/kdebase/runtime/cmake", "kdebase-minimal/cmake")
    checkOutEval(comp, "KDE/kdebase/runtime/phonon", "kdebase-minimal/phonon")
    checkOutEval(comp, "KDE/kdebase/runtime/kstyles", "kdebase-minimal/kstyles")
    checkOutEval(comp, "KDE/kdebase/runtime/kcmshell", "kdebase-minimal/kcmshell")
    checkOutEval(comp, "KDE/kdebase/runtime/pics", "kdebase-minimal/pics")

    # create CMakeLists.txt
    File.delete("#{SVNPATH}/kdebase-minimal/CMakeLists.txt")
    cmakefile = File.new( "#{SVNPATH}/kdebase-minimal/CMakeLists.txt", File::CREAT | File::RDWR | File::TRUNC )
    cmakefile << "file(GLOB _po_files *.po)\n"
    cmakefile << "set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )\n"
    cmakefile << "find_package(KDE4 REQUIRED)\n"
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

    # define subdirectories to use
    cmakefile << "add_subdirectory(cmake)\n"
    cmakefile << "add_subdirectory(phonon)\n"
    cmakefile << "add_subdirectory(kstyles)\n"
    cmakefile << "add_subdirectory(kcmshell)\n"
    cmakefile << "add_subdirectory(pics)\n"
    cmakefile.close()

    createTar(comp)
end

def fetchAmarok()
    comp = "amarok"

    checkOut(comp, "extragear/multimedia/amarok", "amarok-nightly")

    # change version
    file = File.new( "#{@dir}/src/Amarok.h", File::RDWR )
    str = file.read()
    file.rewind()
    file.truncate( 0 )
    str.sub!( /AMAROK_VERSION \".*\"/, "AMAROK_VERSION \"#{AMAROKVERSION}\"" )
    file << str
    file.close()

    createTar(comp, "amarok-nightly")
end

# Full blown
def fetchKdeSupport()
    comp = "kdesupport"
    checkOut(comp, "kdesupport")
    createTar(comp)
end

def fetchKdePimLibs()
    comp = "kdepimlibs"
    checkOut(comp, "KDE/kdepimlibs")
    createTar(comp)
end

def fetchKdeBase()
    comp = "kdebase"
    checkOut(comp, "KDE/kdebase")
    createTar(comp)
end

def fetchKdeSdk()
    comp = "kdesdk"
    checkOut(comp, "KDE/kdesdk")
    createTar(comp)
end

def fetchKdeMultimedia()
    comp = "kdemultimedia"
    checkOut(comp, "KDE/kdemultimedia")
    createTar(comp)
end

def fetchKdeGraphics()
    comp = "kdegraphics"
    checkOut(comp, "KDE/kdegraphics")
    createTar(comp)
end

def fetchKdeNetwork()
    comp = "kdenetwork"
    checkOut(comp, "KDE/kdenetwork")
    createTar(comp)
end
