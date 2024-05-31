include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/tests
)

#------------------------ Test PlaylistFormat -----------------------------

set( testplaylistformat_SRCS TestPlaylistFormat.cpp )
add_executable( testplaylistformat ${testplaylistformat_SRCS} )
add_test(NAME testplaylistformat COMMAND $<TARGET_FILE:testplaylistformat>)
ecm_mark_as_test(testplaylistformat)
target_link_libraries( testplaylistformat Qt::Test amarokcore amaroklib )

#------------------------ Test PlaylistObserver -----------------------------

set( testplaylistobserver_SRCS
     TestPlaylistObserver.cpp
     ${AMAROK_SOURCE_TREE}/core/playlists/Playlist.cpp
     )
add_executable( testplaylistobserver ${testplaylistobserver_SRCS} )
add_test(NAME testplaylistobserver COMMAND $<TARGET_FILE:testplaylistobserver>)
ecm_mark_as_test(testplaylistobserver)
target_link_libraries( testplaylistobserver KF${KF_MAJOR_VERSION}::ThreadWeaver Qt::Test Qt::Core amarokcore amaroklib )
