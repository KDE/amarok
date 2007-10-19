
include_directories( ../..
	../../plugin
	../../amarokcore
	../.. ${KDE4_INCLUDE_DIR}
     ${QT_INCLUDES} )


########### next target ###############

set(amarok_phonon-engine_SRCS phonon-engine.cpp )

kde4_add_plugin(amarok_phonon-engine WITH_PREFIX ${amarok_phonon-engine_SRCS})

target_link_libraries(amarok_phonon-engine  ${KDE4_KDECORE_LIBS}
        amaroklib ${KDE4_PHONON_LIBS} )

install(TARGETS amarok_phonon-engine DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES   amarok_phonon-engine.desktop DESTINATION ${SERVICES_INSTALL_DIR})

