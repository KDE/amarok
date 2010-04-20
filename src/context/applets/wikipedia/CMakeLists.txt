project(context-currenttrack)

set( wiki_SRCS WikipediaApplet.cpp )

include_directories( ../..
                    ../../.. )

kde4_add_ui_files( wiki_SRCS wikipediaSettings.ui )

kde4_add_plugin(amarok_context_applet_wikipedia ${wiki_SRCS})
if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_wikipedia PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_context_applet_wikipedia amarokcore amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTWEBKIT_LIBRARY} ${KDE4_KIO_LIBS})

install(TARGETS amarok_context_applet_wikipedia DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-wikipedia.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES amarok-wikipedia.svg amarok-wikipediaheader.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/ )
install(FILES WikipediaCustomStyle.css bullet.gif DESTINATION ${DATA_INSTALL_DIR}/amarok/data )
