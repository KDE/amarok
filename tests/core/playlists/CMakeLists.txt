include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/tests
)

#------------------------ Test PlaylistFormat -----------------------------

set( testplaylistformat_SRCS TestPlaylistFormat.cpp )
kde4_add_unit_test( testplaylistformat ${testplaylistformat_SRCS} )
target_link_libraries( testplaylistformat ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} amarokcore amaroklib )

#------------------------ Test PlaylistObserver -----------------------------

set( testplaylistobserver_SRCS
     TestPlaylistObserver.cpp
     ${AMAROK_SOURCE_TREE}/core/playlists/Playlist.cpp
     )
kde4_add_unit_test( testplaylistobserver ${testplaylistobserver_SRCS} )
target_link_libraries( testplaylistobserver ${KDE4_THREADWEAVER_LIBRARIES} ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} amarokcore amaroklib )
