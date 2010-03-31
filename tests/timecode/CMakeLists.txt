include_directories( .
  ${AMAROK_TEST_TREE}
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

kde4_add_unit_test( testtimecodetrackprovider ${testtimecodetrackprovider_SRCS} )

target_link_libraries(testtimecodetrackprovider ${QT_QTTEST_LIBRARY} amaroklib )
