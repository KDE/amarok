include_directories( ../..
                    ../../..
                    ${CMAKE_CURRENT_BINARY_DIR} # for amarok_config.h
)

set( labels_engine_SRCS
    LabelsEngine.cpp
)

add_library(amarok_data_engine_labels MODULE ${labels_engine_SRCS})
target_link_libraries( amarok_data_engine_labels amarokcore amaroklib KF5::Plasma KF5::KIOCore)

install( TARGETS amarok_data_engine_labels DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-labels.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
