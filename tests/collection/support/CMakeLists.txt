include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_SOURCE_TREE}/collection/support
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
        ${AMAROK_SOURCE_TREE}/collection/support/MemoryQueryMaker.cpp
        ${AMAROK_SOURCE_TREE}/collection/support/MemoryCustomValue.cpp
        ${AMAROK_SOURCE_TREE}/collection/support/MemoryFilter.cpp
        ${AMAROK_SOURCE_TREE}/collection/support/MemoryMatcher.cpp
        ${AMAROK_SOURCE_TREE}/collection/support/MemoryQueryMakerHelper.cpp
        ${AMAROK_SOURCE_TREE}/collection/QueryMaker.cpp
        ${AMAROK_SOURCE_TREE}/meta/Meta.cpp
    )

kde4_add_unit_test( testmemoryquerymaker ${testmemoryquerymaker_SRCS} )

if(APPLE)
    SET_TARGET_PROPERTIES(testmemoryquerymaker PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

target_link_libraries(testmemoryquerymaker
    ${KDE4_KDECORE_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${GOOGLEMOCK_LIBRARIES})
