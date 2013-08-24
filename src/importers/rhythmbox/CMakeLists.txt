include_directories(
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
)

set( amarok_importer-rhythmbox_PART_SRCS
    RhythmboxConfigWidget.cpp
    RhythmboxManager.cpp
    RhythmboxProvider.cpp
)

kde4_add_ui_files( amarok_importer-rhythmbox_PART_SRCS RhythmboxConfigWidget.ui )

kde4_add_plugin( amarok_importer-rhythmbox ${amarok_importer-rhythmbox_PART_SRCS} )

target_link_libraries( amarok_importer-rhythmbox
    amarokcore
    amaroklib
    ${KDE4_KIO_LIBS}
)

install( TARGETS amarok_importer-rhythmbox DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES amarok_importer-rhythmbox.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
