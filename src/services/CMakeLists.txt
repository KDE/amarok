include_directories( ${Amarok_SOURCE_DIR}/src )

add_subdirectory( magnatune )

# add_subdirectory( jamendo ) // Needs complete rewrite because of API change

if(OPENSSL_FOUND OR LIBGCRYPT_FOUND)
    if(LIBXML2_FOUND AND CURL_FOUND AND LOUDMOUTH_FOUND AND GLIB2_FOUND AND GOBJECT_FOUND AND QT5_GLIB_SUPPORT)
#        add_subdirectory( mp3tunes ) // Needs more work
    endif()
endif()

add_subdirectory( ampache )

if ( Mygpo-qt5_FOUND )
    add_subdirectory( gpodder )
endif()

if( LIBLASTFM_FOUND )
    add_subdirectory( lastfm )
endif()

add_subdirectory( opmldirectory )
