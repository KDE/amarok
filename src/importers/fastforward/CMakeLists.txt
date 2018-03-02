set( amarok_importer-fastforward_PART_SRCS
    FastForwardConfigWidget.cpp
    FastForwardManager.cpp
    FastForwardProvider.cpp
    FastForwardTrack.cpp
)

ki18n_wrap_ui( amarok_importer-fastforward_PART_SRCS FastForwardConfigWidget.ui )

add_library(amarok_importer-fastforward MODULE ${amarok_importer-fastforward_PART_SRCS} )

target_link_libraries( amarok_importer-fastforward
    amarokcore
    amaroklib
    KF5::KIOCore
    Qt5::Sql
)

install( TARGETS amarok_importer-fastforward DESTINATION ${PLUGIN_INSTALL_DIR} )

kcoreaddons_desktop_to_json(amarok_importer-fastforward amarok_importer-fastforward.desktop SERVICE_TYPES ${CMAKE_SOURCE_DIR}/src/amarok-plugin.desktop)
