include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${GOOGLEMOCK_INCLUDE_DIR}
  )

#------------------------------- TestAggregateMeta -------------------------------

kde4_add_unit_test( testaggregatemeta
                        TestAggregateMeta.cpp
                        ${GOOGLEMOCK_SRCS}
                  )

add_dependencies( testaggregatemeta amarokconfig_h )
add_dependencies( testaggregatemeta amarokcore )
add_dependencies( testaggregatemeta amaroklib)

if(APPLE)
    SET_TARGET_PROPERTIES( testaggregatemeta PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" )
endif(APPLE)

target_link_libraries( testaggregatemeta
    amarokcore
    amaroklib
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${GOOGLEMOCK_LIBRARIES}
)
