include_directories( .
)

if(KDE4_BUILD_TESTS)
QT4_AUTOMOC( TestMetaTrack.cpp
             TestPlaylistFileSupport.cpp
             TestPLSPlaylist.cpp
             TestXSPFPlaylist.cpp
             )
add_subdirectory( cue )
add_subdirectory( file )
add_subdirectory( multi )

#------------------------ Test M3UPlaylist -----------------------------

set( testm3uplaylist_SRCS TestM3UPlaylist.cpp )
kde4_add_unit_test( testm3uplaylist ${testm3uplaylist_SRCS} )
target_link_libraries( testm3uplaylist ${QT_QTTEST_LIBRARY} amaroklib )

endif()

