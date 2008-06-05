macro_optional_find_package(Ipod)

if (IPOD_FOUND)
   include_directories( ../..
   ../../plugin
   ../../meta
   ../../collection
   ${GLIB2_INCLUDE_DIR}
   ${IPOD_INCLUDE_DIR}
   ${CMAKE_CURRENT_BINARY_DIR}/../..
   ${AMAROK_COLLECTION_SUPPORT_DIR}
   ${KDE4_INCLUDE_DIR}
   ${QT_INCLUDES} )


########### next target ###############

	set(amarok_collection-mediadevicecollection_PART_SRCS
	mediadevicemeta.cpp
	mediadevicecollection.cpp)
	
	kde4_add_plugin(amarok_collection-mediadevicecollection WITH_PREFIX ${amarok_collection-mediadevicecollection_PART_SRCS})

	target_link_libraries(amarok_collection-mediadevicecollection amaroklib ${KDE4_THREADWEAVER_LIBRARIES} ${GLIB2_LIBRARIES} ${IPOD_LIBRARIES} )

	install(TARGETS amarok_collection-mediadevicecollection DESTINATION ${PLUGIN_INSTALL_DIR} )


	########### install files ###############

	install( FILES amarok_collection-mediadevicecollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})

endif (IPOD_FOUND)
