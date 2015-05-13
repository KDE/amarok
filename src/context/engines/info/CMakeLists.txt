include_directories( ../../..
                     ../../../context
                     ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
)

set( info_engine_SRCS
    InfoEngine.cpp
)

add_library(amarok_data_engine_info MODULE ${info_engine_SRCS})
target_link_libraries( amarok_data_engine_info amarokcore amaroklib KF5::Plasma KF5::KDELibs4Support Qt5::Gui)

install( TARGETS amarok_data_engine_info DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-info.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES info_frontpage.html DESTINATION ${DATA_INSTALL_DIR}/amarok/data )
install( FILES
        info_frontpage_bg.png
        info_frontpage_logo.png
        info_frontpage_shadow.png
        DESTINATION ${DATA_INSTALL_DIR}/amarok/images
       )
