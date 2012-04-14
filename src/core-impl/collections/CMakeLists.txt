include_directories(
                    ${KDE4_INCLUDE_DIR}
                    ${QT_INCLUDES}
                    ${Amarok_SOURCE_DIR}/src
                   )

set(AMAROK_COLLECTION_SUPPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/support)

add_subdirectory( daap )
# NOTE: disabled until ported to new framework
add_subdirectory( audiocd )

#add_subdirectory( ipodcollection )
add_subdirectory( mtpcollection )
add_subdirectory( umscollection )
add_subdirectory( db )

# needs things from playground/base/nepomuk-kde
#if (SOPRANO_FOUND AND NEPOMUK_FOUND)
#    MESSAGE(STATUS "soprano and nepomuk found, building optional nepomuk collection")
#    add_subdirectory(nepomukcollection)
#endif (SOPRANO_FOUND AND NEPOMUK_FOUND)

# only build playdarcollection if QJson was found
if( QJSON_FOUND )
    add_subdirectory( playdarcollection )
endif( QJSON_FOUND )
add_subdirectory( upnpcollection )
