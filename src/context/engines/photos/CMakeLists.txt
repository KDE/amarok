include_directories( ../..
                    ../../..
                    ${CMAKE_CURRENT_BINARY_DIR} # for amarok_config.h
)

set( photos_engine_SRCS
    PhotosEngine.cpp
)

kde4_add_plugin(amarok_data_engine_photos ${photos_engine_SRCS})
target_link_libraries( amarok_data_engine_photos amarokcore amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KIO_LIBS})

install( TARGETS amarok_data_engine_photos DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-photos.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
