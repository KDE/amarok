include_directories( ../..
   ../../plugin
   ../../meta
   ../../collection
   ${CMAKE_CURRENT_BINARY_DIR}/../..
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${AMAROK_COLLECTION_SUPPORT_DIR}
   ${KDE4_INCLUDE_DIR}
   ${QT_INCLUDES} )


########### next target ###############

	set(mediadevicelib_LIB_SRCS
    support/MediaDeviceInfo.cpp
	support/IpodDeviceInfo.cpp)
	
	kde4_add_library(mediadevicelib SHARED ${mediadevicelib_LIB_SRCS})

	target_link_libraries(mediadevicelib ${KDE4_KUTILS_LIBS})

	set_target_properties(mediadevicelib PROPERTIES VERSION 1.0.0 SOVERSION 1 )
	install(TARGETS mediadevicelib ${INSTALL_TARGETS_DEFAULT_ARGS} )


	########### install files ###############

#	install( FILES amarok_collection-mediadevicecollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})


