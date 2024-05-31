add_subdirectory( support )

include_directories(
  ..
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_SOURCE_TREE}/collection
  ${AMAROK_SOURCE_TREE}/meta
  ${CMAKE_BINARY_DIR}/src
  ${AMAROK_COLLECTION_SUPPORT_DIR}
)
include_directories(SYSTEM
  ${GOOGLEMOCK_INCLUDE_DIR}
  )

#------------------------ Test CollectionLocation -----------------------------

set( testcollectionlocation_SRCS
        CollectionLocationTest.cpp
        ${GOOGLEMOCK_SRCS}
    )

add_executable( testcollectionlocation ${testcollectionlocation_SRCS} )
add_test(NAME testcollectionlocation COMMAND $<TARGET_FILE:testcollectionlocation>)
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
                        KF${KF_MAJOR_VERSION}::KIOCore
                        Qt::Test
                        ${GOOGLEMOCK_LIBRARIES})

#------------------------ Test Collection -----------------------------

set( testcollection_SRCS TestCollection.cpp )
add_executable( testcollection ${testcollection_SRCS} )
add_test(NAME testcollection COMMAND $<TARGET_FILE:testcollection>)
ecm_mark_as_test(testcollection)
target_link_libraries( testcollection  Qt::Test amarokcore )

#------------------------ Test QueryMaker -----------------------------

set( testquerymaker_SRCS TestQueryMaker.cpp ../../mocks/MockQueryMaker.cpp )
add_executable( testquerymaker ${testquerymaker_SRCS} )
add_test(NAME testquerymaker COMMAND $<TARGET_FILE:testquerymaker>)
ecm_mark_as_test(testquerymaker)
target_link_libraries( testquerymaker Qt::Gui Qt::Test amarokcore )
