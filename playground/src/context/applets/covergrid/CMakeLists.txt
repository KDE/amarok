project(context-covergrid)

set(covergrid_SRCS
    CoverGridApplet.cpp
    AlbumItem.cpp
)

include_directories( ../../..
                    ../..
                    ${KDE4_INCLUDE_DIR}/amarok # this way we don't need to prefix it with amarok/ (and it compiles this way too :)
)

kde4_add_ui_files( covergrid_SRCS CoverGridSettings.ui )
kde4_add_plugin(amarok_context_applet_covergrid ${covergrid_SRCS})


target_link_libraries(amarok_context_applet_covergrid
    amarokcore
    amaroklib
    ${KDE4_PLASMA_LIBS} 
    ${KDE4_KIO_LIBS}
)

install(TARGETS amarok_context_applet_covergrid DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-covergrid.desktop DESTINATION ${SERVICES_INSTALL_DIR})
