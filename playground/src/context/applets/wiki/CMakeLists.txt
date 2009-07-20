project(context-currenttrack)

set( wiki_SRCS WikiApplet.cpp )

include_directories( ../..
                    ../../.. )

kde4_add_ui_files( wiki_SRCS wikiSettings.ui )
kde4_add_plugin(amarok_context_applet_wiki ${wiki_SRCS})

if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_wiki PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_context_applet_wiki amaroklib ${KDE4_PLASMA_LIBS} ${QT_QTWEBKIT_LIBRARY} ${KDE4_KIO_LIBS})

install(TARGETS amarok_context_applet_wiki DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-wiki.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES amarok-wiki.svg amarok-wikiheader.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/ )
install(FILES WikiCustomStyle.css bullet.gif DESTINATION ${DATA_INSTALL_DIR}/amarok/data )
