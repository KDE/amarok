add_subdirectory( data )

include_directories(
  ${AMAROK_SOURCE_TREE}/context/engines/upcomingevents
  ${AMAROK_SOURCE_TREE}/context/applets/upcomingevents
  ${AMAROK_SOURCE_TREE}/context
  ${AMAROK_TEST_TREE}/context/engines
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
)


set( testupcomingeventsengine_SRCS
    TestUpcomingEventsEngine.cpp
    ${AMAROK_TEST_TREE}/context/engines/ContextView.cpp
    ${AMAROK_TEST_TREE}/context/engines/TestDataEngine.cpp
    ${AMAROK_SOURCE_TREE}/context/ContextObserver.cpp
    ${AMAROK_SOURCE_TREE}/context/engines/upcomingevents/UpcomingEventsEngine.cpp
    ${AMAROK_SOURCE_TREE}/context/applets/upcomingevents/LastFmEvent.cpp
)

kde4_add_unit_test( testupcomingeventsengine ${testupcomingeventsengine_SRCS} )

add_dependencies( testupcomingeventsengine amaroklib)

target_link_libraries( testupcomingeventsengine amaroklib ${KDE4_KDECORE_LIBS} ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})
