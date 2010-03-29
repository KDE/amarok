include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/tests
)

add_subdirectory( impl )

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
     ${AMAROK_SOURCE_TREE}/core-implementations/playlists/types/file/PlaylistFileSupport.cpp
     )
kde4_add_unit_test( testplaylistfilesupport ${testplaylistfilesupport_SRCS} )
target_link_libraries( testplaylistfilesupport ${QT_QTTEST_LIBRARY} amaroklib )

