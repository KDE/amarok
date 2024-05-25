set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set( AMAROK_SOURCE_TREE ${CMAKE_SOURCE_DIR}/src )
set( AMAROK_TEST_TREE ${CMAKE_SOURCE_DIR}/tests )
set( AMAROK_UTILITY_TREE ${CMAKE_SOURCE_DIR}/utilities )
set( AMAROK_UTILITIES_DIR ${CMAKE_BINARY_DIR}/bin )
set( STRESS_TEST_TRACK_COUNT 20000 )

configure_file(config-amarok-test.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-amarok-test.h )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
)
include_directories(SYSTEM
    ${GOOGLEMOCK_INCLUDE_DIR} )

add_subdirectory( amarokurls )
add_subdirectory( browsers )
add_subdirectory( context )
add_subdirectory( core )
add_subdirectory( core-impl )
add_subdirectory( dynamic )
if( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
add_subdirectory( importers )
endif()
add_subdirectory( playlist )
add_subdirectory( playlistmanager )
add_subdirectory( timecode )
add_subdirectory( scanner )
add_subdirectory( synchronization )

#------------------------ Test Amarok namespace -----------------------------

set( testamarok_SRCS
        TestAmarok.cpp
        ${AMAROK_SOURCE_TREE}/core/support/Amarok.cpp
    )

add_executable( testamarok ${testamarok_SRCS} )
add_test(NAME testamarok COMMAND $<TARGET_FILE:testamarok>)
ecm_mark_as_test(testamarok)

target_link_libraries( testamarok
    KF5::KIOCore
    Qt::Test
    amarokcore
    amaroklib
    )

#------------------------ Test CaseConverter -----------------------------

set( testcaseconverter_SRCS
        TestCaseConverter.cpp
        ${AMAROK_SOURCE_TREE}/CaseConverter.cpp
    )

add_executable( testcaseconverter  ${testcaseconverter_SRCS} )
add_test(NAME testcaseconverter COMMAND $<TARGET_FILE:testcaseconverter>)
ecm_mark_as_test(testcaseconverter)

target_link_libraries(testcaseconverter Qt::Test amarokcore)

#------------------------ Test Debug -----------------------------

add_definitions(-DDEBUG_OVERRIDE_ELAPSED_TIME=4.9)
set( testdebug_SRCS
     TestDebug.cpp
     ${AMAROK_SOURCE_TREE}/core/support/Debug.cpp
     )

add_executable( testdebug ${testdebug_SRCS} )
add_test(NAME testdebug COMMAND $<TARGET_FILE:testdebug>)
ecm_mark_as_test(testdebug)
target_link_libraries(testdebug Qt::Test amarokcore )

#------------------------ Test EngineController -----------------------------

set( testenginecontroller_SRCS TestEngineController.cpp )
add_executable( testenginecontroller ${testenginecontroller_SRCS} )
add_test(NAME testenginecontroller COMMAND $<TARGET_FILE:testenginecontroller>)
ecm_mark_as_test(testenginecontroller)
target_link_libraries( testenginecontroller KF5::ThreadWeaver Qt::Test amaroklib amarokcore )

#------------------------ Test Expression -----------------------------

set( testexpression_SRCS
        TestExpression.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/Expression.cpp
    )

add_executable( testexpression  ${testexpression_SRCS} )
add_test(NAME testexpression COMMAND $<TARGET_FILE:testexpression>)
ecm_mark_as_test(testexpression)

target_link_libraries(testexpression Qt::Test)

#------------------------ Test QStringx -----------------------------

set( testqstringx_SRCS
        TestQStringx.cpp
        ${AMAROK_SOURCE_TREE}/QStringx.cpp
    )

add_executable( testqstringx  ${testqstringx_SRCS} )
add_test(NAME testqstringx COMMAND $<TARGET_FILE:testqstringx>)
ecm_mark_as_test(testqstringx)

target_link_libraries(testqstringx Qt::Test)

#------------------------ Test TagGuesser -----------------------------

set( testtagguesser_SRCS
        TestTagGuesser.cpp
        ${CMAKE_SOURCE_DIR}/shared/TagsFromFileNameGuesser.cpp
        ${AMAROK_SOURCE_TREE}/dialogs/TagGuesser.cpp
    )

add_executable( testtagguesser  ${testtagguesser_SRCS} )
add_test(NAME testtagguesser COMMAND $<TARGET_FILE:testtagguesser>)
ecm_mark_as_test(testtagguesser)

target_link_libraries(testtagguesser Qt::Test amarokcore)

#------------------------ Test TrackOrganizer -----------------------------

set( testtrackorganizer_SRCS
        TestTrackOrganizer.cpp
        ${AMAROK_SOURCE_TREE}/dialogs/TrackOrganizer.cpp
        ${AMAROK_SOURCE_TREE}/core/meta/Meta.cpp
        ${AMAROK_SOURCE_TREE}/QStringx.cpp
        ${GOOGLEMOCK_SRCS}
    )

add_executable( testtrackorganizer  ${testtrackorganizer_SRCS} )
add_test(NAME testtrackorganizer COMMAND $<TARGET_FILE:testtrackorganizer>)
ecm_mark_as_test(testtrackorganizer)

# Since Google recommends not to distribute a pre-compiled gtest library
# we have to build it our own
if(GOOGLEMOCK_GTEST_SOURCES)
    add_subdirectory( ${GOOGLEMOCK_GTEST_SOURCES} gtest )
    add_dependencies( testtrackorganizer gtest )
endif()

add_dependencies( testtrackorganizer amarokcore )
add_dependencies( testtrackorganizer amaroklib )

target_link_libraries( testtrackorganizer amarokcore amaroklib KF5::Solid Qt::Test Qt::Core Qt::Gui ${GOOGLEMOCK_LIBRARIES})
