macro(link_database_test test_target)
    if(APPLE)
        set_target_properties(${test_target} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    endif()

    add_dependencies( ${test_target} amarokconfig_h )
    add_dependencies( ${test_target} amarokcore )
    add_dependencies( ${test_target} amaroklib)

    target_link_libraries(${test_target}
        amarokcore
        amaroklib
        KF5::KIOCore
        KF5::ThreadWeaver
        Qt::Test
        Qt::Core
        ${MYSQL_LIBRARIES}
        ${CMAKE_DL_LIBS}
#        ${TAGLIB-EXTRAS_LIBRARIES}
#        ${TAGLIB_LIBRARIES}
    )

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
)

include_directories(SYSTEM
  ${MYSQL_INCLUDE_DIR}
  )

#------------------------------- DynamicModel Test -------------------------------

set( dynamicmodel_SRCS
        TestDynamicModel.cpp
    )

add_executable( testdynamicmodel ${dynamicmodel_SRCS} )
add_test(NAME testdynamicmodel COMMAND $<TARGET_FILE:testdynamicmodel>)
ecm_mark_as_test(testdynamicmodel)
link_database_test( testdynamicmodel )

#------------------------------- TrackSet Test -------------------------------

set( trackset_SRCS
        TestTrackSet.cpp
    )

add_executable( testtrackset ${trackset_SRCS} )
add_test(NAME testtrackset COMMAND $<TARGET_FILE:testtrackset>)
ecm_mark_as_test(testtrackset)
link_database_test( testtrackset )


