include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/tests
)

#------------------------ Test M3UPlaylist -----------------------------

set( testm3uplaylist_SRCS m3u/TestM3UPlaylist.cpp )
kde4_add_unit_test( testm3uplaylist ${testm3uplaylist_SRCS} )
target_link_libraries( testm3uplaylist ${QT_QTTEST_LIBRARY} amarokcore amaroklib )

#------------------------ Test PLSPlaylist -----------------------------

set( testplsplaylist_SRCS pls/TestPLSPlaylist.cpp )
kde4_add_unit_test( testplsplaylist ${testplsplaylist_SRCS} )
target_link_libraries( testplsplaylist ${QT_QTTEST_LIBRARY} amarokcore amaroklib )

#------------------------ Test XSPFPLAYLIST -----------------------------

set( testxspfplaylist_SRCS xspf/TestXSPFPlaylist.cpp )
kde4_add_unit_test( testxspfplaylist ${testxspfplaylist_SRCS} )
target_link_libraries( testxspfplaylist ${QT_QTTEST_LIBRARY} amarokcore amaroklib )

#------------------------ Test PlaylistFileSupport -----------------------------

set( testplaylistfilesupport_SRCS
     TestPlaylistFileSupport.cpp
     ${AMAROK_SOURCE_TREE}/core-impl/playlists/types/file/PlaylistFileSupport.cpp
     )
kde4_add_unit_test( testplaylistfilesupport ${testplaylistfilesupport_SRCS} )
target_link_libraries( testplaylistfilesupport ${QT_QTTEST_LIBRARY} amarokcore amaroklib )

