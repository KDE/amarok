include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  
  
  ${GOOGLEMOCK_INCLUDE_DIR}
  )

#------------------------------- TestAggregateMeta -------------------------------

add_executable( testaggregatemeta
                        TestAggregateMeta.cpp
                        ${GOOGLEMOCK_SRCS}
                  )
add_test(testaggregatemeta testaggregatemeta)
ecm_mark_as_test(testaggregatemeta)

add_dependencies( testaggregatemeta amarokconfig_h )
add_dependencies( testaggregatemeta amarokcore )
add_dependencies( testaggregatemeta amaroklib)

if(APPLE)
    SET_TARGET_PROPERTIES( testaggregatemeta PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" )
endif(APPLE)

target_link_libraries( testaggregatemeta
    amarokcore
    amaroklib
    KF5::KDELibs4Support
    
    Qt5::Test
    Qt5::Core
    ${GOOGLEMOCK_LIBRARIES}
)
