set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

find_package(Googlemock REQUIRED)

set( AMAROK_SOURCE_TREE ${CMAKE_SOURCE_DIR}/src )
set( AMAROK_TEST_TREE ${CMAKE_SOURCE_DIR}/tests )
set( AMAROK_UTILITY_TREE ${CMAKE_SOURCE_DIR}/utilities )

set( STRESS_TEST_TRACK_COUNT 20000 )

configure_file(config-amarok-test.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-amarok-test.h )

include_directories( .
                     ${AMAROK_SOURCE_TREE}
)

add_subdirectory( data )
add_subdirectory( collection )
add_subdirectory( meta )
add_subdirectory( playlistmanager )
add_subdirectory( timecode )
add_subdirectory( synchronization )

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
        ${AMAROK_SOURCE_TREE}/SmartPointerList.cpp
    )

kde4_add_unit_test( testsmartpointerlist  ${testsmartpointerlist_SRCS} )

target_link_libraries(testsmartpointerlist ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})
