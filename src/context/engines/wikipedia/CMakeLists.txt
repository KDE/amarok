include_directories( ../../..
                     ${CMAKE_SOURCE_DIR}/src/context
                     ${CMAKE_SOURCE_DIR}/src/network
                     ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
)

set( wikipedia_engine_SRCS
    WikipediaEngine.cpp
)

add_library(amarok_data_engine_wikipedia MODULE ${wikipedia_engine_SRCS})
target_link_libraries( amarok_data_engine_wikipedia amarokcore amaroklib KF5::Plasma KF5::KIOCore )

install( TARGETS amarok_data_engine_wikipedia DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-wikipedia.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
