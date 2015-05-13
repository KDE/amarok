include_directories(
        ${AMAROK_TEST_TREE}
        ${CMAKE_BINARY_DIR}/tests
        )

set( testamarokurls_SRCS TestAmarokUrls.cpp )
add_executable( testamarokurls ${testamarokurls_SRCS} )
add_test(testamarokurls testamarokurls)
ecm_mark_as_test(testamarokurls)
target_link_libraries( testamarokurls amarokcore amaroklib KF5::KDELibs4Support Qt5::Test )

