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

#------------------------ Test ITunesImporter -----------------------------

set( testitunesimporter_SRCS TestITunesImporter.cpp TestImportersCommon.cpp )

kde4_add_unit_test( testitunesimporter ${testitunesimporter_SRCS} )

add_dependencies( testitunesimporter amarokcore )
add_dependencies( testitunesimporter amaroklib )
add_dependencies( testitunesimporter importers_files )

target_link_libraries( testitunesimporter
    amarokcore
    amaroklib
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${CMAKE_DL_LIBS}
)

