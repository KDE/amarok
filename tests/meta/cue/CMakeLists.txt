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

#------------------------ Test CueFileSupport -----------------------------

set( testcuefilesupport_SRCS
        TestCueFileSupport.cpp
        ${AMAROK_SOURCE_TREE}/core/capabilities/BoundedPlaybackCapability.cpp
        ${AMAROK_SOURCE_TREE}/core-implementations/meta/cue/CueFileSupport.cpp
        ${AMAROK_SOURCE_TREE}/core-implementations/meta/timecode/TimecodeMeta.cpp
        ${AMAROK_SOURCE_TREE}/core-implementations/capabilities/timecode/TimecodeBoundedPlaybackCapability.cpp
    )

kde4_add_unit_test( testcuefilesupport ${testcuefilesupport_SRCS} )

add_dependencies( testcuefilesupport amaroklib)

target_link_libraries( testcuefilesupport
    amaroklib
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${GOOGLEMOCK_LIBRARIES}
    ${CMAKE_DL_LIBS})

