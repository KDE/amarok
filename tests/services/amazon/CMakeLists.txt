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


#------------------------ Test Meta::AmazonAlbum ---------------------------------

set( testamazonalbum_SRCS
TestAmazonAlbum.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonMeta.cpp
 )

kde4_add_unit_test( testamazonalbum ${testamazonalbum_SRCS} )
target_link_libraries( testamazonalbum
${KDE4_KDECORE_LIBS}
${KDE4_KDEUI_LIBS}
${QT_QTTEST_LIBRARY}
${QT_QTCORE_LIBRARY}
amarokcore
amaroklib
amarok_service_amazonstore )


#------------------------ Test Meta::AmazonArtist ---------------------------------

set( testamazonartist_SRCS
TestAmazonArtist.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonMeta.cpp
 )

kde4_add_unit_test( testamazonartist ${testamazonartist_SRCS} )
target_link_libraries( testamazonartist
${KDE4_KDECORE_LIBS}
${KDE4_KDEUI_LIBS}
${QT_QTTEST_LIBRARY}
${QT_QTCORE_LIBRARY}
amarokcore
amaroklib
amarok_service_amazonstore )


#------------------------ Test Meta::AmazonItem ---------------------------------

set( testamazonitem_SRCS
TestAmazonItem.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonMeta.cpp
 )

kde4_add_unit_test( testamazonitem ${testamazonitem_SRCS} )
target_link_libraries( testamazonitem
${KDE4_KDECORE_LIBS}
${KDE4_KDEUI_LIBS}
${QT_QTTEST_LIBRARY}
${QT_QTCORE_LIBRARY}
amarokcore
amaroklib
amarok_service_amazonstore )


#------------------------ Test Meta::AmazonTrack ---------------------------------

set( testamazontrack_SRCS
TestAmazonTrack.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonMeta.cpp
 )

kde4_add_unit_test( testamazontrack ${testamazontrack_SRCS} )
target_link_libraries( testamazontrack
${KDE4_KDECORE_LIBS}
${KDE4_KDEUI_LIBS}
${QT_QTTEST_LIBRARY}
${QT_QTCORE_LIBRARY}
amarokcore
amaroklib
amarok_service_amazonstore )
