macro(link_database_test test_target)

    add_definitions(${MYSQL_CFLAGS})

    if(APPLE)
        SET_TARGET_PROPERTIES(${test_target} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    endif(APPLE)

    add_dependencies( ${test_target} amarokconfig_h )
    add_dependencies( ${test_target} amarokcore )
    add_dependencies( ${test_target} amaroklib)

    target_link_libraries(${test_target}
        amarokcore
        amaroklib
        ${KDE4_KDECORE_LIBS}
        ${KDE4_KIO_LIBS}
        ${KDE4_THREADWEAVER_LIBRARIES}
        ${KDE4_KDEUI_LIBS}
        ${QT_QTTEST_LIBRARY}
        ${QT_QTCORE_LIBRARY}
        ${MYSQL_LIBRARIES}
        ${CMAKE_DL_LIBS}
        ${TAGLIB-EXTRAS_LIBRARIES}
        ${TAGLIB_LIBRARIES}
        ${ZLIB_LIBRARIES})

endmacro(link_database_test)


include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  # ${AMAROK_SOURCE_TREE}/core-impl/logger
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/src/dynamic
  ${CMAKE_BINARY_DIR}/tests
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${MYSQL_INCLUDE_DIR}
  )

#------------------------------- DynamicModel Test -------------------------------

set( dynamicmodel_SRCS
        TestDynamicModel.cpp
    )

kde4_add_unit_test( testdynamicmodel TESTNAME testdynamicmodel ${dynamicmodel_SRCS} )
link_database_test( testdynamicmodel )


