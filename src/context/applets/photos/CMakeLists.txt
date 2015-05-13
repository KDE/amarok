include_directories(
    ${Amarok_SOURCE_DIR}/src
    ${Amarok_SOURCE_DIR}/src/network
)

set( photos_applet_SRCS
     DragPixmapItem.cpp
     PhotosApplet.cpp
     PhotosScrollWidget.cpp
)

ki18n_wrap_ui( photos_applet_SRCS photosSettings.ui )

add_library(amarok_context_applet_photos MODULE ${photos_applet_SRCS})
target_link_libraries( amarok_context_applet_photos
    amarokcore
    amaroklib
    KF5::Plasma
)

install( TARGETS amarok_context_applet_photos DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES amarok-context-applet-photos.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
