include_directories( .
)

QT4_AUTOMOC( TestPlaylistFileSupport.cpp )

add_subdirectory( cue )
add_subdirectory( file )
add_subdirectory( multi )
