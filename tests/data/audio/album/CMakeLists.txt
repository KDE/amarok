if(CMAKE_BUILD_TYPE MATCHES debugfull)
install(
        FILES
        Track01.ogg
        Track02.ogg
        Track03.ogg
        DESTINATION ${DATA_INSTALL_DIR}/amarok/testdata/album
)
endif(CMAKE_BUILD_TYPE MATCHES debugfull)
