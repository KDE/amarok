#------------------------ Test TrackLoader -----------------------------

add_executable( testtrackloader TestTrackLoader.cpp )
add_test(testtrackloader testtrackloader)
ecm_mark_as_test(testtrackloader)

target_link_libraries( testtrackloader
    amarokcore
    amaroklib
    KF5::KDELibs4Support
    Qt5::Test
    Qt5::Core
    KF5::ThreadWeaver
)
