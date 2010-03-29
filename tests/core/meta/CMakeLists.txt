include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/tests
)

#------------------------ Test MetaTrack -----------------------------

set( testmetatrack_SRCS TestMetaTrack.cpp )
kde4_add_unit_test( testmetatrack ${testmetatrack_SRCS} )
target_link_libraries( testmetatrack ${QT_QTTEST_LIBRARY} amaroklib )

