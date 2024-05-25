set(albums_SRCS
    plugin/AlbumsPlugin.cpp
    plugin/AlbumsEngine.cpp
    plugin/AlbumItem.cpp
    plugin/TrackItem.cpp
    plugin/AlbumsModel.cpp
)

add_library(amarok_context_applet_albums SHARED ${albums_SRCS})


target_link_libraries(amarok_context_applet_albums
    amarokcore
    amaroklib
    Qt::Qml
    Qt::Quick
)

install(TARGETS amarok_context_applet_albums DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/albums)
install(FILES plugin/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/albums)

kpackage_install_package(package org.kde.amarok.albums amarok)
