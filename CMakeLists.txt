project(Amarok)

find_package(KDE4 REQUIRED)
include (KDE4Defaults)
include (MacroLibrary)

include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckPrototypeExists)
include(CheckTypeSize)
include(MacroBoolTo01)

set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
   ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )
add_definitions(-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w") #we're still worrying about all the errors

find_package(Taglib REQUIRED)
find_package(RUBY REQUIRED)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES} ${TAGLIB_INCLUDES})

macro_optional_find_package(Njb)
macro_optional_find_package(Ifp)
macro_optional_find_package(MySQL)
macro_optional_find_package(PostgreSQL)
macro_optional_find_package(Libvisual)
macro_optional_find_package(Mp4v2)
macro_optional_find_package(Xine)
macro_optional_find_package(Helix)
macro_optional_find_package(Mtp)
macro_optional_find_package(GObject)
macro_optional_find_package(GStreamer)

macro_bool_to_01(LIBVISUAL_FOUND HAVE_LIBVISUAL)

check_type_size("int" SIZEOF_INT)
check_type_size("long" SIZEOF_LONG)

configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )

add_subdirectory( src )

