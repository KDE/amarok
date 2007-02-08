
include_directories( ${CMAKE_SOURCE_DIR}/amarok/src/amarokcore ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(konqsidebar_universalamarok_PART_SRCS universalamarok.cpp )

kde4_automoc(${konqsidebar_universalamarok_PART_SRCS})

#kde4_add_dcop_stubs(konqsidebar_universalamarok_PART_SRCS ${CMAKE_SOURCE_DIR}/amarok/src/amarokcore/amarokdcopiface.h )


kde4_add_plugin(konqsidebar_universalamarok ${konqsidebar_universalamarok_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konqsidebar_universalamarok )

target_link_libraries(konqsidebar_universalamarok  konqsidebarplugin ${KDE4_KHTML_LIBS} ${KDE4_KPARTS_LIBS} )

install(TARGETS konqsidebar_universalamarok DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES amarok.desktop DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/entries )
install( FILES  amarok.desktop DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/kicker_entries)
install( FILES amarok.desktop DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/add )

