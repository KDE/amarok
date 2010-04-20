include_directories( ../..
                    ../../..
                    ${CMAKE_CURRENT_BINARY_DIR} # for amarok_config.h
)

set( videoclip_engine_SRCS
    VideoclipEngine.cpp
)

kde4_add_plugin(amarok_data_engine_videoclip ${videoclip_engine_SRCS})
target_link_libraries( amarok_data_engine_videoclip amarokcore amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KIO_LIBS})

install( TARGETS amarok_data_engine_videoclip DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-videoclip.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
