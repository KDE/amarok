
add_subdirectory( void )
add_subdirectory( phonon )
if(XINE_FOUND)
add_subdirectory( xine )
else(XINE_FOUND)
	message(STATUS "Xine lib was not found. Xine engine will not be built")
endif(XINE_FOUND)

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(NMM_ENGINE_SUBDIR)")
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(HELIX_ENGINE_SUBDIR)")
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(GST10_ENGINE_SUBDIR)")
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(YAUAP_ENGINE_SUBDIR)")
