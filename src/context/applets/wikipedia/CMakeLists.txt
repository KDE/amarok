set(wikipedia_SRCS
    plugin/WikipediaPlugin.cpp
    plugin/WikipediaEngine.cpp
)

add_library(amarok_context_applet_wikipedia SHARED ${wikipedia_SRCS})

target_link_libraries(amarok_context_applet_wikipedia
    amarokcore
    amaroklib
    Qt5::Qml
)

install(TARGETS amarok_context_applet_wikipedia DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/wikipedia)
install(FILES plugin/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/wikipedia)

install(FILES WikipediaCustomStyle.css bullet.gif DESTINATION ${DATA_INSTALL_DIR}/amarok/data )

kpackage_install_package(package org.kde.amarok.wikipedia amarok)
