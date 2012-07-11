include_directories( .
  ${AMAROK_TEST_TREE}
  ${CMAKE_BINARY_DIR}/tests
)

#------------------------ Test TrackForUrlWorker -----------------------------

set( testtrackforurlworker_SRCS TestTrackForUrlWorker.cpp ../../../mocks/MockTrackForUrlWorker.cpp )
kde4_add_unit_test( testtrackforurlworker ${testtrackforurlworker_SRCS} )
target_link_libraries( testtrackforurlworker ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} ${KDE4_THREADWEAVER_LIBRARIES} amarokcore amaroklib )
