
add_subdirectory( void )
add_subdirectory( phonon )
if(XINE_FOUND)
add_subdirectory( xine )
else(XINE_FOUND)
	message(STATUS "Xine lib was not found. Xine engine will not be built")
endif(XINE_FOUND)

if (HELIX_FOUND)
add_subdirectory( helix )
else(HELIX_FOUND)
   MESSAGE(STATUS "Helix/Realplayer was not found. Helix engine will not be built")
endif(HELIX_FOUND)

