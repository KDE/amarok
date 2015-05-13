include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests
  
  
  ${GOOGLEMOCK_INCLUDE_DIR}
  )

#------------------------ Test ProxyLogger -----------------------------

set( testproxylogger_SRCS
        TestProxyLogger.cpp
        ${AMAROK_SOURCE_TREE}/core/interfaces/Logger.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/logger/ProxyLogger.cpp
        ${GOOGLEMOCK_SRCS}
    )


add_executable( testproxylogger
add_test(testproxylogger testproxylogger)
ecm_mark_as_test(testproxylogger)
    ${testproxylogger_SRCS}
    )

target_link_libraries( testproxylogger
    KF5::KDELibs4Support
    Qt5::Test
    Qt5::Core
    KF5::ThreadWeaver
    ${GOOGLEMOCK_LIBRARIES}
    ${CMAKE_DL_LIBS})
