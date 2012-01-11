add_subdirectory( wikipedia )
add_subdirectory( lyrics )
add_subdirectory( current )
add_subdirectory( info )
add_subdirectory( videoclip )
add_subdirectory( photos )
add_subdirectory( labels )
add_subdirectory( tabs )

if( OPENGL_FOUND )
    add_subdirectory( spectrumanalyzer )
endif()

#add_subdirectory( songkick )

if(LIBLASTFM_FOUND)
    add_subdirectory( similarartists )
    add_subdirectory( upcomingevents )
endif(LIBLASTFM_FOUND)
