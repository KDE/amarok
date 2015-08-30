include_directories( ../..
   ${CMAKE_CURRENT_BINARY_DIR}/../..
   ${AMAROK_COLLECTION_SUPPORT_DIR}
   
   
)


########### next target ###############

set(amarok_collection-upnpcollection_PART_SRCS
    dbuscodec.cpp
    UpnpCollectionBase.cpp
    UpnpBrowseCollection.cpp
    UpnpSearchCollection.cpp
    UpnpCollectionFactory.cpp
    UpnpMemoryQueryMaker.cpp
    UpnpQueryMaker.cpp
    UpnpQueryMakerInternal.cpp
    UpnpMeta.cpp
    UpnpCache.cpp
    UpnpQuery.cpp
)


add_library(amarok_collection-upnpcollection MODULE ${amarok_collection-upnpcollection_PART_SRCS})

target_link_libraries(
    amarok_collection-upnpcollection
    amarokcore
    amaroklib
    KF5::ThreadWeaver
    KF5::KIOCore
)

if(APPLE)
    set_target_properties(amarok_collection-upnpcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

install(TARGETS amarok_collection-upnpcollection DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_collection-upnpcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})
