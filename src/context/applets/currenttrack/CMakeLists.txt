set(current_SRCS
    plugin/CurrentPlugin.cpp
    plugin/CurrentEngine.cpp
)

add_library(amarok_context_applet_currenttrack SHARED ${current_SRCS})

target_link_libraries(amarok_context_applet_currenttrack
    amarokcore
    amaroklib
    Qt::Qml
    KF5::CoreAddons
)

install(TARGETS amarok_context_applet_currenttrack DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/currenttrack)
install(FILES plugin/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/currenttrack)

kpackage_install_package(package org.kde.amarok.currenttrack amarok)
