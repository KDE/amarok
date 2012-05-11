include_directories(
        ${AMAROK_TEST_TREE}
        ${CMAKE_BINARY_DIR}/tests
        )

set( testamazonshoppingcartitem_SRCS
TestAmazonShoppingCartItem.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonShoppingCartItem.cpp
 )

kde4_add_unit_test( testamazonshoppingcartitem ${testamazonshoppingcartitem_SRCS} )
target_link_libraries( testamazonshoppingcartitem ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} )
