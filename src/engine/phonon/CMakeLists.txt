
include_directories( ${CMAKE_SOURCE_DIR}/amarok/src ${CMAKE_SOURCE_DIR}/amarok/src/plugin
        ${CMAKE_SOURCE_DIR}/amarok/src/amarokcore ${CMAKE_BINARY_DIR}/amarok/src ${KDE4_INCLUDE_DIR}
        ${QT_INCLUDES} )


########### next target ###############

set(amarok_phonon-engine_PART_SRCS phonon-engine.cpp )

kde4_automoc(${amarok_phonon-engine_PART_SRCS})

kde4_add_plugin(amarok_phonon-engine WITH_PREFIX ${amarok_phonon-engine_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} amarok_phonon-engine )

target_link_libraries(amarok_phonon-engine  ${KDE4_KDECORE_LIBS} 
        amarok ${KDE4_PHONONCORE_LIBRARY} )

install(TARGETS amarok_phonon-engine DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES   amarok_phonon-engine.desktop DESTINATION ${SERVICES_INSTALL_DIR})

