set(similarartists_SRCS
    plugin/SimilarArtistsPlugin.cpp
    plugin/SimilarArtist.cpp
    plugin/SimilarArtistsEngine.cpp
)

add_library(amarok_context_applet_similarartists SHARED ${similarartists_SRCS})


target_link_libraries(amarok_context_applet_similarartists
    amarokcore
    amaroklib
    Qt::Qml
    Qt::Quick
)

install(TARGETS amarok_context_applet_similarartists DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/similarartists)
install(FILES plugin/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/similarartists)

kpackage_install_package(package org.kde.amarok.similarartists amarok)
