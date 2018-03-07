include_directories( ../..
                    ../../..)

set( tabs_applet_SRCS TabsApplet.cpp TabsView.cpp TabsItem.cpp )

ki18n_wrap_ui( tabs_applet_SRCS TabsSettings.ui ReloadEditDialog.ui )
add_library(amarok_context_applet_tabs MODULE ${tabs_applet_SRCS} )
target_link_libraries( amarok_context_applet_tabs
    amarokcore
    amaroklib
    KF5::Plasma
    KF5::KIOCore
)

install( TARGETS amarok_context_applet_tabs DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-context-applet-tabs.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES amarok-tabs-guitar.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
install( FILES amarok-tabs-bass.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
install( FILES amarok-tabs-drum.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
install( FILES amarok-tabs-piano.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
