
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../..
	${CMAKE_CURRENT_SOURCE_DIR}/../../plugin
	${CMAKE_CURRENT_SOURCE_DIR}/../../amarokcore
	${CMAKE_CURRENT_BINARY_DIR}/../.. ${KDE4_INCLUDE_DIR}
        ${QT_INCLUDES} ${XINE_INCLUDE_DIR} )


########### next target ###############

set(amarok_xine-engine_PART_SRCS xine-engine.cpp xine-scope.c xine-config.cpp )

kde4_add_ui3_files(amarok_xine-engine_PART_SRCS
   xineconfigbase.ui
)

kde4_add_kcfg_files(amarok_xine-engine_PART_SRCS xinecfg.kcfgc)

kde4_automoc(${amarok_xine-engine_PART_SRCS})

kde4_add_plugin(amarok_xine-engine WITH_PREFIX ${amarok_xine-engine_PART_SRCS})


target_link_libraries(amarok_xine-engine  ${KDE4_KDEUI_LIBS}
        ${KDE4_KDE3SUPPORT_LIBS} amarok ${XINE_LIBRARY} )

install(TARGETS amarok_xine-engine DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES xinecfg.kcfg DESTINATION ${KCFG_INSTALL_DIR})
install( FILES  amarok_xine-engine.desktop DESTINATION ${SERVICES_INSTALL_DIR})
