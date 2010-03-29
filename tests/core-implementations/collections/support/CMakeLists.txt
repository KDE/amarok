include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_SOURCE_TREE}/core-implementations/collections/support
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${GOOGLEMOCK_INCLUDE_DIR}
  )

#------------------------------- TestMemoryQueryMaker -------------------------------

set( testmemoryquerymaker_SRCS
        TestMemoryQueryMaker.cpp
    )

kde4_add_unit_test( testmemoryquerymaker ${testmemoryquerymaker_SRCS} )

if(APPLE)
    SET_TARGET_PROPERTIES(testmemoryquerymaker PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
add_dependencies( testmemoryquerymaker amaroklib )
target_link_libraries(testmemoryquerymaker
    amaroklib
    ${KDE4_KDECORE_LIBS}
    ${QT_QTGUI_LIBRARY}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${GOOGLEMOCK_LIBRARIES})
