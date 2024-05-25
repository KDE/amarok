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
    Qt::Sql
)

install( TARGETS amarok_importer-fastforward DESTINATION ${KDE_INSTALL_PLUGINDIR} )
