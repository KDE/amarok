project(context-albums)

set(albums_SRCS Albums.cpp AlbumsView.cpp AlbumItem.cpp TrackItem.cpp AlbumsModel.cpp)

include_directories(
                    ../..
                    ../../..
                    )

add_library(amarok_context_applet_albums MODULE ${albums_SRCS})
if(APPLE)
   set_target_properties(amarok_context_applet_albums PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()
target_link_libraries(amarok_context_applet_albums
    amarokcore
    amaroklib
    KF5::Plasma
)

install(TARGETS amarok_context_applet_albums DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-albums.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES amarok-albums.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/ )
