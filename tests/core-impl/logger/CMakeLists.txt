include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${GOOGLEMOCK_INCLUDE_DIR}
  )

#------------------------ Test ProxyLogger -----------------------------

set( testproxylogger_SRCS
    TestProxyLogger.cpp
    ${AMAROK_SOURCE_TREE}/core/interfaces/Logger.cpp
    ${AMAROK_SOURCE_TREE}/core-impl/logger/ProxyLogger.cpp )


kde4_add_unit_test( testproxylogger
    ${testproxylogger_SRCS}
    )

target_link_libraries( testproxylogger
    ${KDE4_KDECORE_LIBS}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${GOOGLEMOCK_LIBRARIES}
    ${CMAKE_DL_LIBS})
