if(NOT GDK_FOUND)
    SET(GDK_INCLUDE_DIRS "")
    SET(GDK_LIBRARIES "")
endif(NOT GDK_FOUND)

if (IPOD_FOUND AND WITH_IPOD)
   include_directories( ../..
   ../../plugin
   ../../meta
   ../../meta/file
   ../../collection
   ../../metadata/audible
   ../mediadevicecollection
   ../mediadevicecollection/support
   ../mediadevicecollection/handler
   ../mediadevicecollection/handler/capabilities
   handler
   handler/capabilities
   support
   ${GLIB2_INCLUDE_DIR}
   ${GDK_INCLUDE_DIRS}
   ${IPOD_INCLUDE_DIRS}
   ${CMAKE_CURRENT_BINARY_DIR}/../..
   ${AMAROK_COLLECTION_SUPPORT_DIR}
   ${KDE4_INCLUDE_DIR}
   ${QT_INCLUDES}
)

########### set GDK var ################
     # Generate config-gdk.h.cmake
     configure_file(config-gdk.h.cmake 
         ${CMAKE_CURRENT_BINARY_DIR}/config-gdk.h )


########### next target ################

    set(amarok_collection-ipodcollection_PART_SRCS
	IpodCollection.cpp
	handler/IpodHandler.cpp
	handler/capabilities/IpodArtworkCapability.cpp
	handler/capabilities/IpodPlaylistCapability.cpp
	handler/capabilities/IpodReadCapability.cpp
	handler/capabilities/IpodWriteCapability.cpp	
	support/IpodDeviceInfo.cpp
	support/IpodConnectionAssistant.cpp)
	kde4_add_plugin(amarok_collection-ipodcollection ${amarok_collection-ipodcollection_PART_SRCS})

    target_link_libraries(amarok_collection-ipodcollection
        amaroklib
        amarokpud
        ${KDE4_KDECORE_LIBS}
        ${KDE4_KIO_LIBS}
        ${KDE4_KDEUI_LIBS}
        ${KDE4_SOLID_LIBRARY}
        ${KDE4_THREADWEAVER_LIBRARIES}
        ${QT_QTGUI_LIBRARY}
        ${GLIB2_LIBRARIES}
        ${GDK_LIBRARIES}
        ${IPOD_LIBRARIES} 
        )

    install(TARGETS 
        amarok_collection-ipodcollection 
        DESTINATION 
        ${PLUGIN_INSTALL_DIR} )

    ########### install files ###############

    install(FILES 
        amarok_collection-ipodcollection.desktop 
        DESTINATION 
        ${SERVICES_INSTALL_DIR})
endif (IPOD_FOUND AND WITH_IPOD)
