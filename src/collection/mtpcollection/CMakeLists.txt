macro_optional_find_package(Mtp)

# Old MTP stuff, which might be reverted to or reused to accompany old libmtp cmake madness
#INCLUDE(FindPkgConfig)
#pkg_check_modules(Mtp libmtp>=0.3.0)

if (MTP_FOUND)
   include_directories( ../..
   ../../plugin
   ../../meta
   ../../meta/file
   ../../collection
   handler
   ${GLIB2_INCLUDE_DIR}
   ${MTP_INCLUDE_DIR}
   ${CMAKE_CURRENT_BINARY_DIR}/../..
   ${AMAROK_COLLECTION_SUPPORT_DIR}
   ${KDE4_INCLUDE_DIR}
   ${QT_INCLUDES} )


########### next target ###############

	set(amarok_collection-mtpcollection_PART_SRCS
	MtpMeta.cpp
	MtpCollection.cpp
	MtpCollectionLocation.cpp
	handler/MtpHandler.cpp
	CollectionCapabilityMtp.cpp)	
	kde4_add_plugin(amarok_collection-mtpcollection WITH_PREFIX ${amarok_collection-mtpcollection_PART_SRCS})

	target_link_libraries(amarok_collection-mtpcollection
        amaroklib 
        amarokpud
        ${KDE4_KDECORE_LIBS}
        ${KDE4_KIO_LIBS}
        ${KDE4_THREADWEAVER_LIBRARIES}
        ${GLIB2_LIBRARIES}
        ${MTP_LIBRARIES}
        )

	install(TARGETS amarok_collection-mtpcollection DESTINATION ${PLUGIN_INSTALL_DIR} )


	########### install files ###############

	install( FILES amarok_collection-mtpcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})

endif (MTP_FOUND)
