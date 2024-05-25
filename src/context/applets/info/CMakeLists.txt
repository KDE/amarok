set(info_SRCS
    plugin/InfoPlugin.cpp
    plugin/InfoEngine.cpp
)

add_library(amarok_context_applet_info SHARED ${info_SRCS})

target_link_libraries(amarok_context_applet_info
    amarokcore
    amaroklib
    Qt::Qml
)

install(TARGETS amarok_context_applet_info DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/info)
install(FILES plugin/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/info)

kpackage_install_package(package org.kde.amarok.info amarok)
