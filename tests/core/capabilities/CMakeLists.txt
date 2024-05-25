include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/tests
)

#------------------------ Test ActionsCapability -----------------------------

set( testactionscapability_SRCS TestActionsCapability.cpp )
add_executable( testactionscapability ${testactionscapability_SRCS} )
add_test(NAME testactionscapability COMMAND $<TARGET_FILE:testactionscapability>)
ecm_mark_as_test(testactionscapability)
target_link_libraries( testactionscapability Qt::Gui Qt::Test amarokcore amaroklib )
