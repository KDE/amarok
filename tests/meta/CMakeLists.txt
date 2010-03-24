include_directories( .
)

if(KDE4_BUILD_TESTS)
add_subdirectory( cue )
add_subdirectory( file )
add_subdirectory( multi )

#------------------------ Test M3UPlaylist -----------------------------

set( testm3uplaylist_SRCS TestM3UPlaylist.cpp )
kde4_add_unit_test( testm3uplaylist ${testm3uplaylist_SRCS} )
target_link_libraries( testm3uplaylist ${QT_QTTEST_LIBRARY} amaroklib )

#------------------------ Test MetaTrack -----------------------------

set( testmetatrack_SRCS TestMetaTrack.cpp )
kde4_add_unit_test( testmetatrack ${testmetatrack_SRCS} )
target_link_libraries( testmetatrack ${QT_QTTEST_LIBRARY} amaroklib )

#------------------------ Test PLSPlaylist -----------------------------

set( testplsplaylist_SRCS TestPLSPlaylist.cpp )
kde4_add_unit_test( testplsplaylist ${testplsplaylist_SRCS} )
target_link_libraries( testplsplaylist ${QT_QTTEST_LIBRARY} amaroklib )

#------------------------ Test XSPFPLAYLIST -----------------------------

set( testxspfplaylist_SRCS TestXSPFPlaylist.cpp )
kde4_add_unit_test( testxspfplaylist ${testxspfplaylist_SRCS} )
target_link_libraries( testxspfplaylist ${QT_QTTEST_LIBRARY} amaroklib )

#------------------------ Test PlaylistFileSupport -----------------------------

set( testplaylistfilesupport_SRCS
     TestPlaylistFileSupport.cpp
     ${AMAROK_SOURCE_TREE}/core/playlists/impl/file/PlaylistFileSupport.cpp
     )
kde4_add_unit_test( testplaylistfilesupport ${testplaylistfilesupport_SRCS} )
target_link_libraries( testplaylistfilesupport ${QT_QTTEST_LIBRARY} amaroklib )

endif()

