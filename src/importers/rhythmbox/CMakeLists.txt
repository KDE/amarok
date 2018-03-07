set( amarok_importer-rhythmbox_PART_SRCS
    RhythmboxConfigWidget.cpp
    RhythmboxManager.cpp
    RhythmboxProvider.cpp
    RhythmboxTrack.cpp
)

add_library(amarok_importer-rhythmbox MODULE ${amarok_importer-rhythmbox_PART_SRCS} )

target_link_libraries( amarok_importer-rhythmbox
    amarokcore
    amaroklib
    KF5::KIOCore
)

install( TARGETS amarok_importer-rhythmbox DESTINATION ${PLUGIN_INSTALL_DIR} )

kcoreaddons_desktop_to_json(amarok_importer-rhythmbox amarok_importer-rhythmbox.desktop SERVICE_TYPES ${CMAKE_SOURCE_DIR}/src/amarok-plugin.desktop)
