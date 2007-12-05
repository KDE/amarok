set(phonon_amarok_directshow_PART_SRCS
	DirectShowAudioOutput.cpp
	DirectShowBackend.cpp
	DirectShowMediaObject.cpp
   )
   
kde4_add_plugin(phonon_amarok_directshow ${phonon_amarok_directshow_PART_SRCS})
target_link_libraries(phonon_amarok_directshow ${KDE4_KDECORE_LIBS} ${KDE4_PHONON_LIBS} Strmiids.lib Ole32.lib)

install(TARGETS phonon_amarok_directshow DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok_directshow.desktop DESTINATION ${SERVICES_INSTALL_DIR}/phononbackends)
kde4_install_icons(${ICON_INSTALL_DIR})
