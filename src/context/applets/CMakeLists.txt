add_subdirectory( albums )
add_subdirectory( currenttrack )
add_subdirectory( lyrics )
add_subdirectory( info )
add_subdirectory( wikipedia )
add_subdirectory( photos )
add_subdirectory( labels )
add_subdirectory( tabs )

if( WITH_SPECTRUM_ANALYZER AND OPENGL_FOUND )
    add_subdirectory( spectrumanalyzer )
endif()

if( LIBLASTFM_FOUND )
    add_subdirectory( upcomingevents )
    add_subdirectory( similarartists )
endif()

#add_subdirectory( songkick )
