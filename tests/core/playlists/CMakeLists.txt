include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/tests
)

#------------------------ Test PlaylistFormat -----------------------------

set( testplaylistformat_SRCS TestPlaylistFormat.cpp )
add_executable( testplaylistformat ${testplaylistformat_SRCS} )
add_test(testplaylistformat testplaylistformat)
ecm_mark_as_test(testplaylistformat)
target_link_libraries( testplaylistformat KF5::KDELibs4Support Qt5::Test amarokcore amaroklib )

#------------------------ Test PlaylistObserver -----------------------------

set( testplaylistobserver_SRCS
     TestPlaylistObserver.cpp
     ${AMAROK_SOURCE_TREE}/core/playlists/Playlist.cpp
     )
add_executable( testplaylistobserver ${testplaylistobserver_SRCS} )
add_test(testplaylistobserver testplaylistobserver)
ecm_mark_as_test(testplaylistobserver)
target_link_libraries( testplaylistobserver KF5::ThreadWeaver KF5::KDELibs4Support Qt5::Test Qt5::Core amarokcore amaroklib )
