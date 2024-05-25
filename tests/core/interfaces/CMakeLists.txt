include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/tests
)

#------------------------ Test MetaCapability -----------------------------

set( testmetacapability_SRCS TestMetaCapability.cpp )
add_executable( testmetacapability ${testmetacapability_SRCS} )
add_test(NAME testmetacapability COMMAND $<TARGET_FILE:testmetacapability>)
ecm_mark_as_test(testmetacapability)
target_link_libraries( testmetacapability Qt::Gui Qt::Test amarokcore )
