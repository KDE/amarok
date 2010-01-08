macro_optional_find_package(Mtp)


if (MTP_FOUND)
   include_directories(
       ${Amarok_SOURCE_DIR}/src
       ${Amarok_SOURCE_DIR}/src/plugin
       ${Amarok_SOURCE_DIR}/src/meta
       ${Amarok_SOURCE_DIR}/src/meta/file
       ${Amarok_SOURCE_DIR}/src/collection
       ${Amarok_SOURCE_DIR}/src/collection/mediadevicecollection
       ${Amarok_SOURCE_DIR}/src/collection/mediadevicecollection/support
       ${Amarok_SOURCE_DIR}/src/collection/mediadevicecollection/handler
       ${Amarok_SOURCE_DIR}/src/collection/mediadevicecollection/handler/capabilities
       ${Amarok_SOURCE_DIR}/src/collection/mtpcollection/handler
       ${Amarok_SOURCE_DIR}/src/collection/mtpcollection/handler/capabilities
       ${Amarok_SOURCE_DIR}/src/collection/mtpcollection/support
       ${GLIB2_INCLUDE_DIR}
       ${MTP_INCLUDE_DIR}
       ${CMAKE_CURRENT_BINARY_DIR}/../..
       ${AMAROK_COLLECTION_SUPPORT_DIR}
       ${KDE4_INCLUDE_DIR}
       ${QT_INCLUDES}
)


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
