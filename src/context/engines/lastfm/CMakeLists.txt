
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../..
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../../context
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../..amarokcore
                    ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
                    ${KDE4_INCLUDE_DIR}
                    ${QT_INCLUDES} 
)

set( lastfm_engine_SRCS
    LastFmEngine.cpp 
)

kde4_add_plugin(amarok_data_engine_lastfm ${lastfm_engine_SRCS})
target_link_libraries( amarok_data_engine_lastfm amaroklib ${KDE4_LIBS} )

install( TARGETS amarok_data_engine_lastfm DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-lastfm.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
