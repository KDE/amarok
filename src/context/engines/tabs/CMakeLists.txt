include_directories( ../../..
                     ../../../context
                     ${Amarok_SOURCE_DIR}/src/network
                     ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
)

set( tabs_engine_SRCS TabsEngine.cpp TabsInfo.h )

add_library(amarok_data_engine_tabs MODULE ${tabs_engine_SRCS})
target_link_libraries( amarok_data_engine_tabs amarokcore amaroklib KF${KF_MAJOR_VERSION}::Plasma KF${KF_MAJOR_VERSION}::KIOCore)

install( TARGETS amarok_data_engine_tabs DESTINATION ${KDE_INSTALL_PLUGINDIR} )
install( FILES amarok-data-engine-tabs.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
