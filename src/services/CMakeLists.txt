include_directories(
                    ${KDE4_INCLUDE_DIR}
                    ${QT_INCLUDES}
                    ${Amarok_SOURCE_DIR}/src
                   )

add_subdirectory( magnatune )
add_subdirectory( jamendo )
add_subdirectory( mp3tunes )
add_subdirectory( ampache )
if( LIBLASTFM_FOUND )
	add_subdirectory( lastfm )
endif( LIBLASTFM_FOUND )
add_subdirectory( opmldirectory )
