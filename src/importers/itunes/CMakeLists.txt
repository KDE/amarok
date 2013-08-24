include_directories(
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
)

set( amarok_importer-itunes_PART_SRCS
    ITunesConfigWidget.cpp
    ITunesManager.cpp
    ITunesProvider.cpp
)

kde4_add_ui_files( amarok_importer-itunes_PART_SRCS ITunesConfigWidget.ui )

kde4_add_plugin( amarok_importer-itunes ${amarok_importer-itunes_PART_SRCS} )

target_link_libraries( amarok_importer-itunes
    amarokcore
    amaroklib
    ${KDE4_KIO_LIBS}
)

install( TARGETS amarok_importer-itunes DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES amarok_importer-itunes.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
