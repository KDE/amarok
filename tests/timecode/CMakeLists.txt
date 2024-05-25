include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests
)
#------------------------ Test TimecodeTrackProvider -----------------------------

set( testtimecodetrackprovider_SRCS
     TestTimecodeTrackProvider.cpp
     ${AMAROK_SOURCE_TREE}/core-impl/meta/timecode/TimecodeTrackProvider.cpp
     ${AMAROK_SOURCE_TREE}/core-impl/meta/timecode/TimecodeMeta.cpp
     ${AMAROK_SOURCE_TREE}/core-impl/capabilities/timecode/TimecodeBoundedPlaybackCapability.cpp
     ${AMAROK_SOURCE_TREE}/core/capabilities/BoundedPlaybackCapability.cpp
    )

add_executable( testtimecodetrackprovider ${testtimecodetrackprovider_SRCS} )
add_test(NAME testtimecodetrackprovider COMMAND $<TARGET_FILE:testtimecodetrackprovider>)
ecm_mark_as_test(testtimecodetrackprovider)

target_link_libraries(testtimecodetrackprovider Qt::Test Qt::Core Qt::Gui amarokcore amaroklib )
