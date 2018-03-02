set( amarok_importer-clementine_PART_SRCS
    ClementineConfigWidget.cpp
    ClementineManager.cpp
    ClementineProvider.cpp
    ClementineTrack.cpp
)

add_library(amarok_importer-clementine MODULE ${amarok_importer-clementine_PART_SRCS} )

target_link_libraries( amarok_importer-clementine
    amarokcore
    amaroklib
    KF5::KIOCore
    Qt5::Sql
)

install( TARGETS amarok_importer-clementine DESTINATION ${PLUGIN_INSTALL_DIR} )

kcoreaddons_desktop_to_json(amarok_importer-clementine amarok_importer-clementine.desktop SERVICE_TYPES ${CMAKE_SOURCE_DIR}/src/amarok-plugin.desktop)
