project(context-currenttrack)

set(songkick_SRCS SongkickApplet.cpp )

include_directories( ../..
                     ../../.. )

kde4_add_plugin(amarok_context_applet_songkick ${songkick_SRCS})
if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_songkick PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_context_applet_songkick amarokcore amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS amarok_context_applet_songkick DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-songkick.desktop DESTINATION ${SERVICES_INSTALL_DIR})
