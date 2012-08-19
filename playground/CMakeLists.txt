include (KDE4Defaults)
include (MacroLibrary)

include(CheckTypeSize)
include(MacroBoolTo01)
include(MacroLogFeature)

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
