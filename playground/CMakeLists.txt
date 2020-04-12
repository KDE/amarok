include (KDE4Defaults)
include (MacroLibrary)

include(CheckTypeSize)
include(MacroBoolTo01)
include(MacroLogFeature)

# Add a variable which points to amarok's source directory.
set( AMAROK_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src/ )

include_directories (${CMAKE_CURRENT_SOURCE_DIR} ${AMAROK_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  ${TAGLIB_INCLUDES})

add_subdirectory( src )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")
