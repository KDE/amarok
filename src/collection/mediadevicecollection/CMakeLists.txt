include_directories( ../..
        ../../plugin
   ../../meta
   ../../collection
   ${CMAKE_CURRENT_BINARY_DIR}/../..
   ${AMAROK_COLLECTION_SUPPORT_DIR}
   ${KDE4_INCLUDE_DIR}
   ${QT_INCLUDES} )


########### next target ###############

set(amarok_collection-mediadevicecollection_PART_SRCS
   mediadevicemeta.cpp
   mediadevicecollection.cpp)


kde4_add_plugin(amarok_collection-mediadevicecollection WITH_PREFIX ${amarok_collection-mediadevicecollection_PART_SRCS})

target_link_libraries(amarok_collection-mediadevicecollection amaroklib ${KDE4_THREADWEAVER_LIBRARIES} ${KDE4_KDNSSD_LIBS})

if(APPLE)
	SET_TARGET_PROPERTIES(amarok_collection-mediadevicecollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

install(TARGETS amarok_collection-mediadevicecollection DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

# install( FILES amarok_collection-mediadevicecollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})
