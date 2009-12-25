ENABLE_TESTING()
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set( AMAROK_SOURCE_TREE ${CMAKE_SOURCE_DIR}/src )
set( AMAROK_TEST_TREE ${CMAKE_SOURCE_DIR}/tests )

include_directories( .
                     ../src/
)

QT4_AUTOMOC( TestAmarok.cpp
             TestCaseConverter.cpp
             TestDirectoryLoader.cpp
             TestExpression.cpp
             TestQStringx.cpp
             TestSmartPointerList.cpp
)

add_subdirectory( data )
add_subdirectory( collection )
add_subdirectory( meta )
add_subdirectory( playlistmanager )
add_subdirectory( timecode )
