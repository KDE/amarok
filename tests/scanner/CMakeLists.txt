macro(add_database_test test_target test_sources)
    if(APPLE)
    #one cannot link to plugins on OS X. As a workaround, add anything relevant that goes into the mysqle plugin to each test
        set( test_sources_internal
                ${test_sources} )
    endif()

    add_executable( ${test_target}
        ${test_sources}
        ${test_sources_internal}
        ${GOOGLEMOCK_SRCS}
        )
    add_test(NAME ${test_target} COMMAND $<TARGET_FILE:${test_target}>)
    ecm_mark_as_test(${test_target})

    if(APPLE)
        set_target_properties(${test_target} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    endif()

    add_dependencies( ${test_target} amarokconfig_h )
    add_dependencies( ${test_target} amarokcore )
    add_dependencies( ${test_target} amaroklib)
    add_dependencies( ${test_target} amarokshared)

    target_link_libraries(${test_target}
        amarokcore
        amaroklib
        amarokshared
        KF${KF_MAJOR_VERSION}::KIOCore
        KF${KF_MAJOR_VERSION}::ThreadWeaver
        Qt::Test
        Qt::Core
        ${CMAKE_DL_LIBS}
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

