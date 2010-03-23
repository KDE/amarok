include_directories( ../.. 
					 ../../..
)

set( photos_applet_SRCS
     DragPixmapItem.cpp
     PhotosApplet.cpp
     PhotosScrollWidget.cpp
)

kde4_add_ui_files( photos_applet_SRCS photosSettings.ui )

kde4_add_plugin(amarok_context_applet_photos ${photos_applet_SRCS})
target_link_libraries( amarok_context_applet_photos amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KIO_LIBS} )

install( TARGETS amarok_context_applet_photos DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-context-applet-photos.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
