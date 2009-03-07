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

message(STATUS "Checking if the QtScript Qt Bindings are installed.")
set(LINK_LIBRARIES "-lQtCore -lkdeui -lkdecore -lQtScript")
#set(LINK_LIBRARIES "${QT_QTCORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KDECORE_LIBS} -${QT_QTSCRIPT_LIBS}")
try_run(BINDINGS_RUN_RESULT BINDINGS_COMPILE_RESULT
        ${PROJECT_BINARY_DIR}/CMakeTmp/generator
        ${PROJECT_SOURCE_DIR}/cmake/modules/QtScriptBindingsTest.cpp 
        CMAKE_FLAGS "-DCOMPILE_DEFINITIONS:STRING=${LINK_LIBRARIES}"
        )
if(BINDINGS_RUN_RESULT EQUAL 0)
    message( STATUS "QtBindings found")
    set(QTSCRIPT_QT_BINDINGS_FOUND TRUE)
else(BINDINGS_RUN_RESULT EQUAL 0)
    MESSAGE(STATUS "QtBindings not found")
    	set(QTSCRIPT_QT_BINDINGS_FOUND FALSE)
endif(BINDINGS_RUN_RESULT EQUAL 0)

