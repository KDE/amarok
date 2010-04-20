include_directories( ../../..
                    ../../../context
                    ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
)

set( current_engine_SRCS
    CurrentEngine.cpp
)

kde4_add_plugin(amarok_data_engine_current ${current_engine_SRCS})
target_link_libraries( amarok_data_engine_current amarokcore amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} )

install( TARGETS amarok_data_engine_current DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-current.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
