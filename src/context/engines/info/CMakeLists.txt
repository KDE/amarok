include_directories( ../../..
                     ../../../context
                     ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
)

set( info_engine_SRCS
    InfoEngine.cpp
)

kde4_add_plugin(amarok_data_engine_info ${info_engine_SRCS})
target_link_libraries( amarok_data_engine_info amarokcore amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY})

install( TARGETS amarok_data_engine_info DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-info.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES info_frontpage.html DESTINATION ${DATA_INSTALL_DIR}/amarok/data )
install( FILES
        info_frontpage_bg.png
        info_frontpage_logo.png
        info_frontpage_shadow.png
        DESTINATION ${DATA_INSTALL_DIR}/amarok/images
       )
