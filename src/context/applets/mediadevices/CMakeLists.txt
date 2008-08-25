project(context-mediadevices)

set(mediadevices_SRCS MediaDevicesApplet.cpp )

include_directories( ../..
                    ../../.. )

kde4_add_plugin(amarok_context_applet_mediadevices ${mediadevices_SRCS})
if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_mediadevices PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_context_applet_mediadevices amaroklib amarokplasma ${KDE4_KIO_LIBS})

install(TARGETS amarok_context_applet_mediadevices DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-mediadevices.desktop DESTINATION ${SERVICES_INSTALL_DIR})
