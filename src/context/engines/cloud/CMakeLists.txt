include_directories( ../../..
                    ../../../context
                    ../../../browsers/servicebrowser
                    ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
)

set( cloud_engine_SRCS
    CloudEngine.cpp
)

kde4_add_plugin(amarok_data_engine_cloud ${cloud_engine_SRCS})
target_link_libraries( amarok_data_engine_cloud amaroklib amarokplasma ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY})

install( TARGETS amarok_data_engine_cloud DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-cloud.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
