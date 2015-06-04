add_subdirectory( support )

include_directories(
  ..
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_SOURCE_TREE}/collection
  ${AMAROK_SOURCE_TREE}/meta
  ${CMAKE_BINARY_DIR}/src
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  ${GOOGLEMOCK_INCLUDE_DIR}
  )

#------------------------ Test CollectionLocation -----------------------------

set( testcollectionlocation_SRCS
        CollectionLocationTest.cpp
        ${GOOGLEMOCK_SRCS}
    )

add_executable( testcollectionlocation ${testcollectionlocation_SRCS} )
add_test(testcollectionlocation testcollectionlocation)
ecm_mark_as_test(testcollectionlocation)

add_dependencies( testcollectionlocation amarokconfig_h )
add_dependencies( testcollectionlocation amarokcore)
add_dependencies( testcollectionlocation amaroklib)

if(APPLE)
    set_target_properties(testcollectionlocation PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

target_link_libraries(testcollectionlocation
                        amarokcore
                        amaroklib
                        KF5::KDELibs4Support
                        
                        KF5::KIOCore
                        Qt5::Test
                        ${GOOGLEMOCK_LIBRARIES})

#------------------------ Test Collection -----------------------------

set( testcollection_SRCS TestCollection.cpp )
add_executable( testcollection ${testcollection_SRCS} )
add_test(testcollection testcollection)
ecm_mark_as_test(testcollection)
target_link_libraries( testcollection KF5::KDELibs4Support  Qt5::Test amarokcore )

#------------------------ Test QueryMaker -----------------------------

set( testquerymaker_SRCS TestQueryMaker.cpp ../../mocks/MockQueryMaker.cpp )
add_executable( testquerymaker ${testquerymaker_SRCS} )
add_test(testquerymaker testquerymaker)
ecm_mark_as_test(testquerymaker)
target_link_libraries( testquerymaker KF5::KDELibs4Support Qt5::Gui Qt5::Test amarokcore )
