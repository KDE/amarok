include_directories( ../../..
                     ../../../context
                     ${Amarok_SOURCE_DIR}/src/network
                     ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
)

set( tabs_engine_SRCS TabsEngine.cpp TabsInfo.h )

kde4_add_plugin(amarok_data_engine_tabs ${tabs_engine_SRCS})
target_link_libraries( amarok_data_engine_tabs amarokcore amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KIO_LIBS})

install( TARGETS amarok_data_engine_tabs DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-tabs.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
