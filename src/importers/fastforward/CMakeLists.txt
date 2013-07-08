include_directories(
    ${CMAKE_BINARY_DIR}/src
    ${AMAROK_COLLECTION_SUPPORT_DIR}
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
)

########### next target ###############

set( amarok_importer-fastforwardimporter_PART_SRCS
    FastForwardConfig.cpp
    FastForwardProvider.cpp
    FastForwardTrack.cpp
)

kde4_add_plugin( amarok_importer-fastforwardimporter ${amarok_importer-fastforwardimporter_PART_SRCS} )

target_link_libraries( amarok_importer-fastforwardimporter
    amarokcore
    amaroklib
    ${KDE4_KIO_LIBS}
    ${QT_QTSQL_LIBRARY}
)

install(TARGETS amarok_importer-fastforwardimporter DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

#install( FILES amarok_collection-audiocdcollection.desktop DESTINATION ${SERVICES_INSTALL_DIR})

