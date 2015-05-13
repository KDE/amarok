macro(add_database_test test_target test_sources)
    if(APPLE)
    #one cannot link to plugins on OS X. As a workaround, add anything relevant that goes into the mysqle plugin to each test
        set( test_sources_internal
                ${test_sources} )
    endif(APPLE)

    kde4_add_unit_test( ${test_target} TESTNAME ${test_target}
        ${test_sources}
        ${test_sources_internal}
        ${GOOGLEMOCK_SRCS}
        )

    if(APPLE)
        SET_TARGET_PROPERTIES(${test_target} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    endif(APPLE)

    add_dependencies( ${test_target} amarokconfig_h )
    add_dependencies( ${test_target} amarokcore )
    add_dependencies( ${test_target} amaroklib)
    add_dependencies( ${test_target} amarokshared)

    target_link_libraries(${test_target}
        amarokcore
        amaroklib
        amarokshared
        KF5::KDELibs4Support
        KF5::KIOCore
        KF5::ThreadWeaver
        
        Qt5::Test
        Qt5::Core
        ${CMAKE_DL_LIBS}
        ${ZLIB_LIBRARIES}
        ${GOOGLEMOCK_LIBRARIES})

endmacro(add_database_test)


include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_SOURCE_TREE}/core-impl/logger
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  
  
  )

#-------------------------------- Test SqlScanManager -----------------------

set( testgenericscanmanager_SRCS
        TestGenericScanManager.cpp
    )

add_database_test( testgenericscanmanager "${testgenericscanmanager_SRCS}" )

