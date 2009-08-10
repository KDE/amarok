include_directories( .
)

QT4_AUTOMOC( TestMetaTrack.cpp
             TestPlaylistFileSupport.cpp
             )

add_subdirectory( cue )
add_subdirectory( file )
add_subdirectory( multi )
