include_directories( ../../..
                     ../../../context
                     ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
)

set( wikipedia_engine_SRCS
    WikipediaEngine.cpp
)

kde4_add_plugin(amarok_data_engine_wikipedia ${wikipedia_engine_SRCS})
target_link_libraries( amarok_data_engine_wikipedia amarokcore amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KIO_LIBS} )

install( TARGETS amarok_data_engine_wikipedia DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-wikipedia.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
