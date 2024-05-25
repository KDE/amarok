set(lyrics_SRCS
    plugin/LyricsPlugin.cpp
    plugin/LyricsEngine.cpp
)

add_library(amarok_context_applet_lyrics SHARED ${lyrics_SRCS})

target_link_libraries(amarok_context_applet_lyrics
    amarokcore
    amaroklib
    Qt::Qml
)

install(TARGETS amarok_context_applet_lyrics DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/lyrics)
install(FILES plugin/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/lyrics)

kpackage_install_package(package org.kde.amarok.lyrics amarok)
