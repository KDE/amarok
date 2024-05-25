include_directories(
        ${AMAROK_TEST_TREE}
        ${CMAKE_BINARY_DIR}/tests
        )

set( testamarokurls_SRCS TestAmarokUrls.cpp )
add_executable( testamarokurls ${testamarokurls_SRCS} )
add_test(NAME testamarokurls COMMAND $<TARGET_FILE:testamarokurls>)
ecm_mark_as_test(testamarokurls)
target_link_libraries( testamarokurls amarokcore amaroklib Qt::Test )

