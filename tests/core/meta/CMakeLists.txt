include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/tests
)

#------------------------ Test MetaTrack -----------------------------

set( testmetatrack_SRCS TestMetaTrack.cpp )
kde4_add_unit_test( testmetatrack ${testmetatrack_SRCS} )
target_link_libraries( testmetatrack ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} amarokcore amaroklib )

#------------------------ Test MetaCapability -----------------------------

set( testmetacapability_SRCS TestMetaCapability.cpp )
kde4_add_unit_test( testmetacapability ${testmetacapability_SRCS} )
target_link_libraries( testmetacapability ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} amarokcore )
