
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../plugin
	${CMAKE_CURRENT_SOURCE_DIR}/../../ ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(amarok_void-engine_plugin_PART_SRCS void-engine.cpp )

kde4_automoc(${amarok_void-engine_plugin_PART_SRCS})

kde4_add_plugin(amarok_void-engine_plugin WITH_PREFIX ${amarok_void-engine_plugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} amarok_void-engine_plugin )

target_link_libraries(amarok_void-engine_plugin amarok  ${KDE4_KDECORE_LIBS} )

install(TARGETS amarok_void-engine_plugin DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install(FILES  amarok_void-engine_plugin.desktop  DESTINATION ${SERVICES_INSTALL_DIR} )



