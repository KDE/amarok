include_directories(
  ${AMAROK_TEST_TREE}
  ${AMAROK_SOURCE_TREE}
  ${AMAROK_SOURCE_TREE}/core-impl/collections/support
  ${AMAROK_UTILITY_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}/tests
  ${AMAROK_COLLECTION_SUPPORT_DIR}
)

include_directories(SYSTEM
  ${GOOGLEMOCK_INCLUDE_DIR}
  )

#------------------------------- TestMemoryQueryMaker -------------------------------

set( testmemoryquerymaker_SRCS
        TestMemoryQueryMaker.cpp
        ${GOOGLEMOCK_SRCS}
    )

add_executable( testmemoryquerymaker ${testmemoryquerymaker_SRCS} )
add_test(NAME testmemoryquerymaker COMMAND $<TARGET_FILE:testmemoryquerymaker>)
ecm_mark_as_test(testmemoryquerymaker)

if(APPLE)
    set_target_properties(testmemoryquerymaker PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()
add_dependencies( testmemoryquerymaker amarokcore )
add_dependencies( testmemoryquerymaker amaroklib )
target_link_libraries(testmemoryquerymaker
    amarokcore
    amaroklib
    Qt::Gui
    KF${KF_MAJOR_VERSION}::ThreadWeaver
    Qt::Test
    Qt::Core
    ${GOOGLEMOCK_LIBRARIES}
)

#-------------------------------- Test ArtistHelper -----------------------

set( testartisthelper_SRCS
        TestArtistHelper.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/ArtistHelper.cpp
    )


add_executable( testartisthelper ${testartisthelper_SRCS} )
add_test(NAME testartisthelper COMMAND $<TARGET_FILE:testartisthelper>)
ecm_mark_as_test(testartisthelper)

    if(APPLE)
        set_target_properties(testartisthelper PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    endif()

target_link_libraries( testartisthelper
    Qt::Test
    Qt::Core
    KF${KF_MAJOR_VERSION}::I18n
)

