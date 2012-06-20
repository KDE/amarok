include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/tests
)

#------------------------ Test PlaylistFormat -----------------------------

set( testplaylistformat_SRCS TestPlaylistFormat.cpp )
kde4_add_unit_test( testplaylistformat ${testplaylistformat_SRCS} )
target_link_libraries( testplaylistformat ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} amarokcore amaroklib )
