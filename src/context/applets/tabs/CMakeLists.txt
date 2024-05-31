include_directories( ../..
                    ../../..)

set( tabs_applet_SRCS TabsApplet.cpp TabsView.cpp TabsItem.cpp )

ki18n_wrap_ui( tabs_applet_SRCS TabsSettings.ui ReloadEditDialog.ui )
add_library(amarok_context_applet_tabs MODULE ${tabs_applet_SRCS} )
target_link_libraries( amarok_context_applet_tabs
    amarokcore
    amaroklib
    KF${KF_MAJOR_VERSION}::Plasma
    KF${KF_MAJOR_VERSION}::KIOCore
)

install( TARGETS amarok_context_applet_tabs DESTINATION ${KDE_INSTALL_PLUGINDIR} )
install( FILES amarok-context-applet-tabs.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
install( FILES amarok-tabs-guitar.png DESTINATION ${KDE_INSTALL_DATADIR}/amarok/images/ )
install( FILES amarok-tabs-bass.png DESTINATION ${KDE_INSTALL_DATADIR}/amarok/images/ )
install( FILES amarok-tabs-drum.png DESTINATION ${KDE_INSTALL_DATADIR}/amarok/images/ )
install( FILES amarok-tabs-piano.png DESTINATION ${KDE_INSTALL_DATADIR}/amarok/images/ )
