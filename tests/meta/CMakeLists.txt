include_directories( .
)

if(CMAKE_BUILD_TYPE MATCHES debugfull)
QT4_AUTOMOC( TestM3UPlaylist.cpp
             TestMetaTrack.cpp
             TestPlaylistFileSupport.cpp
             TestPLSPlaylist.cpp
             TestXSPFPlaylist.cpp
             )
endif(CMAKE_BUILD_TYPE MATCHES debugfull)

add_subdirectory( cue )
add_subdirectory( file )
add_subdirectory( multi )
