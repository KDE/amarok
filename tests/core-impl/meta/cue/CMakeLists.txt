include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests
  ${AMAROK_COLLECTION_SUPPORT_DIR}
)

include_directories(SYSTEM
  ${GOOGLEMOCK_INCLUDE_DIR}
  )

#------------------------ Test CueFileSupport -----------------------------

set( testcuefilesupport_SRCS
        TestCueFileSupport.cpp
        ${AMAROK_SOURCE_TREE}/core/capabilities/BoundedPlaybackCapability.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/meta/cue/CueFileSupport.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/meta/timecode/TimecodeMeta.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/capabilities/timecode/TimecodeBoundedPlaybackCapability.cpp
        ${GOOGLEMOCK_SRCS}
    )

add_executable( testcuefilesupport ${testcuefilesupport_SRCS} )
add_test(NAME testcuefilesupport COMMAND $<TARGET_FILE:testcuefilesupport>)
ecm_mark_as_test(testcuefilesupport)

add_dependencies( testcuefilesupport amarokcore )
add_dependencies( testcuefilesupport amaroklib)

target_link_libraries( testcuefilesupport
    amarokcore
    amaroklib
    Qt::Test
    Qt::Core
    ${GOOGLEMOCK_LIBRARIES}
    ${CMAKE_DL_LIBS})

