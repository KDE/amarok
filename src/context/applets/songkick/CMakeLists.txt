project(context-currenttrack)

set(songkick_SRCS SongkickApplet.cpp )

include_directories( ../..
                     ../../.. )

add_library(amarok_context_applet_songkick MODULE ${songkick_SRCS})
if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_songkick PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_context_applet_songkick amarokcore amaroklib ${KDE4_PLASMA_LIBS} KF5::KIOCore)

install(TARGETS amarok_context_applet_songkick DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-songkick.desktop DESTINATION ${SERVICES_INSTALL_DIR})
