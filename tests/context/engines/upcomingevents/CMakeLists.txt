add_subdirectory( data )

include_directories(
  ${AMAROK_SOURCE_TREE}/context/engines/upcomingevents
  ${AMAROK_SOURCE_TREE}/context/applets/upcomingevents
  ${AMAROK_SOURCE_TREE}/context
  ${AMAROK_TEST_TREE}/context/engines
  ${AMAROK_COLLECTION_SUPPORT_DIR}


)


set( testupcomingeventsengine_SRCS
    TestUpcomingEventsEngine.cpp
    ${AMAROK_TEST_TREE}/context/engines/ContextView.cpp
    ${AMAROK_TEST_TREE}/context/engines/TestDataEngine.cpp
    ${AMAROK_SOURCE_TREE}/context/ContextObserver.cpp
    ${AMAROK_SOURCE_TREE}/context/engines/upcomingevents/UpcomingEventsEngine.cpp
    ${AMAROK_SOURCE_TREE}/context/applets/upcomingevents/LastFmEvent.cpp
    ${AMAROK_SOURCE_TREE}/context/applets/upcomingevents/LastFmEventXmlParser.cpp
)

add_executable( testupcomingeventsengine ${testupcomingeventsengine_SRCS} )
add_test(NAME testupcomingeventsengine COMMAND $<TARGET_FILE:testupcomingeventsengine>)
ecm_mark_as_test(testupcomingeventsengine)

add_dependencies( testupcomingeventsengine amaroklib)

target_link_libraries( testupcomingeventsengine amarokcore amaroklib KF${KF_MAJOR_VERSION}::Plasma  KF${KF_MAJOR_VERSION}::KIOCore Qt::Test)
