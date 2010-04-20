project(context-albums)

set(albums_SRCS Albums.cpp AlbumsView.cpp AlbumItem.cpp TrackItem.cpp AlbumsModel.cpp)

include_directories(
                    ../..
                    ../../..
                    )

kde4_add_plugin(amarok_context_applet_albums ${albums_SRCS})
if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_albums PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_context_applet_albums amarokcore amarokcore amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS amarok_context_applet_albums DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-albums.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES amarok-albums.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/ )
