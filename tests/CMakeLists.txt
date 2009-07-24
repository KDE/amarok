ENABLE_TESTING()
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

########### PlaylistManager ###############
set( PlaylistManagerTest_SRCS TestPlaylistManager.cpp TestPlaylistManager.h ../src/playlistmanager/PlaylistManager.cpp )

QT4_AUTOMOC( TestPlaylistManager.cpp )
#QT4_GENERATE_MOC( TestPlaylistManager.h TestPlaylistManager.moc )
#qt4_wrap_cpp( TestPlaylistManager.moc ${PlaylistManagerTest_SRCS} )

########### SmartPointerList ###############
set( SmartPointerListTest_SRCS TestSmartPointerList.cpp ../src/SmartPointerList.cpp )

QT4_AUTOMOC( TestSmartPointerList.cpp )
#qt4_wrap_cpp( TestSmartPointerList.moc ${SmartPointerListTest_SRCS} )
#qt4_automoc( ${SmartPointerListTest_SRCS} )

add_subdirectory( data )
