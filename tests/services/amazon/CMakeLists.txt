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

add_executable( testamazonconfig ${testamazonconfig_SRCS} )
add_test(testamazonconfig testamazonconfig)
ecm_mark_as_test(testamazonconfig)
target_link_libraries( testamazonconfig KF5::KDELibs4Support Qt5::Test )


#------------------------ Test AmazonMetaFactory -------------------------------

set( testamazonmetafactory_SRCS
        TestAmazonMetaFactory.cpp
        ${AMAROK_SOURCE_TREE}/services/amazon/AmazonMeta.cpp
 )

add_executable( testamazonmetafactory ${testamazonmetafactory_SRCS} )
add_test(testamazonmetafactory testamazonmetafactory)
ecm_mark_as_test(testamazonmetafactory)
target_link_libraries( testamazonmetafactory
KF5::KDELibs4Support

Qt5::Test
Qt5::Core
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

add_executable( testamazonshoppingcart ${testamazonshoppingcart_SRCS} )
add_test(testamazonshoppingcart testamazonshoppingcart)
ecm_mark_as_test(testamazonshoppingcart)
target_link_libraries( testamazonshoppingcart
${GOOGLEMOCK_LIBRARIES}
Qt5::Test
KF5::KDELibs4Support
amarokcore )


#------------------------ Test AmazonShoppingCartItem ----------------------

set( testamazonshoppingcartitem_SRCS
TestAmazonShoppingCartItem.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonShoppingCartItem.cpp
 )

add_executable( testamazonshoppingcartitem ${testamazonshoppingcartitem_SRCS} )
add_test(testamazonshoppingcartitem testamazonshoppingcartitem)
ecm_mark_as_test(testamazonshoppingcartitem)
target_link_libraries( testamazonshoppingcartitem KF5::KDELibs4Support Qt5::Test )


#------------------------ Test Meta::AmazonAlbum ---------------------------------

set( testamazonalbum_SRCS
TestAmazonAlbum.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonMeta.cpp
 )

add_executable( testamazonalbum ${testamazonalbum_SRCS} )
add_test(testamazonalbum testamazonalbum)
ecm_mark_as_test(testamazonalbum)
target_link_libraries( testamazonalbum
KF5::KDELibs4Support

Qt5::Test
Qt5::Core
amarokcore
amaroklib )



#------------------------ Test Meta::AmazonArtist ---------------------------------

set( testamazonartist_SRCS
TestAmazonArtist.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonMeta.cpp
 )

add_executable( testamazonartist ${testamazonartist_SRCS} )
add_test(testamazonartist testamazonartist)
ecm_mark_as_test(testamazonartist)
target_link_libraries( testamazonartist
KF5::KDELibs4Support

Qt5::Test
Qt5::Core
amarokcore
amaroklib )


#------------------------ Test Meta::AmazonItem ---------------------------------

set( testamazonitem_SRCS
TestAmazonItem.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonMeta.cpp
 )

add_executable( testamazonitem ${testamazonitem_SRCS} )
add_test(testamazonitem testamazonitem)
ecm_mark_as_test(testamazonitem)
target_link_libraries( testamazonitem
KF5::KDELibs4Support

Qt5::Test
Qt5::Core
amarokcore
amaroklib )


#------------------------ Test Meta::AmazonTrack ---------------------------------

set( testamazontrack_SRCS
TestAmazonTrack.cpp
${AMAROK_SOURCE_TREE}/services/amazon/AmazonMeta.cpp
 )

add_executable( testamazontrack ${testamazontrack_SRCS} )
add_test(testamazontrack testamazontrack)
ecm_mark_as_test(testamazontrack)
target_link_libraries( testamazontrack
KF5::KDELibs4Support

Qt5::Test
Qt5::Core
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

add_executable( testamazonparser ${testamazonparser_SRCS} )
add_test(testamazonparser testamazonparser)
ecm_mark_as_test(testamazonparser)
target_link_libraries( testamazonparser
KF5::KDELibs4Support

KF5::ThreadWeaver
Qt5::Test
Qt5::Core
Qt5::Xml
amarokcore
amaroklib )
