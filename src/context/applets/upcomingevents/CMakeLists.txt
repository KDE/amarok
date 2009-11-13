project(context-upcomingevents)

include_directories( ../..
../../.. )

set( upcomingEvents_SRCS UpcomingEventsApplet.cpp LastFmEvent.cpp)

kde4_add_ui_files( upcomingEvents_SRCS UpcomingEvents.ui)
kde4_add_ui_files( upcomingEvents_SRCS upcomingEventsSettings.ui )
kde4_add_plugin(amarok_context_applet_upcomingEvents ${upcomingEvents_SRCS})

if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_upcomingEvents PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_context_applet_upcomingEvents amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTWEBKIT_LIBRARY} ${KDE4_KIO_LIBS} )

install(TARGETS amarok_context_applet_upcomingEvents DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-upcomingEvents.desktop DESTINATION ${SERVICES_INSTALL_DIR})


