set( amarok_importer-itunes_PART_SRCS
    ITunesConfigWidget.cpp
    ITunesManager.cpp
    ITunesProvider.cpp
    ITunesTrack.cpp
)

add_library(amarok_importer-itunes MODULE ${amarok_importer-itunes_PART_SRCS} )

target_link_libraries( amarok_importer-itunes
    amarokcore
    amaroklib
    KF5::KIOCore
)

install( TARGETS amarok_importer-itunes DESTINATION ${KDE_INSTALL_PLUGINDIR} )

kcoreaddons_desktop_to_json(amarok_importer-itunes amarok_importer-itunes.desktop SERVICE_TYPES ${CMAKE_SOURCE_DIR}/src/amarok-plugin.desktop)
