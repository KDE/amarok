include_directories(
        ${Amarok_SOURCE_DIR}/src/core-impl/collections
        ${Amarok_SOURCE_DIR}/src/services
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


#------------------------ Test AmazonMetaFactory -------------------------------

set( testamazonmetafactory_SRCS
        TestAmazonMetaFactory.cpp
        ${AMAROK_SOURCE_TREE}/services/amazon/AmazonMeta.cpp
 )

kde4_add_unit_test( testamazonmetafactory ${testamazonmetafactory_SRCS} )
target_link_libraries( testamazonmetafactory
${KDE4_KDECORE_LIBS}
${KDE4_KDEUI_LIBS}
${QT_QTTEST_LIBRARY}
${QT_QTCORE_LIBRARY}
amarokcore
amaroklib )



#------------------------ Test AmazonShoppingCart --------------------------

set( testamazonshoppingcart_SRCS
        TestAmazonShoppingCart.cpp
        ${AMAROK_SOURCE_TREE}/services/amazon/AmazonConfig.cpp
        ${AMAROK_SOURCE_TREE}/services/amazon/AmazonShoppingCart.cpp
        ${AMAROK_SOURCE_TREE}/services/amazon/AmazonShoppingCartItem.cpp
        ${GOOGLEMOCK_SRCS}
 )

kde4_add_unit_test( testamazonshoppingcart ${testamazonshoppingcart_SRCS} )
target_link_libraries( testamazonshoppingcart
${GOOGLEMOCK_LIBRARIES}
${QT_QTTEST_LIBRARY}
${KDE4_KDECORE_LIBS}
amarokcore )


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
amaroklib )



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
amaroklib )


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
amaroklib )


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
amaroklib )


#------------------------ Test AmazonParser --------------------------------------

set( testamazonparser_SRCS
TestAmazonParser.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonCollection.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonConfig.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonMeta.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonParser.cpp
 )

kde4_add_unit_test( testamazonparser ${testamazonparser_SRCS} )
target_link_libraries( testamazonparser
${KDE4_KDECORE_LIBS}
${KDE4_KDEUI_LIBS}
${KDE4_THREADWEAVER_LIBRARIES}
${QT_QTTEST_LIBRARY}
${QT_QTCORE_LIBRARY}
${QT_QTXML_LIBRARY}
amarokcore
amaroklib )
