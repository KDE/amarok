add_subdirectory( amzdownloader )
add_subdirectory( collectionscanner )

if( TAGLIB_FOUND )
    add_subdirectory( afttagger )
endif( TAGLIB_FOUND )
