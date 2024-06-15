add_subdirectory( albums )
add_subdirectory( currenttrack )
#add_subdirectory( info )
#add_subdirectory( labels )
add_subdirectory( lyrics )
add_subdirectory( photos )
#add_subdirectory( tabs )
add_subdirectory( analyzer )

if( Qt5WebEngine_FOUND )
    add_subdirectory( wikipedia )
endif()

if( LIBLASTFM_FOUND )
#    add_subdirectory( similarartists )
endif()

