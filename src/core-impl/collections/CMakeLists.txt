include_directories(
                    ${KDE4_INCLUDE_DIR}
                    ${QT_INCLUDES}
                    ${Amarok_SOURCE_DIR}/src
                   )

set(AMAROK_COLLECTION_SUPPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/support)

add_subdirectory( daap )

if( NOT WIN32)
add_subdirectory( audiocd )
endif( NOT WIN32 )

add_subdirectory( ipodcollection )
add_subdirectory( mtpcollection )
add_subdirectory( umscollection )
add_subdirectory( db )

# only build playdarcollection if QJson was found
if( QJSON_FOUND )
    add_subdirectory( playdarcollection )
    add_subdirectory( spotifycollection )
endif( QJSON_FOUND )
add_subdirectory( upnpcollection )
add_subdirectory( nepomukcollection )
