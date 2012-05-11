include_directories(
        ${AMAROK_TEST_TREE}
        ${CMAKE_BINARY_DIR}/tests
        )

#------------------------ Test AmazonConfig -------------------------------

set( testamazonconfig_SRCS
TestAmazonConfig.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonConfig.cpp
 )

kde4_add_unit_test( testamazonconfig ${testamazonconfig_SRCS} )
target_link_libraries( testamazonconfig ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} )


#------------------------ Test AmazonShoppingCart --------------------------

set( testamazonshoppingcart_SRCS
TestAmazonShoppingCart.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonConfig.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonShoppingCart.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonShoppingCartItem.cpp
 )

kde4_add_unit_test( testamazonshoppingcart ${testamazonshoppingcart_SRCS} )
target_link_libraries( testamazonshoppingcart ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} )


#------------------------ Test AmazonShoppingCartItem ----------------------

set( testamazonshoppingcartitem_SRCS
TestAmazonShoppingCartItem.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonShoppingCartItem.cpp
 )

kde4_add_unit_test( testamazonshoppingcartitem ${testamazonshoppingcartitem_SRCS} )
target_link_libraries( testamazonshoppingcartitem ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} )
