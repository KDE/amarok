set(phonon_amarok_directshow_PART_SRCS
	DirectShowAudioOutput.cpp
	DirectShowBackend.cpp
	DirectShowGraph.cpp
	DirectShowMediaObject.cpp
   )
   
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../..
     ${KDE4_INCLUDE_DIR}
     ${QT_INCLUDES} )
   
add_definitions( -DUNICODE -DWINVER=0x0500 )
   
kde4_add_plugin(phonon_amarok_directshow ${phonon_amarok_directshow_PART_SRCS})
target_link_libraries(phonon_amarok_directshow ${KDE4_KDECORE_LIBS} ${KDE4_PHONON_LIBS} amaroklib Strmiids.lib Ole32.lib Oleaut32.lib uuid.lib)

install(TARGETS phonon_amarok_directshow DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok_directshow.desktop DESTINATION ${SERVICES_INSTALL_DIR}/phononbackends)
kde4_install_icons(${ICON_INSTALL_DIR})
