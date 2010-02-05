include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_SOURCE_TREE}/collection/proxycollection
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${GOOGLEMOCK_INCLUDE_DIR}
  )

#------------------------------- TestProxyCollectionMeta -------------------------------

set( testproxycollectionmeta_SRCS
        TestProxyCollectionMeta.cpp
        ${AMAROK_SOURCE_TREE}/collection/Collection.cpp
        ${AMAROK_SOURCE_TREE}/collection/proxycollection/ProxyCollection.cpp
        ${AMAROK_SOURCE_TREE}/collection/proxycollection/ProxyCollectionMeta.cpp
        ${AMAROK_SOURCE_TREE}/meta/Meta.cpp
        ${AMAROK_SOURCE_TREE}/meta/Capability.cpp
        ${AMAROK_SOURCE_TREE}/meta/capabilities/EditCapability.cpp
        ${AMAROK_SOURCE_TREE}/meta/capabilities/OrganiseCapability.cpp
    )

kde4_add_unit_test( testproxycollectionmeta ${testproxycollectionmeta_SRCS} )

add_dependencies( testproxycollectionmeta amarokconfig_h )
add_dependencies( testproxycollectionmeta amaroklib)

if(APPLE)
    SET_TARGET_PROPERTIES(testproxycollectionmeta PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

target_link_libraries(testproxycollectionmeta
    amaroklib
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${GOOGLEMOCK_LIBRARIES})
