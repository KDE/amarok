include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/tests
)

#------------------------ Test TrackForUrlWorker -----------------------------

set( testtrackforurlworker_SRCS TestTrackForUrlWorker.cpp ../../../mocks/MockTrackForUrlWorker.cpp )
add_executable( testtrackforurlworker ${testtrackforurlworker_SRCS} )
add_test(NAME testtrackforurlworker COMMAND $<TARGET_FILE:testtrackforurlworker>)
ecm_mark_as_test(testtrackforurlworker)
target_link_libraries( testtrackforurlworker Qt::Test KF${KF_MAJOR_VERSION}::ThreadWeaver amarokcore amaroklib )
