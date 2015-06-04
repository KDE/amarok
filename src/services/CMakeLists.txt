include_directories(
                    ${Amarok_SOURCE_DIR}/src
                   )

add_subdirectory( amazon )
add_subdirectory( magnatune )
add_subdirectory( jamendo )
add_subdirectory( mp3tunes )
if (QCA2_FOUND)
    add_subdirectory( ampache )
endif ()
if (LIBMYGPO_QT_FOUND )
    add_subdirectory( gpodder )
endif()
if( LIBLASTFM_FOUND )
    add_subdirectory( lastfm )
endif()
add_subdirectory( opmldirectory )
