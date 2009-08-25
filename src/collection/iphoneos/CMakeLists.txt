macro_optional_find_package(Sqlite)
macro_log_feature( SQLITE_FOUND "Sqlite" "Read-only support for iPhone OS 3.0 devices"
      "http://sqlite.org" FALSE "3.6" "" )

if(SQLITE_FOUND)

include_directories( ../..
    ../../plugin
    ../../meta
    ../../collection
    ../mediadevicecollection
    ../mediadevicecollection/support
    ../mediadevicecollection/handler
    ${CMAKE_CURRENT_BINARY_DIR}/../..
    ${AMAROK_COLLECTION_SUPPORT_DIR}
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
    ${SQLITE_INCLUDE_DIR}
)


########### next target ###############

set(amarok_collection-iphoneoscollection_PART_SRCS
   IphoneOsCollection.cpp
   support/IphoneOsConnectionAssistant.cpp
   support/IphoneOsDeviceInfo.cpp
   handler/IphoneOsHandler.cpp
   handler/capabilities/IphoneOsReadCapability.cpp
   handler/capabilities/IphoneOsPlaylistCapability.cpp
   )

kde4_add_plugin(amarok_collection-iphoneoscollection 
   ${amarok_collection-iphoneoscollection_PART_SRCS})

target_link_libraries(amarok_collection-iphoneoscollection
    amaroklib
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${KDE4_SOLID_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
    ${SQLITE_LIBRARIES}
)

if(APPLE)
    SET_TARGET_PROPERTIES(amarok_collection-iphoneoscollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

install(TARGETS amarok_collection-iphoneoscollection DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_collection-iphoneoscollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})

endif(SQLITE_FOUND)

