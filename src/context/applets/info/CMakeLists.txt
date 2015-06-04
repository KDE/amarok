project(context-info)

set(info_SRCS
InfoApplet.cpp )

include_directories(
                    ../..
                    ../../..
                    )

add_library(amarok_context_applet_info MODULE ${info_SRCS})
if(APPLE)
   set_target_properties(amarok_context_applet_info PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()
target_link_libraries(amarok_context_applet_info
    amarokcore
    amaroklib
    KF5::Plasma
    ${KDE4_KDEWEBKIT_LIBS}
    Qt5::WebKitWidgets
    )

install(TARGETS amarok_context_applet_info DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-info.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES amarok-info-applet.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/ )
install(FILES InfoAppletCustomStyle.css DESTINATION ${DATA_INSTALL_DIR}/amarok/data )
