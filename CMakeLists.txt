project(Amarok)

set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
   ${CMAKE_SOURCE_DIR}/amarok/cmake/modules )
add_definitions(-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS) 
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w") #we're still worrying about all the errors

find_package(Taglib REQUIRED)
find_package(RUBY REQUIRED)

macro_optional_find_package(Njb)
macro_optional_find_package(Sqlite)
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

add_subdirectory( src )

