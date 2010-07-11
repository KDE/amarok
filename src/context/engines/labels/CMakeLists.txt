include_directories( ../..
                    ../../..
                    ${CMAKE_CURRENT_BINARY_DIR} # for amarok_config.h
)

set( labels_engine_SRCS
    LabelsEngine.cpp
)

kde4_add_plugin(amarok_data_engine_labels ${labels_engine_SRCS})
target_link_libraries( amarok_data_engine_labels amarokcore amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KIO_LIBS})

install( TARGETS amarok_data_engine_labels DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-labels.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
