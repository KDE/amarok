if(CMAKE_BUILD_TYPE MATCHES debugfull)
install(
        FILES
        no-playlist.png
        test.asx
        test.m3u
        test.pls
        test.ram
        test.smil
        test.xspf
        DESTINATION ${DATA_INSTALL_DIR}/amarok/testdata/playlists
)
endif(CMAKE_BUILD_TYPE MATCHES debugfull)
