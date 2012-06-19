include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/tests
)

#------------------------ Test ActionsCapability -----------------------------

set( testactionscapability_SRCS TestActionsCapability.cpp )
kde4_add_unit_test( testactionscapability ${testactionscapability_SRCS} )
target_link_libraries( testactionscapability ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} amarokcore amaroklib )
