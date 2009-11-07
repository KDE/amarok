project(context-currenttrack)

set( similarArtists_SRCS SimilarArtistsApplet.cpp )

include_directories( ../..
                    ../../.. )


kde4_add_plugin(amarok_context_applet_similarArtists ${similarArtists_SRCS})
if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_similarArtists PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_context_applet_similarArtists amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTWEBKIT_LIBRARY} ${KDE4_KIO_LIBS})

install(TARGETS amarok_context_applet_similarArtists DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-similarArtists.desktop DESTINATION ${SERVICES_INSTALL_DIR})


