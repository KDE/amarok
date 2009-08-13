include_directories( .
)

QT4_AUTOMOC( TestM3UPlaylist.cpp
             TestMetaTrack.cpp
             TestPlaylistFileSupport.cpp
             TestPLSPlaylist.cpp
             TestXSPFPlaylist.cpp
             )

add_subdirectory( cue )
add_subdirectory( file )
add_subdirectory( multi )
