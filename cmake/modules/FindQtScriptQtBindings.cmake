## Ian Monroe <ian@monroe.nu> Copyright 2009 
# released under public domain or:
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
# 

include(CheckCXXSourceRuns)

if(NOT WIN32)
    file( READ "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/bindingstest/QtScriptBindingsTest.cpp" source )
    message(STATUS "Checking if the QtScript Qt Bindings are installed.")


    set(CMAKE_REQUIRED_DEFINTIONS ${QT_DEFINITIONS} ${KDE4_DEFINITIONS} )
    set(CMAKE_REQUIRED_INCLUDES ${QT_QTCORE_INCLUDE_DIR} ${QT_QTSCRIPT_INCLUDE_DIR} ${KDE4_INCLUDES})
    set(CMAKE_REQUIRED_LIBRARIES ${QT_QTSCRIPT_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} -L${KDE4_LIB_DIR} -lkdecore -lkdeui)
    message( STATUS "includes ${CMAKE_REQUIRED_INCLUDES} libraries ${CMAKE_REQUIRED_LIBRARIES}" )
    CHECK_CXX_SOURCE_RUNS( "${source}" BINDINGS_RUN_RESULT)

    if(BINDINGS_RUN_RESULT EQUAL 1)
        message( STATUS "QtBindings found")
        set(QTSCRIPTQTBINDINGS_FOUND TRUE)
    else(BINDINGS_RUN_RESULT EQUAL 1)
        message( STATUS "QtBindings not found. run `cd cmake/modules/bindingstest; mkdir build; cd build; cmake ..; make; ./bindingstest; echo $?` If it prints '0' then you're actually fine.")
        set(QTSCRIPTQTBINDINGS_FOUND FALSE)
    endif(BINDINGS_RUN_RESULT EQUAL 1)

    set(CMAKE_REQUIRED_DEFINTIONS "" )
    set(CMAKE_REQUIRED_INCLUDES "")
    set(CMAKE_REQUIRED_LIBRARIES "")
else(NOT WIN32)
    set(QTSCRIPTQTBINDINGS_FOUND TRUE)
endif(NOT WIN32)
