add_subdirectory( albums )
add_subdirectory( currenttrack )
add_subdirectory( lyrics )
add_subdirectory( info )
add_subdirectory( wikipedia )
add_subdirectory( videoclip )
add_subdirectory( photos )

if( LIBLASTFM_FOUND )
    add_subdirectory( upcomingevents )
    add_subdirectory( similarartists )
endif( LIBLASTFM_FOUND )

#add_subdirectory( songkick )
