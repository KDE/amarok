include_directories(
                    ${Amarok_SOURCE_DIR}/src
                   )

set(AMAROK_COLLECTION_SUPPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/support)

add_subdirectory( daap )

if(NOT WIN32)
    add_subdirectory( audiocd )
endif()

add_subdirectory( ipodcollection )
add_subdirectory( mtpcollection )
add_subdirectory( umscollection )
add_subdirectory( db )
add_subdirectory( playdarcollection )
