project(Amarok)

cmake_minimum_required(VERSION 2.4.5)


#We depend on kde4.1 now.. I believe this is the easiest way to do it.
set(KDE_MIN_VERSION "4.1.0")
find_package( KDE4 REQUIRED )
include (KDE4Defaults)
include (MacroLibrary)

set(TAGLIB_MIN_VERSION "1.5")
find_package(Taglib)

include(CheckTypeSize)
include(MacroBoolTo01)
include(MacroLogFeature)

macro_log_feature( KDE4_FOUND "kdelibs" "The toolkit Amarok uses to build" "http://www.kde.org" TRUE "4.1.0" "" )

set( CMAKE_MODULE_PATH    ${CMAKE_MODULE_PATH}
                          ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )

# Add a variable which points to amarok's source directory.
set( AMAROK_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src/ )

include_directories (${CMAKE_CURRENT_SOURCE_DIR} ${AMAROK_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} ${TAGLIB_INCLUDES})

check_type_size("int" SIZEOF_INT)
check_type_size("long" SIZEOF_LONG)


add_subdirectory( src )


SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")


