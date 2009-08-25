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
   ../mediadevicecollection
   ../mediadevicecollection/support
   ../mediadevicecollection/handler
   ../mediadevicecollection/handler/capabilities
   handler
   handler/capabilities
   support
   ${GLIB2_INCLUDE_DIR}
   ${MTP_INCLUDE_DIR}
   ${CMAKE_CURRENT_BINARY_DIR}/../..
   ${AMAROK_COLLECTION_SUPPORT_DIR}
   ${KDE4_INCLUDE_DIR}
   ${QT_INCLUDES} )


########### next target ###############

	set(amarok_collection-mtpcollection_PART_SRCS
	MtpCollection.cpp
	handler/MtpHandler.cpp
	handler/capabilities/MtpPlaylistCapability.cpp
	handler/capabilities/MtpReadCapability.cpp
	handler/capabilities/MtpWriteCapability.cpp	
	support/MtpDeviceInfo.cpp
	support/MtpConnectionAssistant.cpp)
	kde4_add_plugin(amarok_collection-mtpcollection ${amarok_collection-mtpcollection_PART_SRCS})

	target_link_libraries(amarok_collection-mtpcollection
        amaroklib 
        amarokpud
        ${KDE4_KDECORE_LIBS}
        ${KDE4_KIO_LIBS}
        ${KDE4_KDEUI_LIBS}
        ${KDE4_SOLID_LIBRARY}
        ${KDE4_THREADWEAVER_LIBRARIES}
        ${QT_QTGUI_LIBRARY}
        ${GLIB2_LIBRARIES}
        ${MTP_LIBRARIES}
        )

	install(TARGETS amarok_collection-mtpcollection DESTINATION ${PLUGIN_INSTALL_DIR} )


	########### install files ###############

	install( FILES amarok_collection-mtpcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})

endif (MTP_FOUND)
