include_directories( ../..
   ../../collection
   ${CMAKE_CURRENT_BINARY_DIR}/../..
   ${AMAROK_COLLECTION_SUPPORT_DIR}
   ${KDE4_INCLUDE_DIR}
   ${QT_INCLUDES} )


########### next target ###############

set(amarok_collection-upnpcollection_PART_SRCS
   upnpmeta.cpp
   upnpcollection.cpp)


kde4_add_plugin(amarok_collection-upnpcollection ${amarok_collection-upnpcollection_PART_SRCS})

target_link_libraries(amarok_collection-upnpcollection amarokcore amaroklib ${KDE4_THREADWEAVER_LIBRARIES} ${KDE4_KDNSSD_LIBS})

if(APPLE)
	SET_TARGET_PROPERTIES(amarok_collection-upnpcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

install(TARGETS amarok_collection-upnpcollection DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

# install( FILES amarok_collection-upnpcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})
