
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../..
	${CMAKE_CURRENT_SOURCE_DIR}/../../plugin
	${CMAKE_CURRENT_SOURCE_DIR}/../../amarokcore
	${CMAKE_CURRENT_BINARY_DIR}/../.. ${KDE4_INCLUDE_DIR}
    ${TAGLIB_INCLUDE_DIR}    ${QT_INCLUDES} )


########### next target ###############

set(amarok_phonon-engine_SRCS phonon-engine.cpp )

kde4_automoc(${amarok_phonon-engine_SRCS})

kde4_add_plugin(amarok_phonon-engine WITH_PREFIX ${amarok_phonon-engine_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} amarok_phonon-engine )

target_link_libraries(amarok_phonon-engine  ${KDE4_KDECORE_LIBS} 
        amarok ${KDE4_PHONONCORE_LIBRARY} )

install(TARGETS amarok_phonon-engine DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES   amarok_phonon-engine.desktop DESTINATION ${SERVICES_INSTALL_DIR})

