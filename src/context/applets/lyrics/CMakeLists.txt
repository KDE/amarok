project(context-currenttrack)

set(lyrics_SRCS
LyricsApplet.cpp)

include_directories( ../..
                    ../../..
                    ${KDE4_INCLUDE_DIR}/amarok ) # this way we don't need to prefix it with amarok/ (and it compiles this way too :)

kde4_add_plugin(amarok_context_applet_lyrics ${lyrics_SRCS})
target_link_libraries(amarok_context_applet_lyrics amaroklib ${KDE4_KIO_LIBS})

install(TARGETS amarok_context_applet_lyrics DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-lyrics.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES amarok-lyrics.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/ )
