if(CMAKE_BUILD_TYPE MATCHES debugfull)
    if( WITH_PLAYER )
        add_subdirectory( amaroktest )
    endif( WITH_PLAYER )
endif(CMAKE_BUILD_TYPE MATCHES debugfull)

if(GTACFEAT_FOUND)
    ADD_DEFINITIONS(-DHAVE_FINGERPRINT)
    include_directories( ${GTACFEAT_INCLUDE_DIR} )
endif(GTACFEAT_FOUND)

add_subdirectory( collectionscanner )
add_subdirectory( afttagger )
