if(CMAKE_BUILD_TYPE MATCHES debugfull)
    if( WITH_PLAYER )
        add_subdirectory( amaroktest )
    endif( WITH_PLAYER )
endif(CMAKE_BUILD_TYPE MATCHES debugfull)

add_subdirectory( collectionscanner )
add_subdirectory( afttagger )
