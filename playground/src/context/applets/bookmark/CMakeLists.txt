project(context-bookmark)

set(bookmark_SRCS Bookmark.cpp )

include_directories( ../../../../../src/
                     ../../../../../src/meta
                     ../../../../../src/context
                     ../../../../../src/amarokurls
                     )


kde4_add_plugin(amarok_context_applet_bookmark ${bookmark_SRCS})
if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_bookmark PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_context_applet_bookmark amaroklib amarokplasma ${KDE4_KIO_LIBS})

install(TARGETS amarok_context_applet_bookmark DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-bookmark.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES amarok-bookmark.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/ )
