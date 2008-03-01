
add_subdirectory( void )
add_subdirectory( phonon )
if(XINE_FOUND)
add_subdirectory( xine )
else(XINE_FOUND)
	message(STATUS "Xine lib was not found. Xine engine will not be built")
endif(XINE_FOUND)

