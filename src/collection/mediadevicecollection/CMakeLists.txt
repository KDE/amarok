include_directories( ../..
   ../../plugin
   ../../meta
   ../../meta/file
   ../../collection
   ../../collection/support
   handler
   handler/capabilities
   support
   ${CMAKE_CURRENT_BINARY_DIR}/../..
   ${AMAROK_COLLECTION_SUPPORT_DIR}
   ${KDE4_INCLUDE_DIR}
   ${QT_INCLUDES} )


########### next target ###############

	set(amarok_collection-mediadevicecollection_PART_SRCS
	handler/MediaDeviceHandler.cpp
	handler/MediaDeviceHandlerCapability.cpp
	handler/capabilities/ReadCapability.cpp
	support/MediaDeviceInfo.cpp
	support/ConnectionAssistant.cpp
	MediaDeviceCollection.cpp
	MediaDeviceCollectionLocation.cpp
	CollectionCapabilityMediaDevice.cpp)
	
	kde4_add_plugin(amarok_collection-mediadevicecollection WITH_PREFIX ${amarok_collection-mediadevicecollection_PART_SRCS})

	target_link_libraries(amarok_collection-mediadevicecollection
	  amaroklib
	  ${QT_QTGUI_LIBRARY}
	  ${KDE4_KUTILS_LIBS}
	  ${KDE4_THREADWEAVER_LIBRARIES})
	

	install(TARGETS 
	  amarok_collection-mediadevicecollection
	  DESTINATION
	  ${PLUGIN_INSTALL_DIR} )


	########### install files ###############

#	install( FILES amarok_collection-mediadevicecollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})


