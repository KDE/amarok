include_directories( ../../..
                     ../../../context
                     ../../../dialogs
                     ${CMAKE_CURRENT_BINARY_DIR}/../../.. # for amarok_config.h
)

set( lyrics_engine_SRCS
    LyricsEngine.cpp
)

kde4_add_plugin(amarok_data_engine_lyrics ${lyrics_engine_SRCS})
target_link_libraries( amarok_data_engine_lyrics amarokcore amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY})

install( TARGETS amarok_data_engine_lyrics DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-data-engine-lyrics.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
