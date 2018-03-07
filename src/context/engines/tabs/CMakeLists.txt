include_directories( ../../..
                     ../../../context
                     ${Amarok_SOURCE_DIR}/src/network
                     ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
)

set( tabs_engine_SRCS TabsEngine.cpp TabsInfo.h )

add_library(amarok_data_engine_tabs MODULE ${tabs_engine_SRCS})
target_link_libraries( amarok_data_engine_tabs amarokcore amaroklib KF5::Plasma KF5::KIOCore)

install( TARGETS amarok_data_engine_tabs DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-tabs.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
