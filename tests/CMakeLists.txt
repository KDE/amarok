set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

find_package(Googlemock REQUIRED)
macro_log_feature( GOOGLEMOCK_FOUND "gmock" "Used in Amarok's tests." "http://code.google.com/p/googlemock/" TRUE "1.4" "" )

set( AMAROK_SOURCE_TREE ${CMAKE_SOURCE_DIR}/src )
set( AMAROK_TEST_TREE ${CMAKE_SOURCE_DIR}/tests )
set( AMAROK_UTILITY_TREE ${CMAKE_SOURCE_DIR}/utilities )

set( STRESS_TEST_TRACK_COUNT 20000 )

configure_file(config-amarok-test.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-amarok-test.h )

include_directories( .
                     ${AMAROK_SOURCE_TREE}
)

add_subdirectory( data )
add_subdirectory( context )
add_subdirectory( core )
add_subdirectory( core-implementations )
add_subdirectory( playlist )
add_subdirectory( playlistmanager )
add_subdirectory( timecode )
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
    amaroklib
    )

#------------------------ Test CaseConverter -----------------------------

set( testcaseconverter_SRCS
        TestCaseConverter.cpp
        ${AMAROK_SOURCE_TREE}/CaseConverter.cpp
    )

kde4_add_unit_test( testcaseconverter  ${testcaseconverter_SRCS} )

target_link_libraries(testcaseconverter ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})

#------------------------ Test Expression -----------------------------

set( testexpression_SRCS
        TestExpression.cpp
        ${AMAROK_SOURCE_TREE}/Expression.cpp
    )

kde4_add_unit_test( testexpression  ${testexpression_SRCS} )

target_link_libraries(testexpression ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})

#------------------------ Test QStringx -----------------------------

set( testqstringx_SRCS
        TestQStringx.cpp
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
