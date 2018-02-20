#add_subdirectory( applets )
#add_subdirectory( tools )


########### next target ###############

set( amarokpud_LIB_SRCS
    popupdropper/libpud/PopupDropper.cpp
    popupdropper/libpud/PopupDropperItem.cpp
    popupdropper/libpud/PopupDropperView.cpp
)

add_library( amarokpud SHARED ${amarokpud_LIB_SRCS} )

target_include_directories( amarokpud PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/popupdropper/libpud
    ${CMAKE_CURRENT_SOURCE_DIR}/popupdropper/libpud
)

target_link_libraries( amarokpud
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Svg
)

install( TARGETS amarokpud ${INSTALL_TARGETS_DEFAULT_ARGS} )


set(applet_qml_plugin_qml_files
    applet_qml_plugin/qmldir
    applet_qml_plugin/Applet.qml
    applet_qml_plugin/AppletHeader.qml
)

set(applet_qml_plugin_SRCS
    applet_qml_plugin/src/RatingItem.cpp
    applet_qml_plugin/src/AppletPlugin.cpp
)

#add_library(applet_qml_plugin SHARED ${applet_qml_plugin_SRCS})

#target_link_libraries(applet_qml_plugin
#    amarokcore
#    Qt5::Quick
#    KF5::WidgetsAddons
#)

#install( TARGETS applet_qml_plugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/applet )

#install( FILES ${applet_qml_plugin_qml_files} DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/applet )

#kpackage_install_package( context_qml_package org.kde.amarok.context genericqml )
