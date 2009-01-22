include_directories( ../../..
                     ../../../context
                     ../../../browsers/servicebrowser
                     ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
)

set( service_engine_SRCS
    ServiceEngine.cpp
)

kde4_add_plugin(amarok_data_engine_service ${service_engine_SRCS})
target_link_libraries( amarok_data_engine_service amaroklib amarokplasma ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY})

install( TARGETS amarok_data_engine_service DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-service.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES service_info_frontpage.html DESTINATION ${DATA_INSTALL_DIR}/amarok/data )
install( FILES
        service_info_frontpage_bg.png
        service_info_frontpage_logo.png
        service_info_frontpage_shadow.png
        DESTINATION ${DATA_INSTALL_DIR}/amarok/images
       )
