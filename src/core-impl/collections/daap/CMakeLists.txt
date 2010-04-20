include_directories( ../..
    ../../collection
    ${CMAKE_BINARY_DIR}/src
    ${AMAROK_COLLECTION_SUPPORT_DIR}
    daapreader
    /daapreader/authentication
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES} )

remove_definitions(-DQT_NO_HTTP)

########### next target ###############

set(amarok_collection-daapcollection_PART_SRCS
   DaapMeta.cpp
   DaapCollection.cpp
   daapreader/Reader.cpp
   daapreader/authentication/contentfetcher.cpp
   daapreader/authentication/hasher.c
   daapreader/authentication/md5.c )

kde4_add_plugin(amarok_collection-daapcollection ${amarok_collection-daapcollection_PART_SRCS})

target_link_libraries(amarok_collection-daapcollection
    amarokcore
    amaroklib
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${KDE4_KDNSSD_LIBS}
    ${QT_QTNETWORK_LIBRARY}
)

if(APPLE)
    SET_TARGET_PROPERTIES(amarok_collection-daapcollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

install(TARGETS amarok_collection-daapcollection DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok_collection-daapcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})
