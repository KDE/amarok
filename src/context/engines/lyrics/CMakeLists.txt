include_directories( ../../..
                     ../../../context
                     ../../../dialogs
                     ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
)

set( lyrics_engine_SRCS
    LyricsEngine.cpp
)

add_library(amarok_data_engine_lyrics MODULE ${lyrics_engine_SRCS})
target_link_libraries( amarok_data_engine_lyrics amarokcore amaroklib KF5::Plasma KF5::KDELibs4Support Qt5::Gui)

install( TARGETS amarok_data_engine_lyrics DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-lyrics.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
