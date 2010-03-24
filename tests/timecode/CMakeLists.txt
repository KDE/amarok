#------------------------ Test TimecodeTrackProvider -----------------------------

set( testtimecodetrackprovider_SRCS
     TestTimecodeTrackProvider.cpp
     ${AMAROK_SOURCE_TREE}/core/meta/impl/timecode/TimecodeTrackProvider.cpp
     ${AMAROK_SOURCE_TREE}/core/meta/impl/timecode/TimecodeMeta.cpp
     ${AMAROK_SOURCE_TREE}/core/capabilities/impl/timecode/TimecodeBoundedPlaybackCapability.cpp
     ${AMAROK_SOURCE_TREE}/core/capabilities/BoundedPlaybackCapability.cpp
    )

kde4_add_unit_test( testtimecodetrackprovider ${testtimecodetrackprovider_SRCS} )

target_link_libraries(testtimecodetrackprovider ${QT_QTTEST_LIBRARY} amaroklib )
