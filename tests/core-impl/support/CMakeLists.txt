#------------------------ Test TrackLoader -----------------------------

add_executable( testtrackloader TestTrackLoader.cpp )
add_test(NAME testtrackloader COMMAND $<TARGET_FILE:testtrackloader>)
ecm_mark_as_test(testtrackloader)

target_link_libraries( testtrackloader
    amarokcore
    amaroklib
    Qt5::Test
    Qt5::Core
    KF5::ThreadWeaver
)
