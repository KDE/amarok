include_directories( .
)

if(KDE4_BUILD_TESTS)
QT4_AUTOMOC( TestM3UPlaylist.cpp
             TestMetaTrack.cpp
             TestPlaylistFileSupport.cpp
             TestPLSPlaylist.cpp
             TestXSPFPlaylist.cpp
             )
add_subdirectory( cue )
add_subdirectory( file )
add_subdirectory( multi )

endif()

