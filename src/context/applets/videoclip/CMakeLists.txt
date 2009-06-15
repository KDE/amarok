include_directories( ../.. 
					 ../../..
					 ../../../meta
)

set( videoclip_applet_SRCS
     VideoclipApplet.cpp
	 ../../../widgets/kratingwidget.cpp
     ../../../widgets/kratingpainter.cpp
)

kde4_add_plugin(amarok_context_applet_videoclip ${videoclip_applet_SRCS})
target_link_libraries( amarok_context_applet_videoclip amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KIO_LIBS} ${KDE4_PHONON_LIBS})

install( TARGETS amarok_context_applet_videoclip DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-context-applet-videoclip.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES amarok-videoclip-youtube.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
install( FILES amarok-videoclip-dailymotion.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
install( FILES amarok-videoclip-vimeo.png DESTINATION ${DATA_INSTALL_DIR}/amarok/images/ )
