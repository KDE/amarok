include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests
)

include_directories(SYSTEM
  ${GOOGLEMOCK_INCLUDE_DIR}
  )

#------------------------ Test Logger -----------------------------

set( testlogger_SRCS
        TestLogger.cpp
        ${AMAROK_SOURCE_TREE}/core/logger/Logger.cpp
        ${GOOGLEMOCK_SRCS}
    )


add_executable( testlogger
    ${testlogger_SRCS}
    )
add_test(NAME testlogger COMMAND $<TARGET_FILE:testlogger>)
ecm_mark_as_test(testlogger)

target_link_libraries( testlogger
    Qt::Test
    Qt::Core
    Qt::Network
    KF${KF_MAJOR_VERSION}::CoreAddons
    KF${KF_MAJOR_VERSION}::ThreadWeaver
    ${GOOGLEMOCK_LIBRARIES}
    ${CMAKE_DL_LIBS})
