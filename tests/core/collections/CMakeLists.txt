add_subdirectory( support )

include_directories(
  ..
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_SOURCE_TREE}/collection
  ${AMAROK_SOURCE_TREE}/meta
  ${CMAKE_BINARY_DIR}/src
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${GOOGLEMOCK_INCLUDE_DIR}
  )

#------------------------ Test CollectionLocation -----------------------------

set( testcollectionlocation_SRCS
        CollectionLocationTest.cpp
        ${GOOGLEMOCK_SRCS}
    )

kde4_add_unit_test( testcollectionlocation TESTNAME amarok-testcollectionlocation ${testcollectionlocation_SRCS} )

add_dependencies( testcollectionlocation amarokconfig_h )
add_dependencies( testcollectionlocation amarokcore)
add_dependencies( testcollectionlocation amaroklib)

if(APPLE)
    SET_TARGET_PROPERTIES(testcollectionlocation PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

target_link_libraries(testcollectionlocation
                        amarokcore
                        amaroklib
                        ${KDE4_KDECORE_LIBS}
                        ${KDE4_KDEUI_LIBS}
                        ${KDE4_KIO_LIBS}
                        ${QT_QTTEST_LIBRARY}
                        ${GOOGLEMOCK_LIBRARIES})

#------------------------ Test Collection -----------------------------

set( testcollection_SRCS TestCollection.cpp )
kde4_add_unit_test( testcollection ${testcollection_SRCS} )
target_link_libraries( testcollection ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} amarokcore )

#------------------------ Test QueryMaker -----------------------------

set( testquerymaker_SRCS TestQueryMaker.cpp ../../mocks/MockQueryMaker.cpp )
kde4_add_unit_test( testquerymaker ${testquerymaker_SRCS} )
target_link_libraries( testquerymaker ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} amarokcore )
