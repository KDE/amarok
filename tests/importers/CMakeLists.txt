include_directories(
    ${AMAROK_TEST_TREE}
    ${AMAROK_SOURCE_TREE}
    ${AMAROK_UTILITY_TREE}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_BINARY_DIR}/tests
    ${AMAROK_COLLECTION_SUPPORT_DIR}
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
    ${GOOGLEMOCK_INCLUDE_DIR}
)

add_custom_target( importers_files
    ${CMAKE_COMMAND} -E copy_directory "${AMAROK_TEST_TREE}/importers/files" "${CMAKE_BINARY_DIR}/tests/importers_files"
    VERBATIM
)

set( testimporterscommon_LIBS
    amaroklib
    amarokcore
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_QTSQL_LIBRARY}
    ${QT_QTTEST_LIBRARY}
    ${GOOGLEMOCK_LIBRARIES}
)

#------------------------ Test ImporterManager ----------------------------

set( testimportermanager_SRCS
    TestImporterManager.cpp
    ImporterMocks.cpp
)

kde4_add_unit_test( testimportermanager ${testimportermanager_SRCS} )
target_link_libraries( testimportermanager ${testimporterscommon_LIBS} )
