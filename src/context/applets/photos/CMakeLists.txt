set(photos_SRCS
    plugin/PhotosPlugin.cpp
    plugin/PhotosEngine.cpp
)

add_library(amarok_context_applet_photos SHARED ${photos_SRCS})

target_link_libraries(amarok_context_applet_photos
    amarokcore
    amaroklib
    Qt::Qml
)

install(TARGETS amarok_context_applet_photos DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/photos)
install(FILES plugin/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/photos)

kpackage_install_package(package org.kde.amarok.photos amarok)
