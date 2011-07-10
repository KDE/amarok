include_directories( ../..
                    ../../..)

set( tabs_applet_SRCS TabsApplet.cpp TabsView.cpp TabsItem.cpp )

kde4_add_ui_files( tabs_applet_SRCS TabsSettings.ui ReloadEditDialog.ui )
kde4_add_plugin( amarok_context_applet_tabs ${tabs_applet_SRCS} )
target_link_libraries( amarok_context_applet_tabs
    amarokcore
    amaroklib
    ${KDE4_PLASMA_LIBS}
    ${KDE4_KIO_LIBS}
)

install( TARGETS amarok_context_applet_tabs DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-context-applet-tabs.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES amarok-tabs-guitar.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
install( FILES amarok-tabs-bass.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
install( FILES amarok-tabs-drum.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
install( FILES amarok-tabs-piano.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
