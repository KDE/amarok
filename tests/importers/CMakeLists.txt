include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
)

add_custom_target( importers_files
    ${CMAKE_COMMAND} -E copy_directory "${AMAROK_TEST_TREE}/importers/files" "${CMAKE_BINARY_DIR}/tests/importers_files"
    VERBATIM
)

set( testimporterscommon_SRCS
    TestImportersCommon.cpp
    ${AMAROK_SOURCE_TREE}/databaseimporter/DatabaseImporter.cpp
    ${AMAROK_SOURCE_TREE}/databaseimporter/amarok14/FastForwardImporter.cpp
    ${AMAROK_SOURCE_TREE}/databaseimporter/amarok14/FastForwardImporterConfig.cpp
    ${AMAROK_SOURCE_TREE}/databaseimporter/amarok14/FastForwardWorker.cpp
    ${AMAROK_SOURCE_TREE}/databaseimporter/itunes/ITunesImporter.cpp
    ${AMAROK_SOURCE_TREE}/databaseimporter/itunes/ITunesImporterConfig.cpp
    ${AMAROK_SOURCE_TREE}/databaseimporter/itunes/ITunesImporterWorker.cpp
    ${AMAROK_SOURCE_TREE}/databaseimporter/sqlbatch/SqlBatchImporter.cpp
    ${AMAROK_SOURCE_TREE}/databaseimporter/sqlbatch/SqlBatchImporterConfig.cpp
)

set( testimporterscommon_LIBS
    amaroklib
    amarokcore
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_QTSQL_LIBRARY}
    ${QT_QTTEST_LIBRARY}
)

#------------------------ Test ITunesImporter -----------------------------

set( testitunesimporter_SRCS
    TestITunesImporter.cpp
    ${testimporterscommon_SRCS}
)

kde4_add_unit_test( testitunesimporter ${testitunesimporter_SRCS} )
add_dependencies( testitunesimporter amarokcore )
add_dependencies( testitunesimporter amaroklib )
add_dependencies( testitunesimporter importers_files )
target_link_libraries( testitunesimporter ${testimporterscommon_LIBS} )

#------------------------ Test FastForwardImporter-------------------------

set( testfastforwardimporter_SRCS
    TestFastForwardImporter.cpp
    ${testimporterscommon_SRCS}
)

kde4_add_unit_test( testfastforwardimporter ${testfastforwardimporter_SRCS} )
add_dependencies( testfastforwardimporter amarokcore )
add_dependencies( testfastforwardimporter amaroklib )
add_dependencies( testfastforwardimporter importers_files )
target_link_libraries( testfastforwardimporter ${testimporterscommon_LIBS} )
