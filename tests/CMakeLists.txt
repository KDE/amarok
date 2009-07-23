ENABLE_TESTING()
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

########### PlaylistManager ###############
set( PlaylistManagerTest_SRCS TestPlaylistManager.cpp ../src/playlistmanager/PlaylistManager.cpp )

qt4_automoc( ${PlaylistManagerTest_SRCS} )


########### SmartPointerList ###############
set( SmartPointerListTest_SRCS TestSmartPointerList.cpp ../src/SmartPointerList.cpp )

qt4_automoc( ${SmartPointerListTest_SRCS} )

add_subdirectory( data )
