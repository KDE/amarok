ENABLE_TESTING()
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( .
                     ../src/
                     ../src/playlistmanager/
)

QT4_AUTOMOC( TestAmarok.cpp
             TestCaseConverter.cpp
             TestDirectoryLoader.cpp
             TestExpression.cpp
             TestQStringx.cpp
             TestSmartPointerList.cpp
)

add_subdirectory( data )
add_subdirectory( playlistmanager )
