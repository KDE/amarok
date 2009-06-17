include_directories( ../../..
                     ../../../context
                     ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
)

set( wiki_engine_SRCS
    WikiEngine.cpp
)

kde4_add_plugin(amarok_data_engine_wiki ${wiki_engine_SRCS})
target_link_libraries( amarok_data_engine_wiki amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KIO_LIBS} )

install( TARGETS amarok_data_engine_wiki DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-wiki.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
