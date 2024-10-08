include_directories( ../..
    ${CMAKE_BINARY_DIR}/src
    ${AMAROK_COLLECTION_SUPPORT_DIR}
    daapreader
    )

remove_definitions(-DQT_NO_HTTP)

########### next target ###############

set(amarok_collection-daapcollection_PART_SRCS
   DaapMeta.cpp
   DaapCollection.cpp
   daapreader/Reader.cpp
   daapreader/authentication/contentfetcher.cpp
   daapreader/authentication/hasher.c
   daapreader/authentication/md5.c )

add_library(amarok_collection-daapcollection MODULE ${amarok_collection-daapcollection_PART_SRCS})

target_link_libraries(amarok_collection-daapcollection
    amarokcore
    amaroklib
    KF${KF_MAJOR_VERSION}::ThreadWeaver
    KF${KF_MAJOR_VERSION}::DNSSD
    Qt::Network
)

if(APPLE)
    set_target_properties(amarok_collection-daapcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

install(TARGETS amarok_collection-daapcollection DESTINATION ${KDE_INSTALL_PLUGINDIR} )
