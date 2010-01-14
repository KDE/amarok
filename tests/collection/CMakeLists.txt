
include_directories(
  ..
  ${AMAROK_SOURCE_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
  )

set( testcollectionlocation_SRCS
        CollectionLocationTest.cpp
        ${AMAROK_SOURCE_TREE}/collection/CollectionLocation.cpp
        ../mocks/MetaMock.cpp
        ${AMAROK_SOURCE_TREE}/meta/Capability.cpp
        ${AMAROK_SOURCE_TREE}/meta/Meta.cpp
    )

kde4_add_unit_test( testcollectionlocation TESTNAME amarok-testcollectionlocation ${testcollectionlocation_SRCS} )

if(APPLE)
    SET_TARGET_PROPERTIES(testcollectionlocation PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

target_link_libraries(testcollectionlocation ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})


add_subdirectory( proxycollection )
add_subdirectory( sqlcollection )
add_subdirectory( support )
