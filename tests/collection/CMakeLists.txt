
include_directories(
  ..
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_SOURCE_TREE}/collection
  ${AMAROK_SOURCE_TREE}/meta
  ${CMAKE_BINARY_DIR}/src
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
  )

set( testcollectionlocation_SRCS
        CollectionLocationTest.cpp
        ${AMAROK_SOURCE_TREE}/collection/CollectionLocation.cpp
        ${AMAROK_SOURCE_TREE}/meta/Capability.cpp
        ${AMAROK_SOURCE_TREE}/meta/Meta.cpp
	${AMAROK_SOURCE_TREE}/meta/MetaUtility.cpp
	${AMAROK_SOURCE_TREE}/Amarok.cpp
	${AMAROK_SOURCE_TREE}/meta/PlaylistFileSupport.cpp
    )

kde4_add_unit_test( testcollectionlocation TESTNAME amarok-testcollectionlocation ${testcollectionlocation_SRCS} )

add_dependencies( testcollectionlocation amarokconfig_h )
add_dependencies( testcollectionlocation amaroklib)

if(APPLE)
    SET_TARGET_PROPERTIES(testcollectionlocation PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

target_link_libraries(testcollectionlocation amaroklib ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})


add_subdirectory( proxycollection )
add_subdirectory( sqlcollection )
add_subdirectory( support )
