add_subdirectory( albums )
add_subdirectory( currenttrack )
add_subdirectory( labels )
add_subdirectory( lyrics )
add_subdirectory( photos )
add_subdirectory( tabs )

if( QT_QTOPENGL_FOUND )
    add_subdirectory( analyzer )
endif()

if( WITH_QTWEBKIT )
    add_subdirectory( info )
    add_subdirectory( wikipedia )
endif()

if( LIBLASTFM_FOUND )
    if ( WITH_QTWEBKIT )
        add_subdirectory( upcomingevents )
    endif()
    add_subdirectory( similarartists )
endif()

