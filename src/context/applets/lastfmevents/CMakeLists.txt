project(context-lastfm)

set(lastfmevents_SRCS
LastFmEvents.cpp
 ${CMAKE_SOURCE_DIR}/src/context/Applet.cpp )

include_directories( ../..
                    ../../..
                    ${KDE4_INCLUDE_DIR}/amarok ) # this way we don't need to prefix it with amarok/ (and it compiles this way too :)

kde4_add_plugin(amarok_context_applet_lastfmevents ${lastfmevents_SRCS})
target_link_libraries(amarok_context_applet_lastfmevents amaroklib ${KDE4_KIO_LIBS})

install(TARGETS amarok_context_applet_lastfmevents DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-lastfmevents.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES amarok-lastfm.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/ )
