set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

find_package(Googlemock REQUIRED)
macro_log_feature( GOOGLEMOCK_FOUND "gmock" "Used in Amarok's tests." "http://code.google.com/p/googlemock/" TRUE "1.4" "" )

set( AMAROK_SOURCE_TREE ${CMAKE_SOURCE_DIR}/src )
set( AMAROK_TEST_TREE ${CMAKE_SOURCE_DIR}/tests )
set( AMAROK_UTILITY_TREE ${CMAKE_SOURCE_DIR}/utilities )
set( AMAROK_UTILITIES_DIR ${CMAKE_BINARY_DIR}/utilities )
set( STRESS_TEST_TRACK_COUNT 20000 )

configure_file(config-amarok-test.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-amarok-test.h )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${GOOGLEMOCK_INCLUDE_DIR} )

add_subdirectory( amarokurls )
add_subdirectory( browsers )
add_subdirectory( context )
add_subdirectory( core )
add_subdirectory( core-impl )
add_subdirectory( dynamic )
if(LINUX)
add_subdirectory( importers )
endif(LINUX)
add_subdirectory( playlist )
add_subdirectory( playlistmanager )
add_subdirectory( timecode )
add_subdirectory( scanner )
add_subdirectory( services )
add_subdirectory( synchronization )

#------------------------ Test Amarok namespace -----------------------------

set( testamarok_SRCS
        TestAmarok.cpp
        ${AMAROK_SOURCE_TREE}/core/support/Amarok.cpp
    )

kde4_add_unit_test( testamarok ${testamarok_SRCS} )

target_link_libraries( testamarok
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${QT_QTTEST_LIBRARY}
    amarokcore
    amaroklib
    )

#------------------------ Test CaseConverter -----------------------------

set( testcaseconverter_SRCS
        TestCaseConverter.cpp
        ${AMAROK_SOURCE_TREE}/CaseConverter.cpp
    )

kde4_add_unit_test( testcaseconverter  ${testcaseconverter_SRCS} )

target_link_libraries(testcaseconverter ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} amarokcore)

#------------------------ Test Debug -----------------------------

add_definitions(-DDEBUG_OVERRIDE_ELAPSED_TIME=4.9)
set( testdebug_SRCS
     TestDebug.cpp
     ${AMAROK_SOURCE_TREE}/core/support/Debug.cpp
     )

kde4_add_unit_test( testdebug ${testdebug_SRCS} )
target_link_libraries(testdebug ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} amarokcore )

#------------------------ Test EngineController -----------------------------

set( testenginecontroller_SRCS TestEngineController.cpp )
kde4_add_unit_test( testenginecontroller ${testenginecontroller_SRCS} )
target_link_libraries( testenginecontroller ${KDE4_THREADWEAVER_LIBRARIES} ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} amaroklib amarokcore )

#------------------------ Test Expression -----------------------------

set( testexpression_SRCS
        TestExpression.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/Expression.cpp
    )

kde4_add_unit_test( testexpression  ${testexpression_SRCS} )

target_link_libraries(testexpression ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})

#------------------------ Test QStringx -----------------------------

set( testqstringx_SRCS
        TestQStringx.cpp
        ${AMAROK_SOURCE_TREE}/QStringx.cpp
    )

kde4_add_unit_test( testqstringx  ${testqstringx_SRCS} )

target_link_libraries(testqstringx ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})

#------------------------ Test SmartPointerList -----------------------------

set( testsmartpointerlist_SRCS
        TestSmartPointerList.cpp
        ${AMAROK_SOURCE_TREE}/core/support/SmartPointerList.cpp
    )

kde4_add_unit_test( testsmartpointerlist  ${testsmartpointerlist_SRCS} )

target_link_libraries(testsmartpointerlist ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})

#------------------------ Test TagGuesser -----------------------------

set( testtagguesser_SRCS
        TestTagGuesser.cpp
        ${CMAKE_SOURCE_DIR}/shared/TagsFromFileNameGuesser.cpp
        ${AMAROK_SOURCE_TREE}/dialogs/TagGuesser.cpp
    )

kde4_add_unit_test( testtagguesser  ${testtagguesser_SRCS} )

target_link_libraries(testtagguesser ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} amarokcore)

#------------------------ Test TrackOrganizer -----------------------------

set( testtrackorganizer_SRCS
        TestTrackOrganizer.cpp
        ${AMAROK_SOURCE_TREE}/dialogs/TrackOrganizer.cpp
        ${AMAROK_SOURCE_TREE}/core/meta/Meta.cpp
        ${AMAROK_SOURCE_TREE}/QStringx.cpp
    )

kde4_add_unit_test( testtrackorganizer  ${testtrackorganizer_SRCS} )

# Since Google recommends not to distribute a pre-compiled gtest library
# we have to build it our own
if(GOOGLEMOCK_GTEST_SOURCES)
    add_subdirectory( ${GOOGLEMOCK_GTEST_SOURCES} gtest )
endif(GOOGLEMOCK_GTEST_SOURCES)

add_dependencies( testtrackorganizer amarokcore )
add_dependencies( testtrackorganizer amaroklib )
add_dependencies( testtrackorganizer gtest )

target_link_libraries( testtrackorganizer amarokcore amaroklib ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_SOLID_LIBRARY} ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${GOOGLEMOCK_LIBRARIES})

