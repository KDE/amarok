include_directories( ../..
                    ../../..
                    ${CMAKE_CURRENT_BINARY_DIR} # for amarok_config.h
)

set( labels_engine_SRCS
    LabelsEngine.cpp
)

add_library(amarok_data_engine_labels MODULE ${labels_engine_SRCS})
target_link_libraries( amarok_data_engine_labels amarokcore amaroklib KF${KF_MAJOR_VERSION}::Plasma KF${KF_MAJOR_VERSION}::KIOCore)

install( TARGETS amarok_data_engine_labels DESTINATION ${KDE_INSTALL_PLUGINDIR} )
install( FILES amarok-data-engine-labels.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )
