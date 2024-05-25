include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests

)

#------------------------ Test ASXPlaylist -----------------------------

set( testasxplaylist_SRCS asx/TestASXPlaylist.cpp )
add_executable( testasxplaylist ${testasxplaylist_SRCS} )
add_test(NAME testasxplaylist COMMAND $<TARGET_FILE:testasxplaylist>)
ecm_mark_as_test(testasxplaylist)
target_link_libraries( testasxplaylist KF5::ThreadWeaver Qt::Test Qt::Core Qt::Gui amarokcore amaroklib )

#------------------------ Test M3UPlaylist -----------------------------

set( testm3uplaylist_SRCS m3u/TestM3UPlaylist.cpp )
add_executable( testm3uplaylist ${testm3uplaylist_SRCS} )
add_test(NAME testm3uplaylist COMMAND $<TARGET_FILE:testm3uplaylist>)
ecm_mark_as_test(testm3uplaylist)
target_link_libraries( testm3uplaylist KF5::ThreadWeaver Qt::Test Qt::Core Qt::Gui amarokcore amaroklib )

#------------------------ Test PLSPlaylist -----------------------------

set( testplsplaylist_SRCS pls/TestPLSPlaylist.cpp )
add_executable( testplsplaylist ${testplsplaylist_SRCS} )
add_test(NAME testplsplaylist COMMAND $<TARGET_FILE:testplsplaylist>)
ecm_mark_as_test(testplsplaylist)
target_link_libraries( testplsplaylist KF5::ThreadWeaver Qt::Test Qt::Core amarokcore amaroklib )

#------------------------ Test XSPFPLAYLIST -----------------------------

set( testxspfplaylist_SRCS xspf/TestXSPFPlaylist.cpp )
add_executable( testxspfplaylist ${testxspfplaylist_SRCS} )
add_test(NAME testxspfplaylist COMMAND $<TARGET_FILE:testxspfplaylist>)
ecm_mark_as_test(testxspfplaylist)
target_link_libraries( testxspfplaylist KF5::ThreadWeaver Qt::Test Qt::Core amarokcore amaroklib )

#------------------------ Test PlaylistFileSupport -----------------------------

set( testplaylistfilesupport_SRCS
     TestPlaylistFileSupport.cpp
     ${AMAROK_SOURCE_TREE}/core-impl/playlists/types/file/PlaylistFileSupport.cpp
     )
add_executable( testplaylistfilesupport ${testplaylistfilesupport_SRCS} )
add_test(NAME testplaylistfilesupport COMMAND $<TARGET_FILE:testplaylistfilesupport>)
ecm_mark_as_test(testplaylistfilesupport)
target_link_libraries( testplaylistfilesupport KF5::KIOCore Qt::Test Qt::Core amarokcore amaroklib )

