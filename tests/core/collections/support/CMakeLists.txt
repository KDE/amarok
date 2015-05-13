include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/tests
)

#------------------------ Test TrackForUrlWorker -----------------------------

set( testtrackforurlworker_SRCS TestTrackForUrlWorker.cpp ../../../mocks/MockTrackForUrlWorker.cpp )
add_executable( testtrackforurlworker ${testtrackforurlworker_SRCS} )
add_test(testtrackforurlworker testtrackforurlworker)
ecm_mark_as_test(testtrackforurlworker)
target_link_libraries( testtrackforurlworker KF5::KDELibs4Support Qt5::Test KF5::ThreadWeaver amarokcore amaroklib )
